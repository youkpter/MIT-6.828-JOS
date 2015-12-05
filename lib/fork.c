// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
    /* cprintf("pgfault va:%08x, eip:%08x\n", addr, utf->utf_eip); */
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
    uint32_t pte_perm = uvpt[PGNUM(addr)] & PTE_SYSCALL;
    if (!((err & FEC_WR) && (pte_perm & PTE_COW)))
        panic("pgfault: va: %08x\n", addr);

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
    pte_perm &= ~PTE_COW;
    pte_perm |= PTE_W;
    addr = ROUNDDOWN(addr, PGSIZE);
    int ret = sys_page_alloc(0, PFTEMP, pte_perm);
    if (ret < 0)
        panic("pgfault: sys_page_alloc: %e\n", ret);
    memmove((void *)PFTEMP, addr, PGSIZE);
    ret = sys_page_map(0, (void *)PFTEMP, 0, addr, pte_perm);
    if (ret < 0)
        panic("pgfault: sys_page_map: %e\n", ret);
    sys_page_unmap(0, (void *)PFTEMP);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
    //cprintf("debug:lib/fork/duppage:envid:%8x, pn: %8x\n", envid, pn);
    // I first use the next line to get PTE permissions, but it doesn't work
    // because sys_page_map's(below) third parameter must not more than
    // PTE_SYSCALL
    //uint32_t pte_perm = uvpt[pn] & 0xFFF;
    uint32_t pte_perm = uvpt[pn] & PTE_SYSCALL;
    uint32_t old_perm = pte_perm;
    void *va = (void *)(pn * PGSIZE);
    if (pte_perm & PTE_W || pte_perm & PTE_COW) {
        pte_perm |= PTE_COW;
        pte_perm &= ~PTE_W;
        /* cprintf("duppage: va(%08x) set PTE_COW:%0x\n", va, pte_perm); */
    }
    // Be careful! don't exchange the next two invocations of
    // sys_page_map. Especially, when va belongs to stack memory
    // if you do, the child would get a corrupted stack
    r = sys_page_map(0, va, envid, va, pte_perm);
    if (r < 0)
        panic("duppage: %e\n", r);
    if (pte_perm != old_perm)
        sys_page_map(0, va, 0, va, pte_perm);
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
    set_pgfault_handler(pgfault);
    envid_t envid = sys_exofork();
    if (envid < 0)
        panic("fork: %e\n", envid);
    else if (envid > 0) {
        // alloc a new page for child's exception stack
        int ret = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE),
                PTE_U | PTE_W | PTE_P);
        if (ret < 0)
            panic("fork: sys_page_alloc: %e\n", ret);
        // copy address space
        unsigned i;
        for (i = UTEXT; i < UXSTACKTOP - PGSIZE; i += PGSIZE)
            if((uvpd[PDX(i)] & PTE_P) && (uvpt[PGNUM(i)] & PTE_P))
                duppage(envid, PGNUM(i));

        extern void _pgfault_upcall(void);
        //cprintf("_pgfault_upcall:%08x\n", _pgfault_upcall);
        sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
        ret = sys_env_set_status(envid, ENV_RUNNABLE);
        if (ret < 0)
            panic("sys_env_set_status: %e\n", ret);

        return envid;
    } else {
        thisenv = &envs[ENVX(sys_getenvid())];
        return 0;
    }
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}

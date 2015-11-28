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
    // because sys_page_map's(below) third parameter must be less than
    // PTE_SYSCALL(这是干嘛用的)
    // 难道Dirty位不应该复制吗?
    //uint32_t pte_perm = uvpt[pn] & 0xFFF;
    uint32_t pte_perm = uvpt[pn] & PTE_SYSCALL;
    void *va = (void *)(pn * PGSIZE);
    /* cprintf("debug original pte_perm:%x\n", pte_perm); */
    if (pte_perm & PTE_W || pte_perm & PTE_COW) {
        //之前一直直接对*ptep进行设置,还总找不到出错原因
        //尝试在各个地方使用cprintf输出信息,一步步把范围缩小
        //中途也有使用gdb单步调试,但是在env_run()的unlock_kernel处总是出问题
        //要么内核直接panic,要么就是执行异常(重复执行env_pop_tf,有时候好不容易执
        //行到了forktree程序,居然发现和obj/user/forktree.asm对不上号
        //花了一下午,没办法只有使用cprintf,一步步定位,才猛然发现我居然直接对页表进行操作
        pte_perm |= PTE_COW;
        pte_perm &= ~PTE_W;
        /* cprintf("duppage: va(%08x) set PTE_COW:%0x\n", va, pte_perm); */
    }
    //cprintf("debug duppage: va: %x, pte_perm: %x\n", va, pte_perm);
    r = sys_page_map(0, va, envid, va, pte_perm);
    if (r < 0)
        panic("duppage: %e\n", r);
    r = sys_page_map(0, va, 0, va, pte_perm);
    //cprintf("duppage is returnning, va: %08x, pte_perm: %x\n", va, uvpt[pn] & 0xFFF);
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
        duppage(envid, (USTACKTOP - PGSIZE) / PGSIZE);
        /* cprintf("there should have some pgfaults\n"); */
        // 从UTEXT开始,我只拷贝那些父进程中已存在的页的映射(PTE_P),一旦遇到
        // 无映射的页就终止
        // 所以上面USTACK页的映射和下面的循环要分开
        unsigned i;
        for (i = (UTEXT) / PGSIZE; i < (USTACKTOP - PGSIZE) / PGSIZE; i++)
            if(uvpt[i] & PTE_P) {
                /* cprintf("duppage %x\n", i * PGSIZE); */
                duppage(envid, i);
                //cprintf("there were some pgfaults above\n");
            } else
                break;

        //在trap.c中查看tp_eip的值才发现和pfentry.S中的_pgfaul_upcall不同
        //再比较发现eip等于fork中的pgfault,之后发现不能用这个函数注册用户级
        //的page fault handler,要直接注册lib/pfentry.S中的_pgfault_upcall
        extern void _pgfault_upcall(void);
        //cprintf("_pgfault_upcall:%08x\n", _pgfault_upcall);
        sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
        ret = sys_env_set_status(envid, ENV_RUNNABLE);
        if (ret < 0)
            panic("sys_env_set_status: %e\n", ret);

        return envid;
    } else {
        thisenv = &envs[ENVX(sys_getenvid())];
        //cprintf("child(%08x) return\n", thisenv->env_id);
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

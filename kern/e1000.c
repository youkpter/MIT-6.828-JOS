#include <inc/error.h>
#include <inc/string.h>
#include <kern/e1000.h>
#include <kern/pmap.h>
#include <kern/sched.h>

// LAB 6: Your driver code here
struct e1000_tx_desc tx_desc[TXNUM];   // transmit descriptor ring
char tx_buf[TXNUM][TX_BSIZE];          // transmit buffer array
struct e1000_rx_desc rx_desc[RXNUM];   // receive descriptor ring
char rx_buf[RXNUM][RX_BSIZE];          // receive buffer array
volatile uint32_t *bar0;

// Send a packet starting at 'buf' with len long
int
pkt_try_send(const void *buf, size_t len)
{
    uint32_t tail = bar0[E1000_TDT];
    void *buf_addr;

    if (tx_desc[tail].upper.data & E1000_TXD_STAT_DD) {
        buf_addr = &tx_buf[tail][0];
        tx_desc[tail].lower.data = (E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP);
        tx_desc[tail].lower.flags.length = len;
        memmove(buf_addr, buf, len);
        tx_desc[tail].upper.data = 0; // clear DD bit
        bar0[E1000_TDT] = (tail + 1) % TXNUM;
        return 0;
    }
    return -E_AGAIN;
}

// Receive a packet into buffer starting at 'dst'
// Caller must insure the size of buffer not less than the
// largest packet size(RX_BSIZE)
// Returns packet's length
int
pkt_recv(void *dst)
{
    uint32_t tail;
    void *buf_addr;

    /* cprintf("pkt_recv starting\n"); */
    tail = (bar0[E1000_RDT] + 1) % RXNUM;
    if (rx_desc[tail].status & 1) { // DD bit is set
        buf_addr = &rx_buf[tail][0];
        memmove(dst, buf_addr, rx_desc[tail].length);
        bar0[E1000_RDT] = tail;
        return rx_desc[tail].length;
    }
    return -E_AGAIN;
}

int
pci_attachfn(struct pci_func *f)
{
    pci_func_enable(f);
    bar0 = mmio_map_region(f->reg_base[0], f->reg_size[0]);
    int i;

    // Transmit Initialization
    for(i = 0; i < TXNUM; i++) {
        tx_desc[i].buf_addr = PADDR(&tx_buf[i][0]);
        tx_desc[i].upper.data = E1000_TXD_STAT_DD;
    }
    bar0[E1000_TDBAL] = PADDR(tx_desc);
    bar0[E1000_TDBAH] = 0;
    bar0[E1000_TDLEN] = sizeof(tx_desc);
    bar0[E1000_TDH]   = 0;
    bar0[E1000_TDT]   = 0;
    bar0[E1000_TCTL]  = E1000_TCTL_EN | E1000_TCTL_CT | E1000_TCTL_COLD;
    bar0[E1000_TIPG]  = E1000_TIPG_IPGT | E1000_TIPG_IPGR1 | E1000_TIPG_IPGR2;

    // Receive Initialization
    for(i = 0; i < RXNUM; i++)
        rx_desc[i].buf_addr = PADDR(&rx_buf[i][0]);
    // hard code qemu's default MAC address(52:54:00:12:34:56)
    bar0[E1000_RAL]   = 0x12005452;
    bar0[E1000_RAH]   = 0x00005634 | E1000_RAH_AV;
    bar0[E1000_IMS]   = 0;    //disable interrupt
    bar0[E1000_RDBAL] = PADDR(rx_desc);
    bar0[E1000_RDBAH] = 0;
    bar0[E1000_RDLEN] = sizeof(rx_desc);
    bar0[E1000_RDH]   = 0;
    bar0[E1000_RDT]   = RXNUM - 1; // We can't set to 0 as transmit do, why?
    bar0[E1000_RCTL]  = E1000_RCTL_EN | E1000_RCTL_SZ_1024 | E1000_RCTL_SECRC;
    return 0;
}


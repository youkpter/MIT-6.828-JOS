#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>
#define TXNUM    16     // transmit descriptor list size
#define RXNUM    128    // receive descriptor list size
#define TX_BSIZE 1518   // max length of packet
#define RX_BSIZE 1024   // Receiver Buffer Size

// E1000 register set, divided by 4 for use as uint32_t[] indics
#define E1000_CTRL    (0x00000/4)     // Device Control
#define E1000_STATUS  (0x00008/4)     // Device Status
#define E1000_IMS     (0x000D0/4)     // Interrupt Mask Set
#define E1000_TCTL    (0x00400/4)     // Transmit Control
#define E1000_TIPG    (0x00410/4)     // Transmit IPG
#define E1000_TDBAL   (0x03800/4)     // Transmit Descriptor Base Low
#define E1000_TDBAH   (0x03804/4)     // Transmit Descriptor Base High
#define E1000_TDLEN   (0x03808/4)     // Transmit Descriptor Base Length
#define E1000_TDH     (0x03810/4)     // Transmit Descriptor Base Head
#define E1000_TDT     (0x03818/4)     // Transmit Descriptor Base Tail
#define E1000_RCTL    (0x00100/4)     // Receive Control
#define E1000_RDBAL   (0x02800/4)     // Receive Descriptor Base Low
#define E1000_RDBAH   (0x02804/4)     // Receive Descriptor Base High
#define E1000_RDLEN   (0x02808/4)     // Receive Descriptor Length
#define E1000_RDH     (0x02810/4)     // Receive Descriptor Head
#define E1000_RDT     (0x02818/4)     // Receive Descriptor Tail
#define E1000_RAL     (0x05400/4)     // Receive Address Low
#define E1000_RAH     (0x05404/4)     // Receive Address High

// Transmit Control
#define E1000_TCTL_EN        0x00000002    // enable tx
#define E1000_TCTL_CT        0x00000100    // collision threshold 10h
#define E1000_TCTL_COLD      0x00040000    // collision distance 40h

// Receive Control
#define E1000_RCTL_EN        0x00000002    // Receiver Enable
#define E1000_RCTL_SZ_1024   0x00010000    // rx buffer size 1024
#define E1000_RCTL_SECRC     0x04000000    // Strip Ethernet CRC

// Inter-packet gap
#define E1000_TIPG_IPGT      0x00000010    // set IPGT to 10h
#define E1000_TIPG_IPGR1     0x00001000    // set IPGR1 to 4
#define E1000_TIPG_IPGR2     0x00600000    // set IPGR2 to 6

// Transmit Descriptor
#define E1000_TXD_CMD_RS     0x08000000    // Enable Report Status
#define E1000_TXD_CMD_EOP    0x01000000    // End of Packet
#define E1000_TXD_STAT_DD    0x00000001    // Descriptor Done

// Receive Address
#define E1000_RAH_AV         0x80000000    // Receive descriptor valid

int pci_attachfn(struct pci_func *pcif);
int pkt_try_send(const void *buf, size_t len);
int pkt_recv(void *buf);

struct e1000_tx_desc {
    uint64_t buf_addr;
    union {
        uint32_t data;
        struct {
            uint16_t length;
            uint8_t cso;
            uint8_t cmd;
        } flags;
    } lower;
    union {
        uint32_t data;
        struct {
            uint8_t status;
            uint8_t css;
            uint16_t special;
        } fields;
    }upper;
};

struct e1000_rx_desc {
    uint64_t buf_addr;
    uint16_t length;
    uint16_t checksun;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
};

#endif	// JOS_KERN_E1000_H

#include <inc/lib.h>
// Send the content at 'buf' with 'len' long to network driver
void
pkt_send(const void *buf, size_t len)
{
    int ret;
    while (1) {
        ret = sys_pkt_try_send(buf, len);
        if (!ret)
            break;
        if (ret != -E_AGAIN)
            panic("pkt__send: %e\n", ret);
        sys_yield();
    }
}

// Receive a packet and map it at 'dst'
int
pkt_recv(void *dst)
{
    int ret;
    while(1) {
        ret = sys_pkt_recv(dst);
        if (ret > 0)
            return ret;
        sys_yield();
    }
}

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
    // if (argc < 2 || strcmp(argv[1], "-h") == 0) {
    //     cprintf("usage: send [string]\n");
    //     exit();
    // }
    int r;
    int i;
    char buf[5] = "good";
    /* for(i = 0; i < 3; i++) */
        r = sys_pkt_try_send(buf, 5);
    if (r == 0) {
        cprintf("send success\n");
    } else
        cprintf("send %e\n", r);

}

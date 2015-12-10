#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
    envid_t from_env;
    int ret;
    while (1) {
        ret = ipc_recv(&from_env, &nsipcbuf, NULL);
        // cprintf("data[0]: %x, len: %d\n",
        //         nsipcbuf.pkt.jp_data[0],
        //         nsipcbuf.pkt.jp_len);
        if (ret != NSREQ_OUTPUT)
            panic("output: Unexpected IPC\n");
        pkt_send(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len);
    }
}

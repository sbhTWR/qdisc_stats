/*
 * nlcomm.c 
 * Uses netlink to fetch qdisc stats from kernel.
 * 
 * Author: Shubham Tiwari <f2016935@pilani.bits-pilani.ac.in>
 */ 

#include <linux/rtnetlink.h>
#include <linux/gen_stats.h>
#include <linux/pkt_sched.h>
#include <bits/sockaddr.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

/* 
struct sockaddr_nl {
  sa_family_t     nl_family; // AF_NETLINK
  unsigned short  nl_pad;    // zero
  __u32           nl_pid;    // process pid
  __u32           nl_groups; // multicast grps mask
};
*/
struct sockaddr_nl src_addr, dest_addr;

/* 
struct nlmsghdr {
  __u32  nlmsg_len;   //Length of msg incl. hdr
  __u16  nlmsg_type;  //Message content
  __u16  nlmsg_flags; //Additional flags
  __u32  nlmsg_seq;   //Sequence number
  __u32  nlmsg_pid;   //Sending process PID
}
*/
//struct nlmsghdr *nlh = NULL;

/* vector of data to send */
/* struct iovec {
	void __user *iov_base;	// BSD uses caddr_t (1003.1g requires void *) 
	__kernel_size_t iov_len; // Must be size_t (1003.1g) 
};
*/
//struct iovec iov;

/* number of I/O vector entries */
//size_t iovlen;

/* socket file descriptor */ 
int sock_fd;

/* struct msghdr {
  void *msg_name;        //Address to send to
  socklen_t msg_namelen; //Length of address data

  struct iovec *msg_iov; //Vector of data to send
  size_t msg_iovlen;     //Number of iovec entries

  void *msg_control;     //Ancillary data
  size_t msg_controllen; //Ancillary data buf len

  int msg_flags;         //Flags on received msg
};
*/
//struct msghdr msg;

int main(int argc, char *argv[]) {

    /* Open a netlink socket */ 
    sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sock_fd < 0) {
        printf("\n Failed to open netlink socket");
        return -1;
    }

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; /* For linux kernel */
    dest_addr.nl_groups = 0; /* unicast */


    /* Once socket is opened, it has to be bound to a local address */
    int rtnl = bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr));
    if (rtnl < 0) {
        printf("\n Failed to bind local address to the socket");
        return -1;
    }

    
    /* +----------+
     * | nlmsghdr |
     * |----------|
     * |   len    |
     * |   type   |
     * |   flags  |
     * |   seq    |
     * |   pid    |
     * +----------+
     * 
     * +----------------+
     * |    msghdr      |
     * +----------------+
     * | msg_name       |
     * | msg_namelen    |
     * | msg_iov        | [nlmsghdr + payload]
     * | msg_iovlen     |
     * | msg_control    |
     * | msg_controllen |
     * | msg_flags      |
     * +----------------+
     * 
     * msghdr is the actual message sent through the sendmsg() function
     * and recieved using recvmsg().
     * Message header name is a bit mis-leading in that its just a header. 
     * This header data actually contains the vector to the whole netlink 
     * message.
     * msg_iov is the pointer to the buffer (created using malloc) containing 
     * the actual netlink message. msg_iovlen is the number of iovecs (I/O vectors) 
     * present in the message.
     * 
     * Example: A request message consists of two iovecs, first one pointing to the netlink 
     * message header (struct nlmsghdr) and the second vector pointing to the request message. 
     * Request message is formed using the struct appropriate to what the programmer wants 
     * to achieve. 
     * nlmsghdr->len is the sum of size of nlmsghdr and the request message(s). Netlink socket 
     * is opened and is bound to the source (in our case its our userspace application, whos pid
     * is fetched using getpid()). msghdr->msg_name is the address of struct sockaddr_nl
     * inititalized with the destinations address (We put 0 in pid to refer to kernel).
     * msghdr->msg_namelen is the size of sockaddr_nl.
     * 
     */

    /* Netlink message format
     * 
     * +----------+-----+---------------+-----+-------------+-----+--------------+
     * | nlmsghdr | Pad | Family Header | Pad | Attr Header | Pad | Attr payload |
     * +----------+-----+---------------+-----+-------------+-----+--------------+
     * 
     * There can be any number of messages of in this sequence.
     * Moreover, Attr header and Attr payload can also be in sequence together, forming 
     * multiple attributes in the same netlink message.
     */

    struct tcmsg t = { .tcm_family = AF_UNSPEC };
    char d[IFNAMSIZ] = {};
    strncpy(d, "enp7s0", sizeof(d)-1);

    if (d[0]) {
        t.tcm_ifindex = if_nametoindex(d);
    }

    /* rtnl_dump_request(&rth, RTM_GETQDISC, &t, sizeof(t)) < 0) */
    void *req = (void *)&t;
    int type = RTM_GETQDISC;
    int len = sizeof(t);

    struct nlmsghdr nlh = {
        .nlmsg_len = NLMSG_LENGTH(len),
        .nlmsg_type = type,
        .nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST,
        .nlmsg_seq = 0,
    };
    struct iovec iov[2] = {
        { .iov_base = &nlh, .iov_len = sizeof(nlh) },
        { .iov_base = req, .iov_len = len }
    };

    struct msghdr msg = {
        .msg_name = &dest_addr,
        .msg_namelen = sizeof(dest_addr),
        .msg_iov = iov,
        .msg_iovlen = 2,
    };

    /* message has been framed, now send the message out */ 
    sendmsg(sock_fd, &msg, 0);
    printf("\nWaiting for message from the kernel!");

    /* Read message from the kernel */

    struct sockaddr_nl nladdr;
    struct iovec iovrecv;
    iovrecv.iov_base = NULL;
    iovrecv.iov_len = 0;

    struct msghdr msg_recv = {
        .msg_name = &nladdr,
        .msg_namelen = sizeof(nladdr),
        .msg_iov = &iovrecv,
        .msg_iovlen = 1,
    };

    char *buf;

    /* Determine bufer length for recieving the message */

    int recvlen = recvmsg(sock_fd, &msg_recv, MSG_PEEK | MSG_TRUNC);
    if (recvlen < 32768)
        recvlen = 32768;

    /* Allocate buffer for recieving the message */
    buf = malloc(recvlen);

    /* Reset the I/O vector */
    iovrecv.iov_base = buf;
    iovrecv.iov_len = recvlen;

    /* Now recieve the message */
    recvlen = recvmsg(sock_fd, &msg_recv, 0);

    if (recvlen <0) {
        free(buf);
        printf("\n Error during netlink msg recv: len < 0");
        exit(0);
    }

    /* At this point of time, buf contains the message */

    printf("\nRecieved msg len: %d", recvlen);
    printf("\nRecieved message payload: %s", (char *)buf);

    struct nlmsghdr *h = (struct nlmsghdr *)buf;
    int msglen = recvlen;

    printf("\nnlmsg_type: %d", h->nlmsg_type);

    struct tcmsg *tcrecv = NLMSG_DATA(h);


    while (NLMSG_OK(h, msglen)) {

        printf ("\n -------------- \n");
        if (h->nlmsg_type == NLMSG_DONE) {
            printf("\nDone iterating through the netlink message");
            break;
        }

        if (h->nlmsg_type == NLMSG_ERROR) {
            printf("\nError message encountered!");
            break;
        }

        /* Parse this message */
        struct tcmsg *tcrecv = NLMSG_DATA(h);

        struct rtattr *tb[TCA_MAX+1];
        struct qdisc_util *q;

        /* TODO: ignore this message if not of RTM_NEWQDISC || RTM_DELQDISC type 
           NOTE: reply of RTM_QDISCGET is of type RTM_NEWQDISC.
        */


        int len = h->nlmsg_len;

        len -= NLMSG_LENGTH(sizeof(*tcrecv));

        if (len <0) {
            printf("Wrong len %d\n", len);
            exit(0);
        }

        /* Parse attributes */
        struct rtattr *rta = TCA_RTA(tcrecv);

        memset(tb, 0, sizeof(struct rtattr *)*(TCA_MAX+1));
        unsigned short type;
        while (RTA_OK(rta, len)) {
            type = rta->rta_type;
            if ((type <= TCA_MAX) && (!tb[type])) {
                tb[type] = rta;    
            }

            rta = RTA_NEXT(rta, len);
        }

        if (tb[TCA_KIND] == NULL) {
            printf("\nNULL KIND!");
            exit(0);
        }

        /* convert to string -> (const char *)RTA_DATA(rta); */

        printf("\n qdisc %s", (const char *)RTA_DATA(tb[TCA_KIND]));
        printf("\n handle: %x", tcrecv->tcm_handle >> 16);

        printf("[%08x]", tcrecv->tcm_handle);

        /* Print dev name using if_indextoname */
    
        printf("dev %s", if_indextoname(tcrecv->tcm_ifindex, d));

        if (tcrecv->tcm_parent == TC_H_ROOT)
            printf(" root ");

        if (strcmp("pfifo_fast", RTA_DATA(tb[TCA_KIND])) == 0) {
            /* get prio qdisc kind */

        } else {
            /* get tb[TCA_KIND] qdsic kind */
        }

        /* Print queue stats */
        struct rtattr *tbs[TCA_STATS_MAX + 1];

        /* Parse nested attr using TCA_STATS_MAX */
        rta = RTA_DATA(tb[TCA_STATS2]);
        len = RTA_PAYLOAD(tb[TCA_STATS2]);

        memset(tbs, 0, sizeof(struct rtattr *)*(TCA_STATS_MAX+1));
        
        while (RTA_OK(rta, len)) {
            type = rta->rta_type;
            if ((type <= TCA_STATS_MAX) && (!tbs[type])) {
                tbs[type] = rta;    
            }

            rta = RTA_NEXT(rta, len);
        }

        /* tc stats structs present in linux/gen_stats.h 
           They have been pasted below for reference 
           enum {
            TCA_STATS_UNSPEC,
            TCA_STATS_BASIC,
            TCA_STATS_RATE_EST,
            TCA_STATS_QUEUE,
            TCA_STATS_APP,
            TCA_STATS_RATE_EST64,
            TCA_STATS_PAD,
            TCA_STATS_BASIC_HW,
            __TCA_STATS_MAX,
            };
            #define TCA_STATS_MAX (__TCA_STATS_MAX - 1)

            struct gnet_stats_basic {
                __u64	bytes;
                __u32	packets;
            };

            struct gnet_stats_rate_est {
                __u32	bps;
                __u32	pps;
            };

            struct gnet_stats_rate_est64 {
                __u64	bps;
                __u64	pps;
            };

            struct gnet_stats_queue {
                __u32	qlen;
                __u32	backlog;
                __u32	drops;
                __u32	requeues;
                __u32	overlimits;
            };
        */

        if (tbs[TCA_STATS_BASIC]) {
            struct gnet_stats_basic bs = {0};

            memcpy(&bs, RTA_DATA(tbs[TCA_STATS_BASIC]), MIN(RTA_PAYLOAD(tbs[TCA_STATS_BASIC]), sizeof(bs)));
            printf("bytes: %llu packets %u", bs.bytes, bs.packets);
            
        }

        if (tbs[TCA_STATS_QUEUE]) {
            struct gnet_stats_queue q = {0};

            memcpy(&q, RTA_DATA(tbs[TCA_STATS_QUEUE]), MIN(RTA_PAYLOAD(tbs[TCA_STATS_QUEUE]), sizeof(q)));
            /* From here we can print all the data in the struct q */

            printf(" qlen: %d drops: %d", q.qlen, q.drops);
        }

        /* Print the rate, try est64 followed by est */ 
        if (tbs[TCA_STATS_RATE_EST64]) {
            struct gnet_stats_rate_est64 re = {0};
            memcpy(&re, RTA_DATA(tbs[TCA_STATS_RATE_EST64]),
		       MIN(RTA_PAYLOAD(tbs[TCA_STATS_RATE_EST64]),
                    sizeof(re)));

            printf(" rate (bps): %llu rate (pps): %llu", re.bps, re.pps);        
        }
        else if (tbs[TCA_STATS_RATE_EST]) {
            struct gnet_stats_rate_est re = {0};
            memcpy(&re, RTA_DATA(tbs[TCA_STATS_RATE_EST]),
		       MIN(RTA_PAYLOAD(tbs[TCA_STATS_RATE_EST]),
                    sizeof(re)));

            printf(" rate (bps): %u rate (pps): %u", re.bps, re.pps);        

        }

        /* For backward compatibility. In the newer versions of kernel, struct tc_stats has been broken down 
           into more particular structs (listed above). The previous versions of kernel encapsulated all the 
           stats in struct tc_stats itself.
         */

        if (tb[TCA_STATS]) {
            struct tc_stats st = {};
            memcpy(&st, RTA_DATA(tb[TCA_STATS]), MIN(RTA_PAYLOAD(tb[TCA_STATS]), sizeof(st)));

            /* bps and pps were not showing up in the rate estimator above, so instead printed it 
               from here. All the stats which have been printed above can be printed from here too. 
               I just tried out bps and pps because they were not showing up above */

            printf(" rate (bps): %u rate (pps): %u", st.bps, st.pps);

            /* can print any of the stats present in struct tc_stats here */

        }

        h = NLMSG_NEXT(h, msglen);
    }
    free(buf);
    close(sock_fd);
    printf("\n");
}      


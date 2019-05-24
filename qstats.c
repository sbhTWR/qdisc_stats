/*
 * qstats.c 
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

#include "nlcomm.h"


int main(int argc, char *argv[]) {

    /* Open a netlink socket */ 
    int sock_fd = nl_sock();

    /* Using nl_print_qdisc_stats as the callback function to parse netlink message */
    nl_dump_qdisc_request(sock_fd, nl_print_qdisc_stats);
    
    close(sock_fd);
    printf("\n");
}  
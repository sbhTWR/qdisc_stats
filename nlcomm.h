/*
 * nlcomm.h 
 * 
 * Author: Shubham Tiwari <f2016935@pilani.bits-pilani.ac.in>
 */ 

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

/* 
struct nlmsghdr {
  __u32  nlmsg_len;   //Length of msg incl. hdr
  __u16  nlmsg_type;  //Message content
  __u16  nlmsg_flags; //Additional flags
  __u32  nlmsg_seq;   //Sequence number
  __u32  nlmsg_pid;   //Sending process PID
}
*/

/* vector of data to send */
/* struct iovec {
	void __user *iov_base;	// BSD uses caddr_t (1003.1g requires void *) 
	__kernel_size_t iov_len; // Must be size_t (1003.1g) 
};
*/

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

int nl_sock();
int nl_dump_qdisc_request(int sock_fd, void (*cb)(char *, int));
void nl_print_qdisc_stats(char *buf, int recvlen);
void nl_parse_attr(struct rtattr *rta, int len, struct rtattr *tb[], int max);
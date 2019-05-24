# qdisc_stats
Uses netlink to fetch queueing discipline stats from kernel. A large part of the code has been inspired from iproute2 userspace utility.

TODO: Improve error handling and make the code modular.

### Prerequisites
Linux kernel headers are required to run this program.

Update the package repository:

```sudo apt update```

Install the approriate kernel headers for the kernel version in your system:

```sudo apt install linux-headers-$(uname -r)```

Check if the kernel headers (with the correct version) have been installed:

```ls -l /usr/src/linux-headers-$(uname -r)```

### Compile
Execute the following command in the terminal in the path containing nlcomm.c:

```gcc nlcomm.c qstats.c -o qdisc_stats```

This should generate a binary file named qdisc_stats in the current working directory.

### Execute
Execute the following command in the terminal in the path containing nlcomm.c:

```./qdisc_stats```

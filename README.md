# qdisc_stats
Uses netlink to fetch queueing discipline stats from kernel. A large part of the code has been inspired from iproute2 userspace utility.

TODO: Improve error handling and make the code modular.

### Compile
Execute the following command in the terminal in the path containing nlcomm.c
```gcc nlcomm.c -o qdisc_stats```

This should generate a binary file named qdisc_stats in the current working directory.

### Execute
Execute the following command in the terminal in the path containing nlcomm.c 
```./qdisc_stats```

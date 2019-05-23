# qdisc_stats
Uses netlink to fetch queueing discipline stats from kernel. A large part of the code has been inspired from iproute2 userspace utility.

TODO: Improve error handling and make the code modular.

# Compile
- gcc nlcomm.c -o qdisc_stats
# Execute
- ./qdisc_stats

## Homework 1 PCOM

#### This project was about implementing a router in C, which would handle Ethernet frames with IP packets

Firstly, we allocate a buffer where we are going to store the
data frame. Then we allocate memory for the routing table, which
is given by argument in the command line and arp table, which
we can access from the "arp_table.txt" file. We are going to
sort the rtable by ip and mask descending for a more effective
search in the table (binary instead of linear). We are searching
for incoming packets, and we process it depending on the ethernet
type : IP (0x0800) or ARP (0x0806) (unimplemented). The ip header
is extracted from the packet and the first thing that is checked
has to do with the checksum validation: the program calculates
the checksum of the received ip header and compares it with the
checksum in the header. If theY don't match, it means that the
packet is corrupted. The original checksum is stored in old_check
for later. The program then checks the ttl of the ip header. If the
ttl is 1 or less, it means the packet has expired and we send an icmp
time exceeded message and continue to the next packet. In case that
the packet hasn't expired, we update the ttl and the checksum. The
next step is to check whether the destination address is the same
as the router ip. If they match (ip_hdr->daddr == router_ip), it
means the packet is destined for the router. In this case, the program
sends an icmp reply message. After all these steps, we send the packet
and wait for another one.
# router

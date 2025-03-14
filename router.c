#include "./include/utils.h"
#include "./include/list.h"
#include "./include/queue.h"
#include <arpa/inet.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct route_table_entry *rtable;
int rtable_len;

static struct arp_table_entry *arp_table;
static uint32_t arp_table_len;

int main(int argc, char *argv[])
{
	char buf[MAX_PACKET_LEN];

	// Do not modify this line
	init(argc - 2, argv + 2);

	// allocating memory for the routing table
	rtable = malloc(sizeof(*rtable) * 100000);
	DIE(rtable == NULL, "malloc");
	rtable_len = read_rtable(argv[1], rtable);

	// allocating memory for the ARP table
	arp_table = malloc(sizeof(*arp_table) * 100000);
	DIE(arp_table == NULL, "malloc");
	arp_table_len = parse_arp_table("arp_table.txt", arp_table);

	// sorting the routing table based on prefix and mask for a better efficiency when
	// searching
	qsort(rtable, rtable_len, sizeof(rtable[0]), compare);

	while (1) {

		int interface;
		size_t len;

		interface = recv_from_any_link(buf, &len);
		DIE(interface < 0, "recv_from_any_links");

		struct ether_header *eth_hdr = (struct ether_header *) buf;
		/* Note that packets received are in network order,
		any header field which has more than 1 byte will need to be conerted to
		host order. For example, ntohs(eth_hdr->ether_type). The oposite is needed when
		sending a packet on the link, */
		// checking whether the packet is an IP packet or an ARP packet
		if(ntohs(eth_hdr->ether_type) == 0x800) {
			struct iphdr *ip_hdr = (struct iphdr *)(buf + sizeof(struct ether_header));

			// checksum check
			uint16_t old_check = ip_hdr->check;
			ip_hdr->check = 0;
			printf("Received packet from %d\n", interface);
			if(old_check != htons(checksum((uint16_t *)ip_hdr, sizeof(*ip_hdr)))) {
				// if the checksum is not correct the packet is dropped
				continue;
			}

			// TTL check.
			if (ip_hdr->ttl <= 1) {
				printf("TTL expired\n");
				// if the ttl is 0 or 1 an icmp packet is sent back to the sender
				icmp(eth_hdr, ip_hdr, interface, 11, buf);
				continue;
			} else {
				// if the packet didnt expire we update the ttl and the checksum with
				// the new ttl
				uint16_t old_ttl = ip_hdr->ttl; 
				ip_hdr->ttl--;
				ip_hdr->check = 0;
				ip_hdr->check = ~(~old_check + ~(old_ttl) + (uint16_t)ip_hdr->ttl) - 1;
			}

			struct route_table_entry *best_route = get_best_route(ip_hdr->daddr, rtable, rtable_len);
			if(best_route == NULL) {
				printf("No route found\n");
				// if no route is found an icmp packet is sent back to the sender
				icmp(eth_hdr, ip_hdr, interface, 3, buf);
				continue;
			}

			// if the packet is for the router an icmp packet is sent back to it
			uint32_t router_ip;
			inet_pton(AF_INET, get_interface_ip(interface), &router_ip);
			if(ip_hdr->daddr == router_ip) {
				icmp(eth_hdr, ip_hdr, interface, 0, buf);
				continue;
			}

			struct arp_table_entry *arp_entry = get_arp_entry(best_route->next_hop, arp_table, arp_table_len);
			if(arp_entry == NULL) {
				printf("No ARP entry found\n");
				// if no arp entry is found for the next hop we go to the next packet
				continue;
			}
			
			// we update the mac addresses in the ethernet header and send the packet
			memcpy(eth_hdr->ether_dhost, arp_entry->mac, sizeof(arp_entry->mac));
			get_interface_mac(best_route->interface, eth_hdr->ether_shost);

			send_to_link(best_route->interface, buf, len);
		} else if(ntohs(eth_hdr->ether_type) == 0x806) {
			struct arp_header *arp_hdr = (struct arp_header *)(buf + sizeof(struct ether_header));
			continue;
		}
	}
	free(rtable);
	free(arp_table);
	return 0;
}
#include "../include/utils.h"
#include "../include/lib.h"
#include <arpa/inet.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/**
 * Finds the best matching route in the routing table for a given destination IP.
 *
 * @param ip_dest    Destination IP address in network byte order.
 * @param rtable     Pointer to the routing table.
 * @param rtable_len Length of the routing table.
 * @return Pointer to the best matching route_table_entry.
 */
struct route_table_entry *get_best_route(uint32_t ip_dest, struct route_table_entry *rtable, int rtable_len) {

	int left = 0;
	int right = rtable_len - 1;
	int mid;
	struct route_table_entry *curr_rtable = NULL;
	while (left <= right) {
		mid = left + (right - left) / 2;
		if ((rtable[mid].mask & ip_dest) == rtable[mid].prefix) {
			if(curr_rtable == NULL) {
				curr_rtable = &rtable[mid];
			} else {
				if(ntohl(curr_rtable->mask) < ntohl(rtable[mid].mask)) {
					curr_rtable = &rtable[mid];
				}
			}
		}

		if (ntohl(rtable[mid].prefix) < ntohl(ip_dest)) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}
	return curr_rtable;
}

/**
 * Finds the ARP entry for a given IP address.
 *
 * @param given_ip     IP address in network byte order.
 * @param arp_table    Pointer to the ARP table.
 * @param arp_table_len Length of the ARP table.
 * @return Pointer to the matching arp_table_entry if found, otherwise NULL.
 */
struct arp_table_entry *get_arp_entry(uint32_t given_ip, struct arp_table_entry *arp_table, uint32_t arp_table_len) {

	for (int i = 0; i < arp_table_len; i++) {
		if (arp_table[i].ip == given_ip) {
			return &arp_table[i];
		}
	}
	return NULL;
}

/**
 * Comparison function for qsort to sort the routing table based on prefix and mask.
 *
 * @param rtable1 Pointer to a route_table_entry.
 * @param rtable2 Pointer to another route_table_entry.
 * @return 1 if rtable1 is greater or -1 if rtable2 is equal or greater.
 */
int compare(const void *rtable1, const void *rtable2) {

	struct route_table_entry *route1 = (struct route_table_entry *)rtable1;
	struct route_table_entry *route2 = (struct route_table_entry *)rtable2;

	if(ntohl(route1->prefix) > ntohl(route2->prefix))
		return 1;
	else if(ntohl(route1->prefix) == ntohl(route2->prefix))
			if(ntohl(route1->mask) > ntohl(route2->mask))
				return 1;
	
	return -1;
}

/**
 * Creates and sends an ICMP packet.
 *
 * @param eth_hdr   Pointer to the Ethernet header.
 * @param ip_hdr    Pointer to the IP header.
 * @param interface Interface index.
 * @param type      ICMP message type.
 * @param buf       Buffer containing the packet.
 */
void icmp(struct ether_header *eth_hdr, struct iphdr *ip_hdr, int interface, uint8_t type, char *buf)
{
	struct icmphdr *icmp_hdr = (struct icmphdr *)(buf + sizeof(struct ether_header) + sizeof(struct iphdr));
	memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, sizeof(eth_hdr->ether_shost));
	icmp_hdr->type = type;
	icmp_hdr->code = 0;
	icmp_hdr->checksum = 0;
	icmp_hdr->checksum = htons(checksum((uint16_t *)icmp_hdr, sizeof(*icmp_hdr)));
	ip_hdr->tos = 0;
	ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr) + TTL);
	ip_hdr->id = 0;
	ip_hdr->frag_off = 0;
	ip_hdr->ttl = TTL;
	ip_hdr->protocol = IPPROTO_ICMP;
	ip_hdr->saddr = htonl(ip_hdr->saddr);
	ip_hdr->daddr = htonl(ip_hdr->daddr);
	
	memcpy((char *)icmp_hdr + sizeof(struct icmphdr), (uint8_t *)buf, TTL);
	memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, sizeof(eth_hdr->ether_shost));
	get_interface_mac(interface, eth_hdr->ether_shost);

	send_to_link(interface, (char *)eth_hdr, sizeof(*eth_hdr) + sizeof(*ip_hdr) + sizeof(*icmp_hdr) + TTL);
}

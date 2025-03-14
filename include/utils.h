#include "lib.h"
#include "protocols.h"
#include <arpa/inet.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct route_table_entry *get_best_route(uint32_t ip_dest, struct route_table_entry *rtable, int rtable_len);

struct arp_table_entry *get_arp_entry(uint32_t given_ip, struct arp_table_entry *arp_table, uint32_t arp_table_len);

int compare(const void *rtable1, const void *rtable2);

void icmp(struct ether_header *eth_hdr, struct iphdr *ip_hdr, int interface, uint8_t type, char *buf);
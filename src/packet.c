/*
 * packet.c
 *
 *  Created on: Mar 16, 2012
 *      Author: chenxm
 *		Email: chen_xm@sjtu.edu.cn
 */
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>

#include "packet.h"
#include "util.h"

/*
 * Create new packet
 */
packet_t *PacketNew(void)
{
	packet_t *pkt;
	pkt = MALLOC(packet_t, 1);
	memset(pkt, 0, sizeof(packet_t));
	pkt->tcp_odata = NULL;
	pkt->tcp_sdata = pkt->tcp_odata;
	return pkt;
}

/*
 * Free a memory block allocated to a packet_t object
 */
void PacketFree(packet_t *pkt){
	if(pkt != NULL){
		if(pkt->tcp_odata != NULL)
			free(pkt->tcp_odata);
		free(pkt);
	}
}

/*
 * Parse the ethernet header with little endian format
 */
ethhdr *PacketParseEthhdr(const char *p){
	ethhdr *hdr, *tmp;
	tmp = p;
	hdr = MALLOC(ethhdr, 1);
	memset(hdr, 0, sizeof(ethhdr));
	memcpy(hdr->ether_dhost, tmp->ether_dhost, 6 * sizeof(u_int8_t));
	memcpy(hdr->ether_shost, tmp->ether_shost, 6 * sizeof(u_int8_t));
	hdr->ether_type = ntohs(tmp->ether_type);
	return hdr;
}

/*
 * Parse the IP header with little endian format
 */
iphdr *PacketParseIPhdr(const char *p)
{
	iphdr *hdr, *tmp;
	tmp = (iphdr *)p;
	hdr = MALLOC(iphdr, 1);
	memset(hdr, '\0', sizeof(iphdr));
	hdr->ihl = tmp->ihl;
	hdr->version = tmp->version;
	hdr->tos = tmp->tos;
	hdr->tot_len = ntohs(tmp->tot_len);
	hdr->id = ntohs(tmp->id);
	hdr->frag_off = ntohs(tmp->frag_off);
	hdr->ttl = tmp->ttl;
	hdr->protocol = tmp->protocol;
	hdr->check = ntohs(tmp->check);
	hdr->saddr = ntohl(tmp->saddr);
	hdr->daddr = ntohl(tmp->daddr);
	return hdr;
}

/*
 * Parse the TCP header with little endian format
 */
tcphdr *PacketParseTCPhdr(const char *p)
{
	tcphdr *hdr, *tmp;
	tmp = (tcphdr *)p;
	hdr = MALLOC(tcphdr, 1);
	memset(hdr, '\0', sizeof(tcphdr));
	hdr->th_sport = ntohs(tmp->th_sport);
	hdr->th_dport = ntohs(tmp->th_dport);
	hdr->th_seq = ntohl(tmp->th_seq);
	hdr->th_ack = ntohl(tmp->th_ack);
	hdr->th_x2 = tmp->th_x2;
	hdr->th_off = tmp->th_off;
	hdr->th_flags = tmp->th_flags;
	hdr->th_win = ntohs(tmp->th_win);
	hdr->th_sum = ntohs(tmp->th_sum);
	hdr->th_urp = ntohs(tmp->th_urp);
	return hdr;
}

void PacketEthhdrFree(ethhdr *h){
	free(h);
}

void PacketIPhdrFree(iphdr *h){
	free(h);
}

void PacketTCPhdrFree(tcphdr *h){
	free(h);
}
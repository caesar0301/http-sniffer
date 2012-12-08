/*
 * capture.c
 *
 *  Created on: Mar 16, 2012
 *      Author: chenxm
 *		 Email: chen_xm@sjtu.edu.cn
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
#include <pcap.h>

#include "capture.h"
#include "packet.h"
#include "util.h"
#include "http.h"
#include "queue.h"
#include "flow.h"
	 
static int check_tcp_ports(tcphdr *th);
static packet_t* packet_preprocess(const char *raw_data, const struct pcap_pkthdr *pkthdr);

Queue __GLOBAL_PACKET_QUEUE;

int CaptureProcessRawPacket(const char *raw_data, const struct pcap_pkthdr *pkthdr){
	packet_t *new_packet = packet_preprocess(raw_data, pkthdr);
	if(new_packet == NULL)
		return (-1);
	QueuePush(&__GLOBAL_PACKET_QUEUE, new_packet);
	return 0;
}

int CaptureProcessPacketQueue(void){
	packet_t *new_packet = (packet_t *)QueueFront(&__GLOBAL_PACKET_QUEUE);
	if(new_packet != NULL){
		FHTAddPacket(new_packet);	// Add packet to FHT
		QueuePop(&__GLOBAL_PACKET_QUEUE, 0);
	}
	return 0;
}

static int check_tcp_ports(tcphdr *th){
	// Only standard HTTP, HTTPS and common proxy ports are listened
	int parr[] = {80, 8080, 3128, 443};
	int i;
	for(i=0; i<4; i++){
		if(th->th_dport == parr[i] || th->th_sport == parr[i])
			return 0;
	}
	return -1;
}

	
/* Parse packets' header information and return a packet_t object */
static packet_t* packet_preprocess(const char *raw_data, const struct pcap_pkthdr *pkthdr)
{
	char 	*cp = raw_data;
	ethhdr	*eth_hdr = NULL;
	iphdr	*ip_hdr = NULL;
	tcphdr	*tcp_hdr = NULL;
	packet_t	*pkt = NULL;	/* new packet */

	/* Parse libpcap packet header */
	pkt = PacketNew();
	pkt->cap_sec = pkthdr->ts.tv_sec;
	pkt->cap_usec = pkthdr->ts.tv_usec;
	pkt->raw_len = pkthdr->caplen;

	/* Parse ethernet header */
	eth_hdr = PacketParseEthhdr(cp);
	if(eth_hdr->ether_type != 0x0800){	//Only IP packet is processed
		PacketEthhdrFree(eth_hdr);
		PacketFree(pkt);
		return NULL;
	}

	/* Parse IP header */
	cp = cp + sizeof(ethhdr);
	ip_hdr = PacketParseIPhdr(cp);
	pkt->saddr = ip_hdr->saddr;
	pkt->daddr = ip_hdr->daddr;
	pkt->ip_hl = (ip_hdr->ihl) << 2;	/* bytes */
	pkt->ip_tol = ip_hdr->tot_len;
	pkt->ip_pro = ip_hdr->protocol;
	if(pkt->ip_pro != 0x06){		//Only TCP packet is processed
		PacketEthhdrFree(eth_hdr);
		PacketIPhdrFree(ip_hdr);
		PacketFree(pkt);
		return NULL;
	}

	/* Parse TCP header */
	cp = cp + pkt->ip_hl;
	tcp_hdr = PacketParseTCPhdr(cp);
	pkt->sport = tcp_hdr->th_sport;
	pkt->dport = tcp_hdr->th_dport;
	pkt->tcp_seq = tcp_hdr->th_seq;
	pkt->tcp_ack = tcp_hdr->th_ack;
	pkt->tcp_flags = tcp_hdr->th_flags;
	pkt->tcp_win = tcp_hdr->th_win;
	pkt->tcp_hl = tcp_hdr->th_off << 2;		/* bytes */
	pkt->tcp_odl = pkt->ip_tol - pkt->ip_hl - pkt->tcp_hl;
	if ((pkt->tcp_flags & TH_ACK) == TH_ACK)
		pkt->tcp_acked = 1;
	else
		pkt->tcp_acked = 0;
 	if((pkt->tcp_flags & TH_SYN) == TH_SYN || (pkt->tcp_flags & TH_FIN) == TH_FIN)
 		pkt->tcp_nxt_seq = pkt->tcp_seq + pkt->tcp_odl +1;
 	else
 		pkt->tcp_nxt_seq = pkt->tcp_seq + pkt->tcp_odl;
	// Check HTTP msg
	pkt->http = PACKET_NOT_CARRY_HTTP;
	if( check_tcp_ports(tcp_hdr) != 0 ){
		PacketEthhdrFree(eth_hdr);
		PacketIPhdrFree(ip_hdr);
		PacketTCPhdrFree(tcp_hdr);
		PacketFree(pkt);
		return NULL;
	}

	// Process packets which carry HTTP/HTTPS traffic
	if(pkt->tcp_odl > 0){
		cp = cp + pkt->tcp_hl;
		PacketExtractHTTPMsg(&pkt, cp, pkt->tcp_odl);
	}
	//Do not store any data in self-defined packet_t* object
	pkt->tcp_odata = NULL;
	//Free temp values finally
	PacketEthhdrFree(eth_hdr);
	PacketIPhdrFree(ip_hdr);
	PacketTCPhdrFree(tcp_hdr);
	return pkt;
}
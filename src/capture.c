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
	 
static int check_tcp_ports(tcphdr *th);
static packet_t* packet_preprocess(const char *raw_data, const struct pcap_pkthdr *pkthdr);

int CaptureProcessPacket(const char *raw_data, const pcap_pkthdr *pkthdr, Queue *packet_queue){
	packet_t *new_packet = packet_preprocess(raw_data, pkthdr);
	if(new_packet == NULL)
		return (-1);
	queue_push(packet_queue, new_packet);
	return 0;
}

static int check_tcp_ports(tcphdr *th){
	// Only standard HTTP, HTTPS and common proxy ports are listened
	int parr = [80, 8080, 3128, 443]
	for(int i=0; i<4, i++){
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
	pkt->http = 0x00; 						/* default */
	if( check_tcp_ports(tcp_hdr) != 0 ){
		PacketEthhdrFree(eth_hdr);
		PacketIPhdrFree(ip_hdr);
		PacketTCPhdrFree(tcp_hdr);
		PacketFree(pkt);
		return NULL;
	}

	/* Process packets which carry HTTP traffic */
	if(pkt->tcp_odl != 0){
		char *http_head_end;
		cp = cp + pkt->tcp_hl;
		int res = HttpMessageType(cp, pkt->tcp_odl, &http_head_end);
		if( res == 0x00 ){
			/* If the packet is not HTTP, we erase the payload to save memory space */
			pkt->tcp_odata = NULL;
		}else{
			int hdl = 0;
			if(res == 0x01){
				pkt->http = res;
				hdl = http_head_end - cp + 1;
				pkt->tcp_sdl = hdl;
				//*******************************
				// Extract HTTP message
				pkt->http_request = HTTPReqNew();
				HTTPParseReq(pkt->http_request, cp, http_head_end);
				//*******************************
			}else if(res == 0x10){
				pkt->http = res;
				hdl = head_end - cp + 1;
				pkt->tcp_sdl = hdl;
				//*******************************
				// Extract HTTP message
				pkt->http_response = HTTPRspNew();
				HTTPParseReq(pkt->http_response, cp, http_head_end);
				//*******************************
			}else{
				printf("Invalid HTTP message type.\n");
				exit(-1);	
			}

			
			/* Allocate memory to store HTTP header. */
			int tcp_stored_len = pkt->tcp_sdl + 1;
			pkt->tcp_odata = MALLOC(char, tcp_stored_len);
			memset(pkt->tcp_odata, 0, tcp_stored_len);
			memcpy(pkt->tcp_odata, cp, tcp_stored_len);
		}
	}else
		pkt->tcp_odata = NULL;
	PacketEthhdrFree(eth_hdr);
	PacketIPhdrFree(ip_hdr);
	PacketTCPhdrFree(tcp_hdr);
	return pkt;
}
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

static int no_packet = FALSE;	/* for debugging */

BOOL
capture_finished(void)
{
	return no_packet;
}

/* Parse packets' header information and return a packet_t object */
packet_t*
packet_preprocess(const char *raw_data, const struct pcap_pkthdr *pkthdr)
{
	packet_t	*pkt = NULL;	/* new packet */
	char 	*cp = raw_data;
	ethhdr	*eth_hdr = NULL;
	iphdr	*ip_hdr = NULL;
	tcphdr	*tcp_hdr = NULL;

	/* Parse libpcap packet header */
	pkt = packet_new();
	pkt->cap_sec = pkthdr->ts.tv_sec;
	pkt->cap_usec = pkthdr->ts.tv_usec;
	pkt->raw_len = pkthdr->caplen;

	/* Parse ethernet header */
	eth_hdr = packet_parse_ethhdr(cp);
	/* Is IP...? */
	if(eth_hdr->ether_type != 0x0800){
		free_ethhdr(eth_hdr);
		packet_free(pkt);
		return NULL;
	}

	/* Parse IP header */
	cp = cp+sizeof(ethhdr);
	ip_hdr = packet_parse_iphdr(cp);
	pkt->saddr = ip_hdr->saddr;
	pkt->daddr = ip_hdr->daddr;
	pkt->ip_hl = (ip_hdr->ihl) << 2;	/* bytes */
	pkt->ip_tol = ip_hdr->tot_len;
	pkt->ip_pro = ip_hdr->protocol;

	/* Is TCP...? */
	if(pkt->ip_pro != 0x06){
		free_ethhdr(eth_hdr);
		free_iphdr(ip_hdr);
		packet_free(pkt);
		return NULL;
	}

	/* Parse TCP header */
	cp = cp + pkt->ip_hl;
	tcp_hdr = packet_parse_tcphdr(cp);
	pkt->sport = tcp_hdr->th_sport;
	pkt->dport = tcp_hdr->th_dport;
	pkt->tcp_seq = tcp_hdr->th_seq;
	pkt->tcp_ack = tcp_hdr->th_ack;
	pkt->tcp_flags = tcp_hdr->th_flags;
	pkt->tcp_win = tcp_hdr->th_win;
	pkt->tcp_hl = tcp_hdr->th_off << 2;		/* bytes */
	pkt->tcp_dl = pkt->ip_tol - pkt->ip_hl - pkt->tcp_hl;
	pkt->http = 0; /* default */

	/* Check the TCP ports to identify if the packet carries HTTP data
	   We only consider normal HTTP traffic without encryption */
	if( !(tcp_hdr->th_sport == 80 || tcp_hdr->th_dport == 80 || \
		tcp_hdr->th_sport == 8080 || tcp_hdr->th_dport == 8080 || \
		tcp_hdr->th_sport == 8000 || tcp_hdr->th_dport == 8000)){
		free_ethhdr(eth_hdr);
		free_iphdr(ip_hdr);
		free_tcphdr(tcp_hdr);
		packet_free(pkt);
		return NULL;
	}

	/* Process packets of flows which carry HTTP traffic */
	if(pkt->tcp_dl != 0)
	{
		cp = cp + pkt->tcp_hl;
		if( !IsHttpPacket(cp, pkt->tcp_dl) )
		{
			/* If the packet is not HTTP, we erase the payload. */
			pkt->tcp_odata = NULL;
			pkt->tcp_data = pkt->tcp_odata;
		}
		else
		{
			/* Yes, it's HTTP packet */
			char *head_end = NULL;
			int hdl = 0;
			head_end = IsRequest(cp, pkt->tcp_dl);
			if( head_end != NULL )
			{
				/* First packet of request. */
				hdl = head_end - cp + 1;
				pkt->http = HTTP_REQ;
				/* Fake TCP data length with only HTTP header. */
				pkt->tcp_dl = hdl;
			}

			head_end = IsResponse(cp, pkt->tcp_dl);
			if( head_end != NULL )
			{
				/* First packet of response. */
				hdl = head_end - cp + 1;
				pkt->http = HTTP_RSP;
				/* Fake TCP data length with only HTTP header. */
				pkt->tcp_dl = hdl;
			}
			/* Allocate memory to store HTTP header. */
			pkt->tcp_odata = MALLOC(char, pkt->tcp_dl + 1);
			pkt->tcp_data = pkt->tcp_odata;
			memset(pkt->tcp_odata, 0, pkt->tcp_dl + 1);
			memcpy(pkt->tcp_odata, cp, pkt->tcp_dl);
		}
	}
	else
	{
		pkt->tcp_odata = NULL;
		pkt->tcp_data = pkt->tcp_odata;
	}
	free_ethhdr(eth_hdr);
	free_iphdr(ip_hdr);
	free_tcphdr(tcp_hdr);
	return pkt;
}

/* Capture main function. */
int 
capture_main(const char* interface, void (*pkt_handler)(void*))
{
	char errbuf[PCAP_ERRBUF_SIZE];
	memset(errbuf, 0, PCAP_ERRBUF_SIZE);
	char *raw = NULL;
	pcap_t *cap = NULL;
	struct pcap_pkthdr pkthdr;
	packet_t *packet = NULL;
	
	printf("Online mode ...\n");
	cap = pcap_open_live(interface, 65535, 0, 1000, errbuf);
	if( cap == NULL){
		printf("%s\n",errbuf);
		exit(1);
	}
	while(1){
		raw = pcap_next(cap, &pkthdr);
		if( raw == NULL){
			continue;
		}
		packet = packet_preprocess(raw, &pkthdr);
		if( NULL == packet ){
			continue;
		}
		pkt_handler(packet);
	}

	if( cap != NULL){
		pcap_close(cap);
	}
	return 0;
}

/* Read packets from pcap trace */
int 
capture_offline(const char* filename, void (*pkt_handler)(void*))
{
	char errbuf[PCAP_ERRBUF_SIZE];
	memset(errbuf, 0, PCAP_ERRBUF_SIZE);
	char *raw = NULL;
	pcap_t *cap = NULL;
	struct pcap_pkthdr pkthdr;
	packet_t *packet = NULL;
	extern int GP_CAP_FIN;
	
	// Open handler
	printf("Offline mode ...\n");
	cap = pcap_open_offline(filename, errbuf);
	if( cap == NULL){
		printf("%s\n",errbuf);
		exit(1);
	}

	// Run the loop
	while(1){
		raw = pcap_next(cap, &pkthdr);
		if( raw == NULL){
			// No more packets?
			GP_CAP_FIN = 1;
			break;
		}
		packet = packet_preprocess(raw, &pkthdr);
		if( NULL == packet ){
			continue;
		}
		// Handle the packet
		pkt_handler(packet);
	}

	// Close the handler
	if( cap != NULL){
		printf("Close handler!\n");
		pcap_close(cap);
	}
	return 0;
}

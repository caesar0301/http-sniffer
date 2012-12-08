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
#include "http.h"
#include "queue.h"

/*
 * Create new packet
 */
packet_t *PacketNew(void)
{
	packet_t *pkt;
	pkt = MALLOC(packet_t, 1);
	memset(pkt, 0, sizeof(packet_t));
	// pkt->tcp_odata = NULL;
	// pkt->http_request = NULL;
	// pkt->http_response = NULL;
	return pkt;
}

/*
 * Free a memory block allocated to a packet_t object
 */
void PacketFree(packet_t *pkt){
	if(pkt != NULL){
		if(pkt->tcp_odata != NULL)
			free(pkt->tcp_odata);
		if(pkt->http_request != NULL)
			HTTPReqFree(pkt->http_request);
		if(pkt->http_response != NULL)
			HTTPRspFree(pkt->http_response);
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

int PacketExtractHTTPMsg(const packet_t **p, const char *data, const int data_len){
	packet_t *pkt = *p;
	char *cp = data;
	char *http_head_end;
	if( data_len <= 0 ){
		pkt->http = PACKET_NOT_CARRY_HTTP;
		pkt->http_request = NULL;
		pkt->http_response = NULL;
		//pkt->tcp_sdl = 0;
		return 0;
	}
	// Parse HTTP message type
	int http_message_type = HttpMessageType(cp, data_len, &http_head_end);
	if(http_message_type == PACKET_NOT_CARRY_HTTP ){
		pkt->http_request = NULL;
		pkt->http_response = NULL;
	}else{
		int hdl = 0;
		if(http_message_type == PACKET_CARRY_HTTP_REQUEST){
			pkt->http = PACKET_CARRY_HTTP_REQUEST;
			hdl = http_head_end - cp + 1;
			//pkt->tcp_sdl = hdl;
			//*******************************
			// Extract HTTP message
			pkt->http_request = HTTPReqNew();
			pkt->http_response = NULL;
			HTTPParseReq(pkt->http_request, cp, http_head_end);
			//*******************************
		}else if(http_message_type == PACKET_CARRY_HTTP_RESPONSE){
			pkt->http = PACKET_CARRY_HTTP_RESPONSE;
			hdl = http_head_end - cp + 1;
			//pkt->tcp_sdl = hdl;
			//*******************************
			// Extract HTTP message
			pkt->http_request = NULL;
			pkt->http_response = HTTPRspNew();
			HTTPParseReq(pkt->http_response, cp, http_head_end);
			//*******************************
		}else{
			printf("Invalid HTTP message type.\n");
			exit(-1);	
		}			
	}
	/* Allocate memory to store HTTP header. */
	// int tcp_stored_len = pkt->tcp_sdl + 1;
	// pkt->tcp_odata = MALLOC(char, tcp_stored_len);
	// memset(pkt->tcp_odata, 0, tcp_stored_len);
	// memcpy(pkt->tcp_odata, cp, tcp_stored_len);
	return 0;
}

void PacketEthhdrFree(ethhdr *h){
	if( h != NULL)
		free(h);
}

void PacketIPhdrFree(iphdr *h){
	if( h != NULL)
		free(h);
}

void PacketTCPhdrFree(tcphdr *h){
	if( h != NULL)
		free(h);
}

void PacketQueueFree(Queue *que){
	packet_t *tmpkt;
	while(QueueSize(que) > 0){
		tmpkt = (packet_t *)QueueFront(que);
		if(tmpkt != NULL){
			PacketFree(tmpkt);
			QueuePop(que, 0);
		}
	}
}

int PacketUpdateHTTPPair(http_pair_t *pair, packet_t *packet, int src){
	if(packet->http_request != NULL && packet->http_response == NULL && src == 1){
		pair->request = packet->http_request;
		pair->req_fb_sec = packet->cap_sec;
		pair->req_fb_usec = packet->cap_usec;
		pair->req_total_len += packet->tcp_odl;
		pair->req_body_len += (packet->tcp_odl - packet->tcp_sdl);
	}else if(packet->http_request == NULL && packet->http_response != NULL && src == 0){
		pair->response = packet->http_response;	
		pair->rsp_fb_sec = packet->cap_sec;
		pair->rsp_fb_usec = packet->cap_usec;
		pair->rsp_total_len += packet->tcp_odl;
		pair->rsp_body_len += (packet->tcp_odl - packet->tcp_sdl);
	}else if(packet->http_request == NULL && packet->http_response == NULL){
		if(src == 1){
			pair->req_total_len += packet->tcp_odl;
			pair->req_body_len += packet->tcp_odl;
		}else if(src == 0){
			pair->rsp_total_len += packet->tcp_odl;
			pair->rsp_body_len += packet->tcp_odl;
		}else{
			printf("[ERROR] Wrong packet direction. Src should be 0 or 1\n");
			exit(-1);		
		}
	}
	else{
		printf("[ERROR] Packet can't contain request and response at the same time.\n");
		exit(-1);	
	}
	return 0;
}
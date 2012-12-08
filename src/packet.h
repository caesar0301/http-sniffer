/*
 * packet.h
 *
 *  Created on: Mar 16, 2012
 *      Author: chenxm
 *		Email: chen_xm@sjtu.edu.cn
 */

#ifndef __PACKET_H__
#define __PACKET_H__

#include <time.h>
#include <stdio.h>
#include <sys/types.h>

#include "tcp.h"
#include "http.h"
#include "queue.h"

// Global queue to store captured packets
extern Queue __GLOBAL_PACKET_QUEUE;

/* Self-defined packet structure of this program */
typedef struct _packet_t packet_t;
struct _packet_t 
{
	time_t cap_sec;         /* Capture time sec */
	time_t cap_usec;        /* Capture time usec */
	u_int32_t	raw_len;
	u_int32_t 	saddr;		/* 0x: source IP address */
	u_int32_t 	daddr;		/* 0x: destination IP address */
	u_int16_t 	sport;		/* TCP source port */
	u_int16_t 	dport;		/* TCP destination port */
	u_int8_t 	ip_hl;		/* Bytes: IP header length */
	u_int16_t 	ip_tol;		/* Bytes: IP total length */
	u_int8_t	ip_pro;		/* Protocol */
	u_int16_t	ip_pid;		/* (Unused), needed if packet is fragmented */
	u_int32_t 	tcp_seq;	/* TCP sequence number */
	u_int32_t 	tcp_ack;	/* TCP acknowledge number */
	u_int32_t	tcp_nxt_seq;/* Next seq number induced by TCP flags*/
	u_int8_t	tcp_acked;	/* Flag whether or not acked (1 or 0) */
	u_int8_t 	tcp_flags;	/* TCP flags */
	u_int16_t 	tcp_win;	/* TCP window size */
	u_int8_t 	tcp_hl;		/* Bytes: TCP header length */
	u_int16_t	tcp_odl;	/* Bytes: original TCP payload length */
	u_int16_t	tcp_sdl;	/* (deprecated) Bytes: stored TCP payload length, usually equal to the HTTP header length */
	char		*tcp_odata;	/* (deprecated) Orignal TCP payload */
#define PACKET_NOT_CARRY_HTTP	0x00
#define PACKET_CARRY_HTTP_REQUEST	0x01
#define PACKET_CARRY_HTTP_RESPONSE	0x10
	u_int8_t	http;		/* 0x00: not http, 0x01: is_request, 0x10: is_response */
	request_t*	http_request;
	response_t*	http_response;
};

/* Basic functions of packets */
/* Create a new packet object and return it.
** The new packet is initialized with members set zero or NULL */
packet_t *PacketNew(void);

/* Parse the Ethernet part of packet header.
** A new Ethernet header object is created in new memory area
** and filled with header fileds */
ethhdr *PacketParseEthhdr(const char *p);

/* Parse the IP part of packet header.
** A new IP header object is created in new memory area
** and filled with header fileds */
iphdr *PacketParseIPhdr(const char *p);

/* Parse the TCP part of packet header.
** A new TCP header object is created in new memory area
** and filled with header fileds */
tcphdr *PacketParseTCPhdr(const char *p);

/* Parse the HTTP header of the packet if it exists.
** A new HTTP request or response object is created and stored 
**in the packet_t */
int PacketExtractHTTPMsg(const packet_t **p, const char *data, const int data_len);

/* Free a Ethernet header object.
** All members (including objects indicated by pointers) are freed. */
void PacketEthhdrFree(ethhdr *h);

/* Free a IP header object.
** All members (including objects indicated by pointers) are freed. */
void PacketIPhdrFree(iphdr *h);	

/* Free a TCP header object.
** All members (including objects indicated by pointers) are freed. */
void PacketTCPhdrFree(tcphdr *h);

/* Free a Packet object.
** All members (including objects indicated by pointers) are freed. */
void PacketFree(packet_t *pkt);	

/* Add packet information to HTTP pair.
** The packet and HTTP pair belong to the same flow carrying HTTP messages.
** This function will choose to update the HTTP pair as request or response
** based on the packet_t * and int parameters. */
int PacketUpdateHTTPPair(http_pair_t *pair, packet_t *pkt, int src);

/* Free a queue whose data is Packet type.
** We add this function to replace the QueueClear to make sure the packet
** objects are free rightly. */
void PacketQueueFree(Queue *que);


#endif /* __PACKET_H__ */



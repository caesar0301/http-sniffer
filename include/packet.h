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
#include <pthread.h>
#include <sys/types.h>
#include <pcap/pcap.h>

#include "util.h"

/* Ethernet header structure */
typedef struct ethernet_header ethhdr;
struct ethernet_header
{
  u_int8_t  ether_dhost[6];		/* Destination addr	*/
  u_int8_t  ether_shost[6];		/* Source addr */
  u_int16_t ether_type;			/* Packet type */
};

/* IP header structure */
typedef struct ip_header iphdr;
struct ip_header
{
    u_int8_t ihl:4;
    u_int8_t version:4;
    u_int8_t tos;
    u_int16_t tot_len;
    u_int16_t id;
    u_int16_t frag_off;
#define	IP_RF 0x8000			/* Reserved fragment flag */
#define	IP_DF 0x4000			/* Dont fragment flag */
#define	IP_MF 0x2000			/* More fragments flag */
#define	IP_OFFMASK 0x1fff		/* Mask for fragmenting bits */
    u_int8_t ttl;
    u_int8_t protocol;
    u_int16_t check;
    u_int32_t saddr;
    u_int32_t daddr;
    /*The options start here. */
};

/* TCP header structure */
typedef struct tcp_header tcphdr;
struct tcp_header
{
    u_int16_t th_sport;		/* Source port */
    u_int16_t th_dport;		/* Destination port */
    u_int32_t th_seq;		/* Sequence number */
    u_int32_t th_ack;		/* Acknowledgement number */
    u_int8_t th_x2:4;		/* (Unused) */
    u_int8_t th_off:4;		/* Data offset */
    u_int8_t th_flags;
#  define TH_FIN	0x01
#  define TH_SYN	0x02
#  define TH_RST	0x04
#  define TH_PUSH	0x08
#  define TH_ACK	0x10
#  define TH_URG	0x20
    u_int16_t th_win;		/* Window */
    u_int16_t th_sum;		/* Checksum */
    u_int16_t th_urp;		/* Urgent pointer */
};

/* Basic packet structure of this program */
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
	u_int8_t 	tcp_flags;	/* TCP flags */
	u_int16_t 	tcp_win;	/* TCP window size */
	u_int8_t 	tcp_hl;		/* Bytes: TCP header length */
	u_int16_t	tcp_dl;		/* Bytes: TCP payload length */
#define HTTP_REQ	0x01
#define HTTP_RSP	0x10
	u_int8_t	http;		/* is_http or is_request or is_response */
	char		*tcp_odata;	/* Orignal TCP payload */
	char		*tcp_data;	/* Real useful data */
	packet_t	*next;		/* Next packet in packet queue */
};

/* Basic functions of packets */
packet_t *
packet_new(void);						/* Produce a new packet object */
void
packet_free(packet_t *pkt);				/* Free a packet object */
ethhdr *
packet_parse_ethhdr(const char *p);		/* Parse the Ethernet part of packet header */
iphdr *
packet_parse_iphdr(const char *p);		/* Parse the IP part of packet header */
tcphdr *
packet_parse_tcphdr(const char *p);		/* Parse the TCP part of packet header */
void 
free_ethhdr(ethhdr *h);					/* Free a Ethernet header object */
void 
free_iphdr(iphdr *h);					/* Free a IP header object */
void
free_tcphdr(tcphdr *h);					/* Free a TCP header object */

/* Packet queue functions */
/* Located in packet/queue.c */
BOOL
packet_queue_init(void);			/* Initialize */		
BOOL
packet_queue_enq(packet_t *pkt);	/* Enqueue */
packet_t *
packet_queue_deq(void);				/* Dequeue */
BOOL
packet_queue_clr(void);				/* Clear queue */
unsigned int
packet_queue_len(void);				/* Get the length of queue */
void 
packet_queue_print(void);			/* Print queue details for debugging */


#endif /* __PACKET_H__ */

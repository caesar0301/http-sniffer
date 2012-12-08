/*
 * tcp.h
 *
 *  Created on: Jun 15, 2011
 *      Author: chenxm
 */

#ifndef __TCP_H__
#define __TCP_H__
	 
#include <time.h>
#include <sys/types.h>

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

// // Self-defined TCP sequence number structure
// typedef struct _seq_t seq_t;
// struct _seq_t{
// 	time_t		cap_sec;
// 	time_t		cap_usec;
// 	packet_t 	*pkt;		/* packet */
// 	u_int32_t	seq;		/* seq order */
// 	u_int32_t	nxt_seq;	/* next in seq order */
// 	u_int8_t	ack;		/* acknowledged */
// 	u_int8_t	th_flags;	/* TCP flags of packet */
// 	seq_t		*next;		/* next in seq order */
// };
// 
// /* data to order a TCP stream */
// typedef struct _order_t order_t;
// struct _order_t {
// 	seq_t		*src;		/* source packet list ordered */
// 	seq_t		*dst;		/* destination packet list ordered */
// 	seq_t		*last_src;	/* last in src queue inserted */
// 	seq_t		*last_dst;	/* last in dst queue inserted */
// 	u_int32_t	num;		/* number of packet in queue */
// 	u_int8_t	rst;		/* reset */
// };
// 
// seq_t *TCPSeqNew(void);
// seq_t *TCPSeqFromPkt(packet_t *p);
// void TCPSeqFree(seq_t *seq);
// 
// order_t *TCPOrderNew(void);
// void TCPOrderFree(order_t *order);
// int TCPOrderAddSeq(order_t *ord, seq_t *new_seq, u_int8_t src);
// int TCPOrderCheck(order_t *order);		// for debugging


#endif /* __TCP_H__ */

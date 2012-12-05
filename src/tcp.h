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
	 
#include "packet.h"
	 
typedef struct _seq_t seq_t;
struct _seq_t{
	packet_t 	*pkt;		/* packet */
	u_int32_t	seq;		/* seq order */
	u_int32_t	nxt_seq;	/* next in seq order */
	u_int8_t	ack;		/* acknowledged */
	u_int8_t	th_flags;	/* TCP flags of packet */
	time_t		cap_sec;
	time_t		cap_usec;
	seq_t		*next;		/* next in seq order */
};

/* data to order a TCP stream */
typedef struct _order_t order_t;
struct _order_t {
	seq_t		*src;		/* source packet list ordered */
	seq_t		*dst;		/* destination packet list ordered */
	seq_t		*last_src;	/* last in src queue inserted */
	seq_t		*last_dst;	/* last in dst queue inserted */
	u_int32_t	num;		/* number of packet in queue */
	u_int8_t	rst;		/* reset */
};

seq_t *TCPSeqNew(void);
seq_t *TCPSeqFromPkt(packet_t *p);
void TCPSeqFree(seq_t *seq);

order_t *TCPOrderNew(void);
void TCPOrderFree(order_t *order);
int TCPOrderAddSeq(order_t *ord, seq_t *new_seq, u_int8_t src);
int TCPOrderCheck(order_t *order);		// for debugging

#endif /* __TCP_H__ */

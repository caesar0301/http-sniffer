#ifndef __ORDER_H__
#define __ORDER_H__

#include <sys/types.h>

#include "packet.h"
#include "util.h"

typedef struct _seq_t seq_t;
struct _seq_t{
	packet_t 	*pkt;		/* packet */
	u_int32_t	seq;		/* seq order */
	u_int32_t	nxt_seq;	/* next in seq order */
	BOOL		ack;		/* acknowledged */
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
	BOOL		rst;		/* reset */
};

seq_t *seq_new(void);
void	seq_free(seq_t *seq);
order_t *order_new(void);
void	order_free(order_t *order);
seq_t *seq_pkt(packet_t *p);

#endif	/* __ORDER_H__ */

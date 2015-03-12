/*
 * flow.h
 *
 *  Created on: Jun 10, 2011
 *      Author: chenxm
 */
#ifndef FLOW_H_
#define FLOW_H_

#include <pthread.h>
#include <sys/types.h>

#include "packet.h"
#include "order.h"
#include "http.h"
#include "util.h"

/**
 * flow socket
 */
typedef struct _flow_s	flow_s;
struct _flow_s
{
	u_int32_t	saddr;
	u_int32_t	daddr;
	u_int16_t	sport;
	u_int16_t	dport;
};

/**
 * Hash management block
 */
typedef struct _hash_mb_t hash_mb_t;

/**
 * Structure flow_t to store flow's info
 */
typedef struct _flow_t flow_t;

struct _hash_mb_t
{
	flow_t	*first;
	flow_t	*last;
	pthread_mutex_t mutex;
	int		elm_cnt;
};

struct _flow_t
{
	flow_t		*next;
	flow_t		*prev;
/* packets of flow */
	packet_t	*pkt_src_f;		/* front of packets src queue */
	packet_t	*pkt_src_e;		/* end of packets src queue */
	packet_t	*pkt_dst_f;		/* front of packets dst queue */
	packet_t	*pkt_dst_e;		/* end of packets dst queue */
	u_int32_t	pkt_src_n;		/* number of packet in src queue */
	u_int32_t	pkt_dst_n;		/* number of packet in dst queue */
	u_int32_t	pkts_src;	/* total packets count of the flow sent by src */
	u_int32_t	pkts_dst;	/* total packets count of the flow sent by dst */
	order_t		*order;		/* info to order the packet */
	hash_mb_t	*hmb;		/* hash management block */
/* TCP info */
	flow_s		socket;		/* flow socket with 4 tuple */
	u_int8_t	rtt;		/* round trip time (us) */
	time_t		syn_sec;	/* syn time in sec */
	time_t		syn_usec;	/* syn time in usec */
	time_t		ack2_sec;	/* third handshake time in sec */
	time_t		ack2_usec;	/* third handshake time in usec */
	time_t		fb_sec;		/* first byte time of flow payload in sec */
	time_t		fb_usec;	/* first byte time of flow payload in usec */
	time_t		lb_sec;		/* last byte time of flow payload in sec */
	time_t		lb_usec;	/* last byte time of flow payload in usec */
	u_int32_t	payload_src;	/* bytes of payload sent from source to destination */
	u_int32_t 	payload_dst;	/* bytes of payload sent from destination to source */
/* Control */
	time_t		last_action_sec;	/* latest modified time to the flow in seconds */
	time_t		last_action_usec;	/* latest modified time to the flow in microseconds */
	u_int8_t	close;
#define CLIENT_CLOSE	0x01
#define SERVER_CLOSE	0x02
#define FORCED_CLOSE	0x04
/* HTTP info*/
	BOOL		http;		/* carrying www ? */
	http_pair_t		*http_f;	/* front of HTTP pair queue */
	http_pair_t		*http_e;	/* end of HTTP pair queue */
	u_int32_t	http_cnt;	/* count of HTTP pairs */
};

/**
 * Basic flow functions
 * flow.c
 */
int flow_init(void);	/* Initiate both flow hash table and flow queue */
flow_t* flow_new(void);		/* Create a flow_t object */
int flow_free(flow_t*);	/* Free a flow_t object */
int flow_add_packet(flow_t *f, packet_t *packet, BOOL src);	/* Add a packet_t object to flow's packet_t chain */
int flow_socket_cmp(flow_s *a, flow_s *b);	/* Compare flows' sockets */
int flow_extract_http(flow_t *f);			/* Extract http_pair_t objects from flow's packet_t chain */
int flow_add_http(flow_t *f, http_pair_t *h);	/* Add a http_pair_t objects to flow's http_pair_t chain */	
void flow_print(const flow_t *flow);		/* Print flow's details for debugging */

/**
 * Functions of flow hash table
 * hash_table.c
 */
int flow_hash_init(void);			/* Initiate the flow hash table with no flows */
flow_t* flow_hash_new(flow_s s);		/* Create a new record in hash table according to flow's socket */
flow_t* flow_hash_delete(flow_t *f);	/* Delete a flow record in hash table */
flow_t* flow_hash_find(flow_s s);		/* Try to find a flow record in hash table based on its socket */
int flow_hash_add_packet(packet_t *packet);	/* Add a packet to one of the flow records in hash table */
int flow_hash_clear(void);	/* Clear all records of flow hash table */
int flow_hash_size(void);	/* Size of flow hash table */
int flow_hash_fcnt(void);	/* Number of flows online */
int flow_hash_scnt(void);	/* Count of hash slots consumed */
int flow_scrubber(const int timeslot);	/* Remove a flow record from hash table manually if the arriving period is greater than the timeslot */
void flow_hash_print(void);	/* Print hash table details for debugging */

/**
 * Functions of flow queue for further processing
 * queue.c
 */
int flow_queue_init(void);		/* Initialize a flow queue */
int flow_queue_enq(flow_t *flow);	/* Add a flow_t object to queue */
flow_t* flow_queue_deq(void);		/* Fetch a flow_t object from queue and remove it from queue */
int flow_queue_clear(void);		/* Clear all item in queue */
int flow_queue_len(void);		/* Length of queue */
void flow_queue_print(void);	/* Print queue details for debugging */

#endif /* FLOW_H_ */
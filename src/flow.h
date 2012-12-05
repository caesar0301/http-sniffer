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
#include "tcp.h"
#include "http.h"

/* flow socket */
typedef struct _flow_s	flow_s;
struct _flow_s
{
 	u_int32_t	saddr;
 	u_int32_t	daddr;
 	u_int16_t	sport;
 	u_int16_t	dport;
};

typedef struct _hash_mb_t hash_mb_t;
typedef struct _flow_t flow_t;
/* Flow hash table management block */
struct _hash_mb_t
{
 	flow_t	*first;
 	flow_t	*last;
 	pthread_mutex_t mutex;
 	int		elm_cnt;
};

/* Structure flow_t to store flow's info */
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
 	u_int8_t		http;		/* carrying www ? */
 	http_pair_t		*http_f;	/* front of HTTP pair queue */
 	http_pair_t		*http_e;	/* end of HTTP pair queue */
 	u_int32_t	http_cnt;	/* count of HTTP pairs */
};

/* Basic flow functions */
int FlowInt(void);			/* Initiate both flow hash table and flow queue */
flow_t* FlowNew(void);		/* Create a flow_t object */
int FlowFree(flow_t*);		/* Free a flow_t object */
int FlowPacketAdd(flow_t *f, packet_t *packet, u_int8_t src);	/* Add a packet_t object to flow's packet_t chain */
int FlowSocketCmp(flow_s *a, flow_s *b);	/* Compare flows' sockets */
int FlowHTTPExtract(flow_t *f);				/* Extract http_pair_t objects from flow's packet_t chain */
int FlowHTTPAdd(flow_t *f, http_pair_t *h);	/* Add a http_pair_t objects to flow's http_pair_t chain */	
void FlowDumpFile(const flow_t *flow, const char* file); 	/* Dump a flow_t object into a file */ 
void FlowPrint(const flow_t *flow);			/* Print flow's details for debugging */


/* Functions of flow hash table */
int FHTInit(void);				/* Initiate the flow hash table with no flows */
flow_t* FHTNew(flow_s s);		/* Create a new record in hash table according to flow's socket */
flow_t* FHTDelete(flow_t *f);	/* Delete a flow record in hash table */
flow_t* FHTFind(flow_s s);		/* Try to find a flow record in hash table based on its socket */
int FHTPacketAdd(packet_t *packet);		/* Add a packet to one of the flow records in hash table */
int FHTClear(void);				/* Clear all records of flow hash table */
int FHTFlowLive(void);			/* Number of flows online */
int FHTSize(void);				/* Size of flow hash table */
int FHTSizeUsed(void);			/* Count of hash slots consumed */
int FHTScrubber(const int timeslot);	/* Remove a flow record from hash table manually if the arriving period is greater than the timeslot */
void FHTPrint(void);			/* Print hash table details for debugging */

#endif /* FLOW_H_ */

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
#include "queue.h"
#include "tcp.h"
#include "http.h"

// Global queue to store finished flows
extern Queue __GLOBAL_FLOW_QUEUE;

// Self-defined quantuple flow socket structure
typedef struct _flow_s	flow_s;
struct _flow_s
{
 	u_int32_t	saddr;		// Source IP address
 	u_int32_t	daddr;		// Destination IP address
 	u_int16_t	sport;		// Source TCP port
 	u_int16_t	dport;		// Destination TCP port
};

// Self-defined flow structure: detailed flow info
typedef struct _flow_t flow_t;
// Self-defined Flow Hash Table Management UNIT (FHTMU)
typedef struct _hash_mu_t hash_mu_t;

//Assuming multiple flows may be hashed into the same FHT block,
//in each FHT block, flows are managed by the FHTMU and linked with bidirected links
//to each other in the order of creation.
struct _hash_mu_t{
	int		elm_cnt;		// Number of flows in each Flow Hash Table (FHT) block
 	flow_t	*first;			// First flow in the FHT block
 	flow_t	*last;			// Last flow in the FHT block
 	pthread_mutex_t mutex;	// Thread locker for each FHT block
};

struct _flow_t{
	// FHT management
	hash_mu_t	*hmu;			// The FHTMU associated with the flow
 	flow_t		*next;			// The next flow on the bidirected links
 	flow_t		*prev;			// The previous flow on the bidirected links
	// Packets management
	Queue		pkt_queue_src;	// (deprecated) The queue to store source packets
	Queue		pkt_queue_dst;	// (deprecated) The queue to store destination packets
 	u_int16_t	pkts_tot_src;	// Different to the pkt_queue_src.size which records the packet count stored, and the total source packet count means the packets including invlaid and retransmited ones and others like this
 	u_int16_t	pkts_tot_dst;	// Different to the pkt_queue_dst.size which records the packet count stored, and the total source packet count means the packets including invlaid and retransmited ones and others like this
 	u_int32_t	payload_tot_src;// Bytes of payload sent from source to destination
 	u_int32_t 	payload_tot_dst;// Bytes of payload sent from destination to source
 	// Basic flow features
 	flow_s		socket;			// Flow socket
 	u_int8_t	rtt;			// Round Trip Time (us)
 	time_t		syn_sec;		// First valid syn ::second part
 	time_t		syn_usec;		// First valid syn ::usecond part
 	time_t		ack2_sec;		// Third handshake time in sec
 	time_t		ack2_usec;		// Third handshake time in usec
 	time_t		fb_sec;			// First byte time of flow payload in sec
 	time_t		fb_usec;		// First byte time of flow payload in usec
 	time_t		lb_sec;			// Last byte time of flow payload in sec
 	time_t		lb_usec;		// Last byte time of flow payload in usec
	// HTTP message management
	Queue		http_pair_queue;// The queue to store HTTP pairs
#define FLOW_CARRY_HTTP	0x00
#define FLOW_NOT_CARRY_HTTP	0x01
	u_int8_t	carry_http;		// 0x00: no; 0x01: yes
	// State management by the program
#define NOT_CLOSED			0x00
#define CLIENT_CLOSE_FIN	0x01
#define SERVER_CLOSE_FIN	0x02
#define CLIENT_CLOSE_RST	0x03
#define SERVER_CLOSE_RST	0x04
#define FORCED_CLOSE		0x05
 	u_int8_t	close;			// Flow close status
 	time_t		last_action_sec;	// Latest modified time to the flow in seconds
 	time_t		last_action_usec;	// Latest modified time to the flow in microseconds
};

/* Basic flow functions */
flow_t* FlowNew(void);				// Create a emtpy flow_t object
int FlowFree(flow_t*);				// Free a flow_t object */
int FlowAddPacket(flow_t *f, packet_t *packet, u_int8_t src);	//Add a packet_t object to flow's packet_t chain
int FlowSocketCmp(flow_s *a, flow_s *b);	// Compare flows' sockets */
int FlowExtractHTTPPair(flow_t *f);			// Extract http_pair_t objects from flow's packet_t chain */
void FlowPrint(const flow_t *flow);			// Print flow's details for debugging */

/* Functions of FHT */
int FHTInit(void);					// Initiate the FHT with no flows 
int FHTAddPacket(packet_t *packet);	// Add a packet to one of the flow records in hash table 
int FHTItemCount(void);				// Count of hash slots consumed 
int FHTSize(void);					// Size of FHT 
int FHTLiveFlow(void);				// Number of flows online 
flow_t* FHTNewItem(flow_s s);		// Create a new record in hash table according to flow's socket 
flow_t* FHTDeleteFlow(flow_t *f);	// Delete a flow record in hash table 
flow_t* FHTFindFlow(flow_s s);		// Try to find a flow record in hash table based on its socket
int FHTClearFlowTimeout(const int timeslot);	// Remove a flow record from hash table manually if the arriving period is greater than the timeslot
int FHTClearAllFlow(void);			// Clear all records of FHT 

#endif /* FLOW_H_ */

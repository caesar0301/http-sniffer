/*
 * flow.c
 *
 *  Created on: Jun 10, 2011
 *      Author: chenxm
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <unistd.h>

#include "util.h"
#include "flow.h"
#include "packet.h"
#include "tcp.h"
#include "http.h"
#include "queue.h"

#define HASH_SIZE	10007
#define HASH_FLOW(flow_socket) ( \
( (flow_socket.sport & 0xff) | ((flow_socket.dport & 0xff) << 8) | \
  ((flow_socket.saddr & 0xff) << 16) | ((flow_socket.daddr & 0xff) << 24) \
) % HASH_SIZE)
static hash_mu_t *FLOW_HASH_TABLE[HASH_SIZE];
static int flow_cnt = 0;			// flows live in hash table

static void reset_flow(flow_t *f);
static int update_flow_statistics(flow_t *f, packet_t *packet, u_int8_t src);
// static int compare_sequence_time(seq_t *seq1, seq_t *seq2);

/* Create a new flow record and initiate its members */
flow_t* FlowNew(void)
{
	flow_t *f;
	f = MALLOC(flow_t, 1);
	memset(f, 0, sizeof(flow_t));
	//Special members
	f->hmu = f->next = f->prev = NULL;
	QueueInit(&(f->pkt_queue_src));
	QueueInit(&(f->pkt_queue_dst));
	return f;
}

// Free a flow_t object
int FlowFree(flow_t *f)
{
	if (f != NULL){
		//Special members
		if(f->hmu != NULL)
			f->hmu = NULL;
		if(f->next != NULL)
			f->next = NULL;
		if(f->prev != NULL)
			f->prev = NULL;
		PacketQueueFree(&(f->pkt_queue_src));
		PacketQueueFree(&(f->pkt_queue_dst));
		HTTPPairQueueFree(&(f->http_pair_queue));
		//General free
		free(f);
	}
	return 0;
}

/* Compare two flow_s objects */
int FlowSocketCmp(flow_s *a, flow_s *b){
	return memcmp(a, b, sizeof(flow_s));
}

// Add a packet to flow
int FlowAddPacket(flow_t *f, packet_t *packet, u_int8_t src){
	Queue __GLOBAL_FLOW_QUEUE;
	flow_t *tmp_flow;
	pthread_mutex_lock(&(f->hmu->mutex));
	// Delete flows that are not HTTP overlayed
	if( f->pkts_tot_src >= 5 && f->carry_http != FLOW_CARRY_HTTP){
		/* We make sure that the flow is not a HTTP flow,
		 * then remove it */
		PacketFree(packet);
		tmp_flow = FHTDeleteFlow(f);
		pthread_mutex_unlock(&(f->hmu->mutex));
		FlowFree(FHTDeleteFlow(tmp_flow));
		return 0;
	}
	/* TH_ACK: third handshake */
	if(src == 1 && f->pkts_tot_src == 1 && (packet->tcp_flags & TH_ACK) == TH_ACK){
		f->ack2_sec = packet->cap_sec;
		f->ack2_usec = packet->cap_usec;
		f->rtt = (f->ack2_sec - f->syn_sec) * 1000000 + (f->ack2_usec - f->syn_usec);
		update_flow_statistics(f, packet, src);
		PacketFree(packet);
		pthread_mutex_unlock(&(f->hmu->mutex));
		return 0;
	}
	/* TH_RST:
	 * If the flow is reset by sender or receiver*/
	if((packet->tcp_flags & TH_RST) == TH_RST){
		if( f->carry_http != FLOW_CARRY_HTTP ){
			// Invalid flow
			PacketFree(packet);
			tmp_flow = FHTDeleteFlow(f);
			pthread_mutex_unlock(&(f->hmu->mutex));
			FlowFree(FHTDeleteFlow(tmp_flow));
			return 0;
		}else{
			// Valid flow
			if(src == 1)
				f->close = CLIENT_CLOSE_RST;
			else
				f->close = SERVER_CLOSE_RST;
			update_flow_statistics(f, packet, src);
			PacketFree(packet);
			tmp_flow = FHTDeleteFlow(f);
			pthread_mutex_unlock(&(f->hmu->mutex));
			//Your code of processing each finished flow
			//............................
			printf("A HTTP-overlayed flow is reset by %s\n", src == 1 ? "client" : "server");
			FlowFree(tmp_flow);
			//............................
            return 0;
		}
	}
	/* TH_FIN:
	 * The flow will be closed if the both fins are detected */
	if( (packet->tcp_flags & TH_FIN) == TH_FIN){
		if( src == 1 )
			f->close = CLIENT_CLOSE_FIN;
		else
			f->close = SERVER_CLOSE_FIN;	
		update_flow_statistics(f, packet, src);
		PacketFree(packet);
		if(f->close == CLIENT_CLOSE_FIN  || f->close == SERVER_CLOSE_FIN){		/* && or || */
			tmp_flow = FHTDeleteFlow(f);
			pthread_mutex_unlock(&(f->hmu->mutex));
			//Your code of processing each finished flow
			//............................
			printf("A HTTP-overlayed flow is closed by %s\n", src == 1 ? "client" : "server");
			FlowFree(tmp_flow);
			//............................
		}
		return 0;
	}
	/* Other packets, processed and freed then */
	if(src == 1){
		// Request data packets
		if( f->pkts_tot_src == 0 && (packet->tcp_flags & TH_SYN) == TH_SYN){
			// First SYN
			f->syn_sec = packet->cap_sec;
			f->syn_usec = packet->cap_usec;
		}else{
			if( f->pkts_tot_src < 3 && (packet->tcp_flags & TH_SYN) == TH_SYN){
				// SYN repeated, reset flow as a fresh one
				reset_flow(f);
				f->syn_sec = packet->cap_sec;
				f->syn_usec = packet->cap_usec;
			}else{
				if(packet->http != PACKET_NOT_CARRY_HTTP ){
					if(packet->http_request == NULL){
						printf("[ERROR] Packet dose not carry HTTP request message in the client --> server direction.\n");
						exit(-1);
					}
					// HTTP content detected
					f->carry_http = FLOW_CARRY_HTTP;
					// Create a new HTTP pair and add it to the HTTP pair queue of the flow
					// The pair contains the HTTP information carried by the HTTP packet
					http_pair_t *new_pair = HTTPPairNew();
					// Upate the HTTP pair status with the packet info
					PacketUpdateHTTPPair(new_pair, packet, src);
					// Add the new pair to the queue belonging to the flow					
					QueuePush(&(f->http_pair_queue), new_pair);
				}else{
					// Request data chunk
					// Update the status of latest request in the HTTP pair queue
					http_pair_t *latest_pair = QueueTail(&(f->http_pair_queue));
					if (latest_pair != NULL)
						PacketUpdateHTTPPair(latest_pair, packet, src);
				}
			}
		}
	}else if(src == 0){
		// Response data packets
		if(packet->http != PACKET_NOT_CARRY_HTTP ){
			if(packet->http_response == NULL){
				printf("[ERROR] Packet dose not carry HTTP response message in the server --> client direction.\n");
				exit(-1);
			}
			// Update the status of EARLIEST response in the HTTP pair queue, i.e., pipelining in HTTP1.1
			// Get the front HTTP pair
			http_pair_t *tmp = (http_pair_t *)QueueFront(&(f->http_pair_queue));
			while(tmp != NULL){
				if(tmp->request != NULL && tmp->response != NULL){
					//Your code of processing each HTTP pair
					//............................
					//Add later: dump HTTP pair according with flow id
					printf("(HOST)%s (URI)%s\n", tmp->request->host, tmp->request->uri);
					//............................
					HTTPPairFree(tmp);
					QueuePop(&(f->http_pair_queue), 0);
					tmp = (http_pair_t *)QueueFront(&(f->http_pair_queue));
					continue;
				}else if(tmp->request != NULL && tmp->response == NULL){
					// Upate the HTTP pair status with the packet info
					PacketUpdateHTTPPair(tmp, packet, src);
					//Your code of processing each HTTP pair
					//............................
					//Add later: dump HTTP pair according with flow id
					printf("(HOST)%s (URI)%s\n", tmp->request->host, tmp->request->uri);
					//HTTPPairFree(tmp);
					QueuePop(&(f->http_pair_queue), 0);
					//............................
					break;
				}else{
					// Request is NULL, transmission error occurs, just omit the packet
					break;
				}
			}
		}else{
			// Response data chunk
			// Update the status of latest response in the HTTP pair queue
			http_pair_t *latest_pair = QueueFront(&(f->http_pair_queue));
			if (latest_pair != NULL)
				PacketUpdateHTTPPair(latest_pair, packet, src);
		}
	}else{
		printf("[ERROR] Wrong packet direction. Src should be 0 or 1\n");
		exit(-1);
	}
	
	// All the HTTP information is stored in the HTTP pair queue of the flow
	// Reset these pointers to free the packet securately in the future	
	packet->http_request = NULL;
	packet->http_response = NULL;
	// if(src == 1)
	// 	// Add the packet to the packet queue of the flow
	// 	QueuePush(&(f->pkt_queue_src), packet);
	// else
	// 	// Add the packet to the packet queue of the flow
	// 	QueuePush(&(f->pkt_queue_dst), packet);
	update_flow_statistics(f, packet, src);
	PacketFree(packet);
	pthread_mutex_unlock(&(f->hmu->mutex));
	return 0;
}

/* Extract http_pair_t objects from flow's packet_t chain */
int FlowExtractHTTPPair(flow_t *f){
	
}

/* Dump a flow_t object into a file */ 
void FlowDumpFile(const flow_t *flow, const char* file){

}

/* Print flow's details for debugging */
void FlowPrint(const flow_t *flow)
{
	/*Convert IP addr */
	char *saddr = malloc(sizeof("aaa.bbb.ccc.ddd"));
	char *daddr = malloc(sizeof("aaa.bbb.ccc.ddd"));
	strncpy(saddr, ip_ntos(flow->socket.saddr), sizeof("aaa.bbb.ccc.ddd"));
	strncpy(daddr, ip_ntos(flow->socket.daddr), sizeof("aaa.bbb.ccc.ddd"));
	/* Print flow information. */
	printf("%s:%d-->%s:%d\n", saddr, flow->socket.sport, daddr, flow->socket.dport);
	free(saddr);
	free(daddr);
}


/***********************
 * Static functions      *
 ***********************/
/* Reset a flow_t object to reuse */
static void reset_flow(flow_t *f){
	// The socket info must be maintained
	flow_s flow_socket;
	memcpy(&flow_socket, &(f->socket), sizeof(flow_s));
	hash_mu_t *hmu = f->hmu;
	flow_t	*next = f->next;
	flow_t	*prev = f->prev;
	QueueClear(&(f->pkt_queue_src), 1);
	QueueClear(&(f->pkt_queue_dst), 1);
	memset(f, 0, sizeof(flow_t));
	f->hmu = hmu;
	f->next = next;
	f->prev = prev;
}

static int update_flow_statistics(flow_t *f, packet_t *packet, u_int8_t src){
	u_int32_t *pkt_num;
	u_int32_t *byt_num;
	if(src == 1){
		pkt_num = &(f->pkts_tot_src);
		byt_num = &(f->payload_tot_src);
	}else{
		pkt_num = &(f->pkts_tot_dst);
		byt_num = &(f->payload_tot_dst);
	}
	// Update total packet count and byte count
	(*pkt_num)++;
	(*byt_num) += packet->tcp_odl;
	// Update last action time
	f->last_action_sec = packet->cap_sec;
	f->last_action_usec = packet->cap_usec;	
	return 0;
}

/* Compare seq_t objects if equel */
// static int compare_sequence_time(seq_t *seq1, seq_t *seq2){
// 	u_int32_t	sec1 = seq1->cap_sec;
// 	u_int32_t	usec1 = seq1->cap_usec;
// 	u_int32_t	sec2 = seq2->cap_sec;
// 	u_int32_t	usec2 = seq2->cap_usec;
// 	int ret = 0;
// 	if(sec1 > sec2 || (sec1 == sec2 && usec1 > usec2)){
// 		ret = 1;
// 	}else if (sec1 < sec2 || (sec1 == sec2 && usec1 < usec2)){
// 		ret = -1;
// 	}else{
// 		ret = 0;
// 	}
// 	return ret;
// }



/***********************
 * FHT functions      *
 ***********************/
	 
/* Initiate the flow hash table with no flows */
int FHTInit(void){
	int ret, i;
	flow_cnt = 0;
	for(i=0; i<HASH_SIZE; i++){
		FLOW_HASH_TABLE[i] = MALLOC(hash_mu_t, 1);
		FLOW_HASH_TABLE[i]->first = NULL;
		FLOW_HASH_TABLE[i]->last = NULL;
		FLOW_HASH_TABLE[i]->elm_cnt = 0;
		ret = pthread_mutex_init(&(FLOW_HASH_TABLE[i]->mutex), NULL);
		if (ret != 0)
			return -1;
	}
	return 0;
}

/* Create a new record in hash table according to flow's socket */
flow_t* FHTNewItem(flow_s s){
	flow_t *f = FlowNew();
	f->socket.saddr = s.saddr;
	f->socket.daddr = s.daddr;
	f->socket.sport = s.sport;
	f->socket.dport = s.dport;
	
	hash_mu_t *hm = FLOW_HASH_TABLE[HASH_FLOW(s)];
	pthread_mutex_lock(&(hm->mutex));
	f->hmu = hm;
	// Added to the tail
	f->next = NULL;
	f->prev = hm->last;
	if(hm->last == NULL){
		hm->first = f;
		hm->last = f;
	}else{
		hm->last->next = f;
		hm->last = f;
	}
	hm->elm_cnt++;
	flow_cnt++;
	pthread_mutex_unlock(&(hm->mutex));
	return f;
}

/* Delete a flow record in hash table */
flow_t* FHTDeleteFlow(flow_t *f){
	hash_mu_t *hm = f->hmu;
	if( f->prev == NULL && f->next == NULL){
		// The only flow
		hm->first = NULL;
		hm->last = NULL;
		hm->elm_cnt--;
	}else if(f->prev == NULL && f->next != NULL){
		// The first flow
		hm->first = f->next;
		hm->first->prev = NULL;
		hm->elm_cnt--;
	}else if(f->prev != NULL && f->next == NULL){
		// The tail flow
		hm->last = f->prev;
		hm->last->next = NULL;
		hm->elm_cnt--;
	}else{
		// Others
		f->prev->next = f->next;
		f->next->prev = f->prev;
		hm->elm_cnt--;
	}
	f->next = NULL;
	f->prev = NULL;
	flow_cnt--;
	return f;
}

/* Try to find a flow record in hash table based on its socket */
flow_t* FHTFindFlow(flow_s s){
	hash_mu_t *hm = NULL;
	flow_t	*flow = NULL;
	hm = FLOW_HASH_TABLE[HASH_FLOW(s)];
	pthread_mutex_lock(&(hm->mutex));
	if (hm->elm_cnt > 0){
		flow = hm->first;
		while(flow != NULL){
			if(FlowSocketCmp(&s, &flow->socket) == 0){
				pthread_mutex_unlock(&(hm->mutex));
				return flow;
			}else{
				flow = flow->next;
				continue;
			}
		}
	}
	pthread_mutex_unlock(&(hm->mutex));
	return NULL;
}

/*
 * Add a packet to hash table
 * Link the packet to flow's packet list if the flow has already existed;
 * Otherwise, make a new flow record and add the packet to it.
 */
int FHTAddPacket(packet_t *packet){
	flow_s cs;	// Temp socket
	cs.saddr = packet->daddr;
	cs.sport = packet->dport;
	cs.daddr = packet->saddr;
	cs.dport = packet->sport;

	flow_t *f = FHTFindFlow(cs);
	if(f != NULL){
		// src == 0
		FlowAddPacket(f, packet, 0);
	}else{
		// Source and destination switched
		cs.saddr = packet->saddr;
		cs.daddr = packet->daddr;
		cs.sport = packet->sport;
		cs.dport = packet->dport;

		f = FHTFindFlow(cs);
		if(f != NULL){
			// src == 1
			FlowAddPacket(f, packet, 1);
		}else{
			if((packet->tcp_flags & TH_SYN) == TH_SYN){
				f = FHTNewItem(cs);
				FlowAddPacket(f, packet, 1);
			}else{
				PacketFree(packet);
			}
		}
	}
	return 0;
}

/* Clear the flow hash table thoroughly */
int FHTClearAllFlow(void)
{
	int i = 0;
	for(i=0; i<HASH_SIZE; i++){
		pthread_mutex_lock(&(FLOW_HASH_TABLE[i]->mutex));
	}

	flow_t	*f = NULL;
	flow_cnt = 0;
	for(i=0; i<HASH_SIZE; i++){
		while(FLOW_HASH_TABLE[i]->first != NULL ){
			f = FLOW_HASH_TABLE[i]->first;
			FLOW_HASH_TABLE[i]->first = FLOW_HASH_TABLE[i]->first->next;
			FlowFree(f);
			FLOW_HASH_TABLE[i]->elm_cnt--;
		}
		FLOW_HASH_TABLE[i]->first = NULL;
		FLOW_HASH_TABLE[i]->last = NULL;
	}

	for(i=0; i<HASH_SIZE; i++){
		pthread_mutex_unlock(&(FLOW_HASH_TABLE[i]->mutex));
	}
	return 0;
}

/* Return the size of hash table */
int FHTSize(void)
{
	return HASH_SIZE;
}

/* Return the flow count of hash table */
int FHTLiveFlow(void)
{
	return flow_cnt;
}

/* Return the count of hash slots consumed */
int FHTItemCount(void)
{
	int i = 0;
	int slot_consumed = 0;

	for(i=0; i<HASH_SIZE; i++)
	{
		pthread_mutex_lock(&(FLOW_HASH_TABLE[i]->mutex));
	}

	for(i=0; i<HASH_SIZE; i++)
	{
		if(FLOW_HASH_TABLE[i]->first != NULL)
		{
			slot_consumed++;
		}
	}

	for(i=0; i<HASH_SIZE; i++)
	{
		pthread_mutex_unlock(&(FLOW_HASH_TABLE[i]->mutex));
	}
	return slot_consumed;
}

/*
 * Close a flow forcedly if the delta time is lower than timeslot.
 * Then add the flow to flow queue.
 * Return the number of flows deleted forcedly.
 */
int FHTClearFlowTimeout(const int timeslot)
{
	int i = 0;
	unsigned long delta = 0;
	flow_t *flow = NULL, *flow_next = NULL;
	struct timeval tv;
	struct timezone tz;
	int num = 0;

	for (i=0; i<HASH_SIZE; i++){
		pthread_mutex_lock(&(FLOW_HASH_TABLE[i]->mutex));

		flow = FLOW_HASH_TABLE[i]->first;
		while(flow != NULL){
			flow_next = flow->next;

			gettimeofday(&tv, &tz);
			delta = abs(tv.tv_sec - flow->last_action_sec);

			if (delta > timeslot){
				num++;
				flow->close = FORCED_CLOSE;	// Close flow forcedly.
				//flow_queue_enq(flow_hash_delete(flow));
			}

			flow = flow_next;
		}

		assert(flow == NULL);
		pthread_mutex_unlock(&(FLOW_HASH_TABLE[i]->mutex));
	}
	return num;
}
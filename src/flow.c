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

#include "util.h"
#include "flow.h"
#include "packet.h"
#include "tcp.h"
#include "http.h"

/***********************
 * Static functions      *
 ***********************/
static void 
flow_reset(flow_t *f);
static int 
hook_packet(flow_t *f, packet_t *packet, BOOL src);
static int 
cal_packet(flow_t *f, packet_t *packet, BOOL src);
static int 
compare_sequence_time(seq_t *seq1, seq_t *seq2);

/* Reset a flow_t object to reuse */
static void 
flow_reset(flow_t *f){
	packet_t *cp;

	while(f->pkt_dst_f != NULL){
		cp = f->pkt_dst_f;
		f->pkt_dst_f = f->pkt_dst_f->next;
		packet_free(cp);
	}

	while(f->pkt_src_f != NULL){
		cp = f->pkt_src_f;
		f->pkt_src_f = f->pkt_src_f->next;
		packet_free(cp);
	}

	if(f->order != NULL){
		order_free(f->order);
		f->order = order_new();
	}
	/*
	 * Note: maintain the f->socket.
	 */
	f->pkt_src_e = NULL;
	f->pkt_dst_e = NULL;
	f->rtt = 0;
	f->syn_sec = 0;
	f->syn_usec = 0;
	f->ack2_sec = 0;
	f->ack2_usec = 0;
	f->fb_sec = 0;
	f->fb_usec = 0;
	f->lb_sec = 0;
	f->lb_usec = 0;
	f->last_action_sec = 0;
	f->last_action_usec = 0;
	f->payload_src = 0;
	f->payload_dst = 0;
	f->pkt_src_n = 0;
	f->pkt_dst_n = 0;
	f->pkts_src = 0;
	f->pkts_dst = 0;
	f->http = 0;
	f->close = 0;
}

/* Serve for flow_add_packet(). Add packet to the flow packet chain. */
static int 
hook_packet(flow_t *f, packet_t *packet, BOOL src){
	/* set some pointers */
	packet_t **pkt_f = NULL, **pkt_e = NULL;
	u_int32_t *pkt_num = NULL;

	if(src == TRUE){
		pkt_f = &(f->pkt_src_f);
		pkt_e = &(f->pkt_src_e);
		pkt_num = &(f->pkt_src_n);
	}else{
		pkt_f = &(f->pkt_dst_f);
		pkt_e = &(f->pkt_dst_e);
		pkt_num = &(f->pkt_dst_n);
	}

	if((*pkt_num) == 0){
		(*pkt_f) = packet;
	}else{
		(*pkt_e)->next = packet;
	}

	(*pkt_e) = packet;
	(*pkt_e)->next = NULL;
	(*pkt_num)++;
	return 0;
}

/* Serve for flow_add_packet(). Update flow packet number and bytes, then update the last action time of flow. */
static int 
cal_packet(flow_t *f, packet_t *packet, BOOL src){
	u_int32_t *pkt_num = NULL, *byt_num = NULL;
	struct timeval tv;
	struct timezone tz;
	seq_t *seq = NULL;

	if(src == TRUE){
		pkt_num = &(f->pkts_src);
		byt_num = &(f->payload_src);
	}else{
		pkt_num = &(f->pkts_dst);
		byt_num = &(f->payload_dst);
	}

	(*pkt_num)++;
	(*byt_num) += packet->tcp_dl;

	/*
	 * Update the last action time of flow.
	 * Used to delete some dead flows forcedly.
	 */
	gettimeofday(&tv, &tz);
	f->last_action_sec = tv.tv_sec;
	f->last_action_usec = tv.tv_usec;
	
	/* Process ordering, which must be right AFTER flow info updated */
	seq = seq_pkt(packet);
	if ( packet->http == 0 ){
		/*
		 * The packet without http payload will NOT be stored.
		 */
		seq->pkt = NULL;
	}
	tcp_order(f->order, seq, src);

	return 0;
}

/* Compare seq_t objects if equel */
static int 
compare_sequence_time(seq_t *seq1, seq_t *seq2){
	u_int32_t	sec1 = seq1->cap_sec;
	u_int32_t	usec1 = seq1->cap_usec;
	u_int32_t	sec2 = seq2->cap_sec;
	u_int32_t	usec2 = seq2->cap_usec;
	int ret = 0;

	if(sec1 > sec2 || (sec1 == sec2 && usec1 > usec2)){
		ret = 1;
	}else if (sec1 < sec2 || (sec1 == sec2 && usec1 < usec2)){
		ret = -1;
	}else{
		ret = 0;
	}

	return ret;
}

/***********************
 * Flow functions      *
 ***********************/
/* Initiate both the flow hash table and flow queue */
int
flow_init(void){
	int ret;
	ret = flow_hash_init();
	if(ret != 0){
		return -1;
	}

	ret = flow_queue_init();
	if(ret != 0){
		return -1;
	}

	return 0;
}

/* Create a new flow record and initiate its members */
flow_t*
flow_new(void){
	flow_t *f;
	f = MALLOC(flow_t, 1);
	memset(f, 0, sizeof(flow_t));
	f->order = order_new();
	return f;
}


/* Free a flow_t object */
int 
flow_free(flow_t *f){
	packet_t *cp;
	http_pair_t	*h;
	while(f->pkt_dst_f != NULL){
		cp = f->pkt_dst_f;
		f->pkt_dst_f = f->pkt_dst_f->next;
		packet_free(cp);
	}

	while(f->pkt_src_f != NULL){
		cp = f->pkt_src_f;
		f->pkt_src_f = f->pkt_src_f->next;
		packet_free(cp);
	}

	if(f->order != NULL){
		order_free(f->order);
	}

	if(f->http_f != NULL){
		h = f->http_f;
		f->http_f = f->http_f->next;
		http_free(h);
	}

	free(f);
	return 0;
}

/* Compare two flow_s objects */
int 
flow_socket_cmp(flow_s *a, flow_s *b){
	return memcmp(a, b, sizeof(flow_s));
}

/* Add a http_pair_t object to flow's http_pair_t chain */
int 
flow_add_http(flow_t *f, http_pair_t *h){
	if(f->http_cnt == 0){
		f->http_f = h;
	}else{
		f->http_e->next = h;
	}
	f->http_e = h;
	f->http_e->next = NULL;
	f->http_cnt++;
	return 0;
}

/* Add a packet_t object to flow's packet_t chain */
int 
flow_add_packet(flow_t *f, packet_t *packet, register BOOL src){
	pthread_mutex_lock(&(f->hmb->mutex));

	if( f->http == FALSE ){
		if( f->pkt_src_n >= 5){
			/* We make sure that the flow is not a HTTP flow,
			 * then remove it */
			packet_free(packet);
			flow_free(flow_hash_delete(f));
			pthread_mutex_unlock(&(f->hmb->mutex));
			return 1;
		}
	}

	/* TH_RST:
	 * If the flow is reset by sender or receiver*/
	if((packet->tcp_flags & TH_RST) == TH_RST){
		if( f->pkts_src < 4){
			// Flow with uncomplete information. Drop it.
			packet_free(packet);
			flow_free(flow_hash_delete(f));
			pthread_mutex_unlock(&(f->hmb->mutex));
			return 1;
		}else{
			cal_packet(f, packet, src);
			packet_free(packet);
			f->close = TRUE;
			flow_queue_enq(flow_hash_delete(f));
			pthread_mutex_unlock(&(f->hmb->mutex));
            return 0;
		}
	}

	/* TH_ACK: third handshake */
	if(f->pkts_src == 1 && src == TRUE){
		if((packet->tcp_flags & TH_ACK) == TH_ACK){
			f->ack2_sec = packet->cap_sec;
			f->ack2_usec = packet->cap_usec;
			/* round trip time in microsecond */
			f->rtt = (f->ack2_sec - f->syn_sec) * 1000000 + (f->ack2_usec - f->syn_usec);

			cal_packet(f, packet, src);
			packet_free(packet);
			pthread_mutex_unlock(&(f->hmb->mutex));
			return 0;
		}
	}

	/* TH_FIN:
	 * The flow will be closed if the both fins are detected */
	if( (packet->tcp_flags & TH_FIN) == TH_FIN){
		if( src == TRUE ){
			f->close = CLIENT_CLOSE;
		}else{
			f->close = SERVER_CLOSE;
		}		
		cal_packet(f, packet, src);
		packet_free(packet);

		if(f->close == CLIENT_CLOSE  || f->close == SERVER_CLOSE){		/* && or || */
			/* flow finished and send it to the flow queue */
			f->close = TRUE;
			flow_queue_enq(flow_hash_delete(f));
		}

		pthread_mutex_unlock(&(f->hmb->mutex));
		return 0;
	}

	/* other packets, without sequence number checked */
	if(src == TRUE){
		if( f->pkts_src == 0){
			/* syn */
			f->syn_sec = packet->cap_sec;
			f->syn_usec = packet->cap_usec;

			cal_packet(f, packet, src);
			packet_free(packet);
		}else{
			if(packet->tcp_flags == TH_SYN){
				/*syn repeatly*/
				flow_reset(f);		// Reset flow
				f->syn_sec = packet->cap_sec;
				f->syn_usec = packet->cap_usec;
				cal_packet(f, packet, src);
				packet_free(packet);
			}else{
				if(packet->http != 0 ){
					f->http = TRUE;
					/*
					 * only packets with HTTP payload
					 * are hooked on the packet chain
					 */
					hook_packet(f, packet, src);
					cal_packet(f, packet,src);
				}else{
					cal_packet(f, packet, src);
					packet_free(packet);
				}
			}
		}
	}else{
		if(packet->http != 0){
			f->http = TRUE;
			/*
			 * only packets with HTTP payload
			 * are hooked on the packet chain
			 */
			hook_packet(f, packet, src);
			cal_packet(f, packet, src);
		}else{
			cal_packet(f, packet, src);
			packet_free(packet);
		}
	}

	pthread_mutex_unlock(&(f->hmb->mutex));
	return 0;
}

/* Extract http_pair_t objects from flow's packet_t chain */
int 
flow_extract_http(flow_t *f){
	/* check if the flow is carrying HTTP again */
	if( f->http == FALSE)
		return 1;
		
	/*
	 * Find the actual FIN sequences.
	 */
	seq_t	*seq = f->order->src;
	seq_t	*src_fin_seq = NULL;
	seq_t	*dst_fin_seq = NULL;
	int found = 0;
	
	while(seq != NULL){
		/* Update flow's first byte time.
		 * FBT of flow refers to the payload's FBT.
		 */
		if(seq->pkt != NULL && found == 0){
			found = 1;
			f->fb_sec = seq->cap_sec;
			f->fb_usec = seq->cap_usec;
		}
		
		/*Search the FIN sequence in sequence queue.*/
		if((seq->th_flags & TH_FIN) == TH_FIN){
			src_fin_seq = seq;
			break;
		}
		seq = seq->next;
	}
	
	seq = f->order->dst;
	while(seq != NULL){
		/*Search the FIN sequence in sequence queue.*/
		if((seq->th_flags & TH_FIN) == TH_FIN){
			dst_fin_seq = seq;
			break;
		}
		seq = seq->next;
	}
	
	/*
	 * Set the client and server FIN sequences.
	 */
	seq_t	*fin_seq = NULL;	/* The actual FIN sequence. */
	u_int8_t	fin_dir = 0;	/* fin_dir:
	 * 0: Not set;
	 * 1: src_fin_seq is used;
	 * 2: dst_fin_seq is used;
	 */
	
	if (src_fin_seq != NULL && dst_fin_seq == NULL){
		fin_seq = src_fin_seq;
		fin_dir = 1;
	}else if (src_fin_seq == NULL && dst_fin_seq != NULL){
		fin_seq = dst_fin_seq;
		fin_dir = 2;
	}else if (src_fin_seq != NULL && dst_fin_seq != NULL){
		fin_seq = src_fin_seq;
		fin_dir = 1;
		if(compare_sequence_time(src_fin_seq, dst_fin_seq) == 1){
			fin_seq = dst_fin_seq;
			fin_dir = 2;
		}
	}else{
		fin_seq = NULL;
		fin_dir = 0;
	}
	
	/* 
	 * First step: find requests 
	 */
	packet_t *pkt;
	request_t	*req;
	response_t	*rsp;
	int reqn = 0;	// Number of requests.
	int rspn = 0;	// Number of responses.
	
	http_pair_t *new_http = NULL;
	seq_t *seq_next = NULL;	/* for temp */
	seq_t *first_seq = NULL;
	/* Set seq and seq_next */
	seq = f->order->src;
	if( seq != NULL){
		seq_next = seq->next;
	}else{
		seq_next = NULL;		/* NULL */
	}
	
	if (fin_seq != NULL && seq != NULL){
		/*A FIN packet exists.*/
		while(compare_sequence_time(seq, fin_seq) < 0){
			pkt = seq->pkt;
			if(pkt != NULL && pkt->http == HTTP_REQ){
				/* When a new HTTP request is found,
				 * create a HTTP pair object, then add the object to
				 * flow's HTTP chain.
				 */
				reqn++;
				/* new HTTP pair object*/
				new_http = http_new();
				first_seq = seq;
				new_http->req_fb_sec = seq->cap_sec;
				new_http->req_fb_usec = seq->cap_usec;
				new_http->req_lb_sec = seq->cap_sec;
				new_http->req_lb_usec = seq->cap_usec;
					
				/* Add the object to flow's HTTP chain */
				flow_add_http(f, new_http);
				/* new request object */
				req = http_request_new();
				/* Add the request object to the foregoing HTTP pair object */
				http_add_request(new_http, req);
				/* parse and write values to foregoing request object */
				http_parse_request(req, pkt->tcp_odata, pkt->tcp_odata + pkt->tcp_dl);
			}else{
				/*Omit the TCP handshake sequences.*/
				if(new_http == NULL){
					seq = seq->next;
					if(seq != NULL)
						seq_next = seq->next;
					else
						break;
					continue;
				}
			}

			if( new_http != NULL ){
				if( seq_next == NULL || seq_next == fin_seq || seq_next->pkt != NULL ||\
						compare_sequence_time(seq_next, fin_seq) >= 0 ){
					//assert(seq->nxt_seq != 0);
					if( seq->nxt_seq != 0){
						new_http->req_total_len = seq->nxt_seq - first_seq->seq;
						new_http->req_body_len = 0;
					}
					/*Update flow's last byte time.*/
					if ((seq->cap_sec > f->lb_sec) || (seq->cap_sec == f->lb_sec && seq->cap_usec > f->lb_usec)){
						f->lb_sec = seq->cap_sec;
						f->lb_usec = seq->cap_usec;
					}
				}else{
					//assert(seq->seq <= seq_next->seq);
				}
			}
			
			/* Continue to next sequence.*/
			seq = seq->next;
			if(seq != NULL)
				seq_next = seq->next;
			else
				break;
		}
	}else{
		/* No FIN packet found.*/
		while(seq != NULL){
			pkt = seq->pkt;
			if(pkt != NULL && pkt->http == HTTP_REQ){
				/* When a new HTTP request is found,
				 * create a HTTP pair object, then add the object to
				 * flow's HTTP chain.
				 */
				reqn++;
				/* new HTTP pair object*/
				new_http = http_new();
				first_seq = seq;
				new_http->req_fb_sec = seq->cap_sec;
				new_http->req_fb_usec = seq->cap_usec;
				new_http->req_lb_sec = seq->cap_sec;
				new_http->req_lb_usec = seq->cap_usec;
				
				/* Add the object to flow's HTTP chain */
				flow_add_http(f, new_http);
				/* new request object */
				req = http_request_new();
				/* Add the request object to the foregoing HTTP pair object */
				http_add_request(new_http, req);
				/* parse and write values to foregoing request object */
				http_parse_request(req, pkt->tcp_odata, pkt->tcp_odata + pkt->tcp_dl);
			}else{
				if(new_http == NULL){
					/*Omit the TCP handshake sequences.*/
					seq = seq->next;
					if(seq != NULL)
						seq_next = seq->next;
					else
						break;
					continue;
				}
			}
			if( new_http != NULL ){
				if( seq_next == NULL || seq_next->pkt != NULL ){
					//assert(seq->nxt_seq != 0);
					if( seq->nxt_seq != 0){
						new_http->req_total_len = seq->nxt_seq - first_seq->seq;
						new_http->req_body_len = 0;
					}
					/*Update flow's last byte time.*/
					if ((seq->cap_sec > f->lb_sec) || (seq->cap_sec == f->lb_sec && seq->cap_usec > f->lb_usec)){
						f->lb_sec = seq->cap_sec;
						f->lb_usec = seq->cap_usec;
					}
				}else{
					//assert(seq->seq <= seq_next->seq);
				}
			}
			/*Continue to next sequence.*/
			seq = seq->next;
			if(seq != NULL)
				seq_next = seq->next;
			else
				break;
		}
	}

	/* If no responses found, we treat the flow as invalid and stop parsing */
	if(reqn == 0)
		return 1;
		
	/* Second step: find responses */
	http_pair_t *tmp = f->http_f;
	http_pair_t *found_http = NULL;
	seq = f->order->dst;
	if( seq != NULL)
		seq_next = seq->next;
	else
		seq_next = NULL;		/* NULL */
	if(fin_seq != NULL && seq != NULL){
		/*There is FIN packet.*/
		while(compare_sequence_time(seq, fin_seq) < 0){
			pkt = seq->pkt;
			if ( pkt != NULL && pkt->http == HTTP_RSP){
				/*
				 * Similar to the request parsing, a new response is
				 * added to the first pair without response
				 */
				rspn++;
				/* Try to find the first pair without response */
				while(tmp != NULL){
					if(tmp->response_header == NULL)
						break;
					tmp = tmp->next;
				}
				if(tmp == NULL)
					/* no response empty, then return */
					return 1;
				else{
					/*Found!*/
					found_http = tmp;
					first_seq = seq;
					found_http->rsp_fb_sec = seq->cap_sec;
					found_http->rsp_fb_usec = seq->cap_usec;
					rsp = http_response_new();
					http_add_response(found_http, rsp);
					http_parse_response(rsp, pkt->tcp_odata, pkt->tcp_odata + pkt->tcp_dl);
				}
			}else{
				if(found_http == NULL){
					seq = seq->next;
					if(seq != NULL)
						seq_next = seq->next;
					else
						break;
					continue;
				}
			}

			if ( found_http != NULL ){
				/*first_seq != NULL*/
				if( seq_next == NULL || seq_next == fin_seq || seq_next->pkt != NULL ||\
						compare_sequence_time(seq_next, fin_seq) >= 0 ){
					found_http->rsp_lb_sec = seq->cap_sec;
					found_http->rsp_lb_usec = seq->cap_usec;
					//assert( seq->nxt_seq != 0 );
					if(seq->nxt_seq != 0){
						found_http->rsp_total_len = seq->nxt_seq - first_seq->seq;
						found_http->rsp_body_len = found_http->rsp_total_len - found_http->response_header->hdlen;
						if (found_http->rsp_body_len < 0)
						{
						    found_http->rsp_body_len = -1;
						}
					}
					/*Update flow's last byte time.*/
					if ((seq->cap_sec > f->lb_sec) || (seq->cap_sec == f->lb_sec && seq->cap_usec > f->lb_usec)){
						f->lb_sec = seq->cap_sec;
						f->lb_usec = seq->cap_usec;
					}
				}else{
					//assert(seq->seq <= seq_next->seq);
				}
			}

			seq = seq->next;
			if(seq != NULL)
				seq_next = seq->next;
			else
				break;
		}
	}else{
		/*There is no FIN packet.*/
		while(seq != NULL){
			pkt = seq->pkt;
			if ( pkt != NULL && pkt->http == HTTP_RSP ){
				/*
				 * Similar to the request parsing, a new response is
				 * added to the first pair without response
				 */
				rspn++;
				/* Try to find the first pair without response */
				while(tmp != NULL){
					if(tmp->response_header == NULL)
						break;
					tmp = tmp->next;
				}
				if(tmp == NULL)
					/* no response empty, then return */
					return 1;
				else{
					/*Found!*/
					found_http = tmp;
					first_seq = seq;
					found_http->rsp_fb_sec = seq->cap_sec;
					found_http->rsp_fb_usec = seq->cap_usec;
					rsp = http_response_new();
					http_add_response(found_http, rsp);
					http_parse_response(rsp, pkt->tcp_odata, pkt->tcp_odata + pkt->tcp_dl);
				}
			}else{
				if(found_http == NULL){
					seq = seq->next;
					if(seq != NULL)
						seq_next = seq->next;
					else
						break;
					continue;
				}
			}

			if ( found_http != NULL ){
				/*first_seq != NULL*/
				if( seq_next == NULL || seq_next->pkt != NULL ){
					found_http->rsp_lb_sec = seq->cap_sec;
					found_http->rsp_lb_usec = seq->cap_usec;
					//assert( seq->nxt_seq != 0 );
					if(seq->nxt_seq != 0){
						found_http->rsp_total_len = seq->nxt_seq - first_seq->seq;	
						found_http->rsp_body_len = found_http->rsp_total_len - found_http->response_header->hdlen;
						if (found_http->rsp_body_len < 0)
                        {
                            found_http->rsp_body_len = -1;
                        }
					}
					/*Update flow's last byte time.*/
					if ((seq->cap_sec > f->lb_sec) || (seq->cap_sec == f->lb_sec && seq->cap_usec > f->lb_usec)){
						f->lb_sec = seq->cap_sec;
						f->lb_usec = seq->cap_usec;
					}
				}else{
					//assert(seq->seq <= seq_next->seq);
				}
			}

			seq = seq->next;
			if(seq != NULL)
				seq_next = seq->next;
			else
				break;
		}
	}
	return 0;
}

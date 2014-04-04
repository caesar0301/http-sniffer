/*
 * order.c
 *
 *  Created on: Jun 14, 2011
 *      Author: chenxm
 */
#include "packet.h"
#include "util.h"
#include "order.h"

seq_t *seq_new(void){
	seq_t *seq;
	seq = MALLOC(seq_t, 1);
	memset(seq, 0, sizeof(seq_t));
	return seq;
}

void seq_free(seq_t *seq){
	/* packet is freed by packet_free to avoid double free */
	free(seq);
}

order_t *order_new(void){
	order_t	*order;
	order = MALLOC(order_t, 1);
	memset(order, 0, sizeof(order_t));
	return order;
}

void order_free(order_t *order){
	seq_t *s = NULL;

	while(order->src != NULL){
		s = order->src;
		order->src = order->src->next;
		seq_free(s);
	}
	while(order->dst != NULL){
		s = order->dst;
		order->dst = order->dst->next;
		seq_free(s);
	}

	free(order);
}

seq_t *seq_pkt(packet_t *p){
	seq_t *s;
	s = MALLOC(seq_t, 1);
	s->pkt = p;
	if ((p->tcp_flags & TH_ACK) == TH_ACK)
		s->ack = TRUE;
	else
		s->ack = FALSE;
	s->next = NULL;
	s->seq = p->tcp_seq;
	if((p->tcp_flags & TH_SYN) == TH_SYN || (p->tcp_flags & TH_FIN) == TH_FIN){
		s->nxt_seq = p->tcp_seq + p->tcp_dl +1;
	}else{
		s->nxt_seq = p->tcp_seq + p->tcp_dl;
	}
	s->th_flags = p->tcp_flags;
	s->cap_sec = p->cap_sec;
	s->cap_usec = p->cap_usec;

	return s;
}

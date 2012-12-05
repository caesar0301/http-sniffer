/*
 * tcp.c
 *
 *  Created on: Jun 15, 2011
 *      Author: chenxm
 */
#include <assert.h>

#include "tcp.h"
#include "util.h"
	 
	 
seq_t *TCPSeqNew(void){
 	seq_t *seq;
 	seq = MALLOC(seq_t, 1);
 	memset(seq, 0, sizeof(seq_t));
 	return seq;
}

void TCPSeqFree(seq_t *seq){
 	/* packet is freed by packet_free to avoid double free */
 	free(seq);
}

order_t *TCPOrderNew(void){
 	order_t	*order;
 	order = MALLOC(order_t, 1);
 	memset(order, 0, sizeof(order_t));
 	return order;
}

void TCPOrderFree(order_t *order){
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

seq_t *TCPSeqFromPkt(packet_t *p){
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

/*
 * Search for a continuous segment in order
 */
static seq_t *tcp_cont_seq(seq_t *lst)
{
    while (lst->next != NULL && lst->nxt_seq == lst->next->seq)
        lst = lst->next;
    return lst;
}

/*
 * Add seq of packet to the TCP order
 */
int TCPOrderAddSeq(order_t *ord, seq_t *new_seq, u_int8_t src){
	seq_t **plist=NULL, **plist_last=NULL, *pre=NULL, *cp=NULL, *aft=NULL;
	u_int32_t fr=0, bk=0;

	if(src == TRUE){
		plist = &(ord->src);
		plist_last = &(ord->last_src);
	}else{
		plist = &(ord->dst);
		plist_last = &(ord->last_dst);
	}

	if( (*plist) == NULL ){
		/* first packet */
		(*plist) = new_seq;
		(*plist_last) = new_seq;
		ord->num++;
		return 0;
	}else{
		/* set some variables fr,pre,cp,aft,bk to add a new seq to order */
		if((*plist_last)->nxt_seq == new_seq->seq){
			fr = ((*plist_last)->seq);
			pre = (*plist_last);
			cp = tcp_cont_seq((*plist_last));
			aft = cp->next;
			bk = bk = cp->nxt_seq - 1;
		}
		else{
			cp = (*plist);
			/* search position */
			pre = cp;
			while(cp != NULL && (cp->nxt_seq) <= (new_seq->seq)){	/* not (cp->nxt_seq) <= (new_seq->seq) */
				pre = cp;
				cp = cp->next;
			}
			if( cp == NULL){
				/* at list end */
				pre->next = new_seq;
				(*plist_last) = new_seq;
				ord->num++;
				return 0;
			}else{
				fr = pre->seq;				/* start of continuous segment */
				cp = tcp_cont_seq(pre);	 	/* the last seq_t in a continuous segment */
				aft = cp->next;
				bk = cp->nxt_seq - 1;		/* end of continuous segment */
			}
		}
		/* add new seq to the right position */
		if((new_seq->seq) >= fr && (new_seq->seq) <= bk ){
			if( (new_seq->nxt_seq -1) <= bk){
				/* retransmission */
				seq_free(new_seq);
				return 1;
			}else{
				/* adjust the packet */
				u_int32_t	delta = (new_seq->nxt_seq-1) - bk;
				if(new_seq->pkt != NULL){
					if(new_seq->pkt->tcp_data != NULL){				/* check if we store the packet payload */
						new_seq->pkt->tcp_data = new_seq->pkt->tcp_data + (new_seq->pkt->tcp_dl - delta);
					}
					new_seq->pkt->tcp_dl = delta;
				}
				new_seq->seq = bk + 1;
				/* update order */
				cp->next = new_seq;
				(*plist_last) = new_seq;
				if( aft != NULL ){
					new_seq->next = aft;
					/* Connect with follower */
					if(new_seq->nxt_seq <= aft->seq){
						new_seq->next = aft;
						(*plist_last) = new_seq;
						ord->num++;
						return 0;
					}else{
						/* partially overlapped */
						if(new_seq->pkt != NULL){
							new_seq->pkt->tcp_dl -= (new_seq->nxt_seq - aft->seq);
						}
						new_seq->nxt_seq = aft->seq;
						new_seq->next = aft;
						(*plist_last) = new_seq;
						ord->num++;
						return 0;
					}
				}

				return 0;
			}
		}else{
			cp->next = new_seq;

			if(aft != NULL){
				if(new_seq->nxt_seq <= aft->seq){
					new_seq->next = aft;
					(*plist_last) = new_seq;
					ord->num++;
					return 0;
				}else{
					/* partially overlapped */
					if(new_seq->pkt != NULL){
						new_seq->pkt->tcp_dl -= (new_seq->nxt_seq - aft->seq);
					}
					new_seq->nxt_seq = aft->seq;
					new_seq->next = aft;
					(*plist_last) = new_seq;
					ord->num++;
					return 0;
				}
			}

			return 0;
		}
	}
}

/* for debugging */
/* check the if TCP is ordered correctly */
int TCPOrderCheck(order_t *order){
	u_int32_t seq, next_seq;
	int src_check = 0, dst_check = 0;
	seq_t	*ps;

	if(order == NULL){
		return 0;
	}

	ps = order->src;
	if(ps == NULL){
		src_check = 1;
	}else{
		while(ps->next != NULL){
			if(ps->nxt_seq == ps->next->seq){
				ps = ps->next;
			}else{
				printf("%lu\t%lu\n", ps->nxt_seq, ps->next->seq);
				src_check = 0;
				break;
			}
		}
		if(ps->next == NULL){
			src_check = 1;
		}
	}

	ps = order->dst;
	if( ps == NULL){
		dst_check = 1;
	}else{
		while(ps->next != NULL){
			if(ps->nxt_seq == ps->next->seq){
				ps = ps->next;
			}else{
				printf("%lu\t%lu\n", ps->nxt_seq, ps->next->seq);
				dst_check = 0;
				break;
			}
		}
		if(ps->next == NULL){
			dst_check = 1;
		}
	}

	if(src_check == 1 && dst_check == 1){
		return TRUE;
	}
	return FALSE;
}

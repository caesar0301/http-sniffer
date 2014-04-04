/*
 * queue.c
 *
 *  Created on: Mar 16, 2012
 *      Author: chenxm
 *		Email: chen_xm@sjtu.edu.cn
 */
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "packet.h"
#include "util.h"

/* Queue vars */
static unsigned int que_len = 0;
static packet_t *pkt_first = NULL;
static packet_t *pkt_last = NULL;
static pthread_mutex_t mutex;

BOOL 
packet_queue_init(void)
{
	int ret;
	pkt_first = NULL;
	pkt_last = NULL;
	que_len = 0;
	ret = pthread_mutex_init(&mutex, NULL);
	if(ret != 0)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL 
packet_queue_enq(packet_t *pkt)
{
	pthread_mutex_lock(&mutex);
	if(que_len == 0)
	{
		pkt_first = pkt;
		pkt_last = pkt;
		pkt_last->next = NULL;
		que_len++;
		pthread_mutex_unlock(&mutex);
		return TRUE;
	}
	pkt_last->next = pkt;
	pkt_last = pkt;
	pkt_last->next = NULL;
	que_len++;
	pthread_mutex_unlock(&mutex);
	return TRUE;
}

packet_t *
packet_queue_deq(void)
{
	pthread_mutex_lock(&mutex);
	packet_t *pkt;
	if(que_len == 0)
	{
		pthread_mutex_unlock(&mutex);
		return NULL;
	}
	else if (que_len == 1 )
	{
		pkt_last = NULL;
	}
	pkt = pkt_first;
	pkt_first = pkt_first->next;
	que_len--;
	pthread_mutex_unlock(&mutex);
	return pkt;
}

BOOL 
packet_queue_clr(void)
{
	pthread_mutex_lock(&mutex);
	packet_t *pkt;
	while(que_len > 0)
	{
		pkt = pkt_first;
		pkt_first = pkt_first->next;
		packet_free(pkt);
		que_len--;
	}
	pkt_first =  NULL;
	pkt_last = NULL;
	que_len = 0;
	pthread_mutex_unlock(&mutex);
	return TRUE;
}

unsigned int 
packet_queue_len(void)
{
	return que_len;
}

void 
packet_queue_print(void)
{
	printf("(Packet queue length)%d\n", packet_queue_len());
}

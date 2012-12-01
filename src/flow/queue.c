/*
 * queue.c
 *
 *  Created on: Mar 16, 2012
 *      Author: front
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

static pthread_mutex_t mutex_queue;
static flow_t *flow_queue_first = NULL;
static flow_t *flow_queue_last = NULL;
static int flow_qlen = 0;	/* flow queue length */

/************************
 * flow queue functions *
 ************************/
int 
flow_queue_init(void)
{
	int ret = 0;
	ret = pthread_mutex_init(&mutex_queue, NULL);
	if(ret != 0){
		return -1;
	}

	flow_queue_first = NULL;
	flow_queue_last = NULL;
	flow_qlen = 0;

	return 0;
}

int 
flow_queue_enq(flow_t *flow)
{
	pthread_mutex_lock(&mutex_queue);

	if (flow_qlen == 0){
		flow_queue_first = flow;
		flow_queue_last = flow;
		flow_queue_last->next = NULL;
		flow_qlen++;
		pthread_mutex_unlock(&mutex_queue);
		return 0;
	}

	flow_queue_last->next = flow;
	flow_queue_last = flow;
	flow_queue_last->next = NULL;
	flow_qlen++;

	pthread_mutex_unlock(&mutex_queue);
	return 0;
}

flow_t*
flow_queue_deq(void)
{
	pthread_mutex_lock(&mutex_queue);

	flow_t *f = NULL;
	if(flow_qlen == 0){
		pthread_mutex_unlock(&mutex_queue);
		return NULL;
	}else if(flow_qlen == 1){
		flow_queue_last = NULL;
	}

	f = flow_queue_first;
	flow_queue_first = flow_queue_first->next;
	flow_qlen--;

	pthread_mutex_unlock(&mutex_queue);
	return f;
}

int 
flow_queue_clear(void)
{
	pthread_mutex_lock(&mutex_queue);

	flow_t *f;

	while(flow_qlen > 0){
		f = flow_queue_first;
		flow_queue_first = flow_queue_first->next;
		flow_free(f);
		flow_qlen--;
	}
	flow_queue_first =  NULL;
	flow_queue_last = NULL;
	flow_qlen = 0;

	pthread_mutex_unlock(&mutex_queue);
	return 0;
}

int 
flow_queue_len(void)
{
	return flow_qlen;
}

void 
flow_queue_print(void)
{
	printf("(Flow queue length)%d\n", flow_queue_len());
}

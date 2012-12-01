/*
 * tcp.h
 *
 *  Created on: Jun 15, 2011
 *      Author: chenxm
 */

#ifndef __TCP_H__
#define __TCP_H__

#include "order.h"
#include "util.h"

int tcp_order(order_t *ord, seq_t *new_seq, BOOL src);
int tcp_order_check(order_t *order);	// for debugging

#endif /* __TCP_H__ */

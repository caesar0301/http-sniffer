/* capture.h */
#ifndef __CAPTURE_H__
#define __CAPTURE_H__

#include "util.h"

BOOL
capture_finished(void);
int
capture_main(const char*, void (*pkt_handler)(void*), int);
	
#endif /* __CAPTURE_H__ */

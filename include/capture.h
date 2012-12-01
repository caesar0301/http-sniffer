/* capture.h */
#ifndef __CAPTURE_H__
#define __CAPTURE_H__

#include "util.h"

BOOL
capture_finished(void);
int
capture_main(const char* interface, void (*pkt_handler)(void*));
int 
capture_offline(const char* filename, void (*pkt_handler)(void*));
	
#endif /* __CAPTURE_H__ */

#ifndef __JOBS_H__
#define __JOBS_H__

int process_packet_queue(void);
void scrubbing_flow_htbl(void);
int process_flow_queue(const char* dump_file);
void debugging_print(void);

#endif	/* __JOBS_H__ */
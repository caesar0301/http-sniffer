/*
 * Main.c
 */
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>		/* getopt() */

#include "capture.h"
#include "jobs.h"
#include "packet.h"

void
print_usage(const char* pro_name)
{
	printf("Usage: %s -i interface [-o dumpfile]\n", pro_name);
	printf("    or %s -f tracefile [-o dumpfile]\n", pro_name);
}

int main(int argc, char *argv[]){
	char* interface = NULL;
	char* dumpfile = NULL;
	char* tracefile = NULL;
	int opt;
	while((opt = getopt(argc, argv, ":i:f:o:h")) != -1)
	{
		switch(opt)
		{
			case 'h':
				print_usage(argv[0]);
				return (1);
			case 'i':
				interface = optarg;
				break;
			case 'o':
				dumpfile = optarg;
				break;
			case 'f':
				tracefile = optarg;
				break;
			default:	/* '?' */
				print_usage(argv[0]);
				return (1);
		}
	}
	if (interface == NULL && tracefile == NULL)
	{
		print_usage(argv[0]);
		return (1);
	}

	time_t start, end;
	time(&start);
	printf("Working from: %s\n", ctime(&start));
	
	pthread_t j_pkt_q;
	pthread_t j_flow_q;
	pthread_t j_scrb_htbl;
	pthread_t j_debug_p;
	void *thread_result;
	packet_queue_init();	/* Initialize packet queue */
	flow_init();			/* Initialize flow queue and hashtable */
	pthread_create(&j_pkt_q, NULL, (void*)process_packet_queue, NULL);
	pthread_create(&j_debug_p, NULL, (void*)debugging_print, NULL);
	pthread_create(&j_flow_q, NULL, (void*)process_flow_queue, dumpfile);
	pthread_create(&j_scrb_htbl, NULL, (void*)scrubbing_flow_htbl, NULL);
	
	if (interface != NULL){
		capture_main(interface, packet_queue_enq);
	}else	//tracefile != NULL
	{
		capture_offline(tracefile, packet_queue_enq);
	}

	pthread_join(j_pkt_q, &thread_result);
	pthread_join(j_debug_p, &thread_result);
	pthread_join(j_flow_q, &thread_result);
	pthread_join(j_scrb_htbl, &thread_result);
	
	time(&end);
	printf("time elapsed is %d s\n", (int)(end - start));
	return 0;
}

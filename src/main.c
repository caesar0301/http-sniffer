/*
 * Main.c
 */
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>		/* getopt() */

#include "capture.h"
#include "packet.h"
#include "flow.h"

int GP_CAP_FIN = 0; // Used for offline pcap sniffing
int process_packet_queue(void);
int process_flow_queue(const char*);
void scrubbing_flow_htbl(void);
void debugging_print(void); // For debugging

void
print_usage(const char* pro_name){
	printf("Usage: %s -i interface [-o dumpfile]\n", pro_name);
	printf("    or %s -f tracefile [-o dumpfile]\n", pro_name);
}


int main(int argc, char *argv[]){
	char* interface = NULL;
	char* dumpfile = NULL;
	char* tracefile = NULL;
	int opt;
	// Parse arguments
	while((opt = getopt(argc, argv, ":i:f:o:h")) != -1){
		switch(opt){
		case 'h':
			print_usage(argv[0]); return (1);
		case 'i':
			interface = optarg; break;
		case 'o':
			dumpfile = optarg; break;
		case 'f':
			tracefile = optarg; break;
		default:	/* '?' */
			print_usage(argv[0]); return (1);
		}
	}
	// Check interfaces
	if (interface == NULL && tracefile == NULL){
		print_usage(argv[0]); return (1);
	}

	time_t start, end;
	time(&start);
	pthread_t j_pkt_q;
	pthread_t j_flow_q;
	pthread_t j_scrb_htbl;
	pthread_t j_debug_p;
	void *thread_result;

	printf("Working from: %s\n", ctime(&start));
	packet_queue_init();	/* Initialize packet queue */
	flow_init();			/* Initialize flow queue and hashtable */

	// Start backend jobs defined in jobs.c
	pthread_create(&j_pkt_q, NULL, (void*)process_packet_queue, NULL);
	pthread_create(&j_scrb_htbl, NULL, (void*)scrubbing_flow_htbl, NULL);
	pthread_create(&j_flow_q, NULL, (void*)process_flow_queue, dumpfile);
	pthread_create(&j_debug_p, NULL, (void*)debugging_print, NULL);

	// Start main capture in live or offline mode
	if (interface != NULL)
		capture_main(interface, packet_queue_enq, 1);
	else //tracefile != NULL
		capture_main(tracefile, packet_queue_enq, 0);
	printf("hello");
	
	// Wait the threads to end
	pthread_join(j_pkt_q, &thread_result);
	pthread_join(j_debug_p, &thread_result);
	pthread_join(j_flow_q, &thread_result);
	pthread_join(j_scrb_htbl, &thread_result);
	
	time(&end);
	printf("time elapsed is %d s\n", (int)(end - start));
	return 0;
}


/* Fetch a packet from packet queue and add it to any flow. */
int 
process_packet_queue(void){
	packet_t *pkt = NULL;
	while(1){
		pkt = packet_queue_deq();
		if (pkt != NULL){
			flow_hash_add_packet(pkt);
			continue;
		} else if ( GP_CAP_FIN == 1 ) 
			break;
	}
	pthread_exit("Packet processing finished.\n");
	return 0;
}

// char*
// gen_filename(void){
// 	char file_name_buf[64];
// 	memset(file_name_buf, 0, sizeof(file_name_buf));
	
// 	time_t raw_time;
// 	struct tm *timeinfo = NULL;
// 	char time_buf[20];
// 	memset(time_buf, 0, sizeof(time_buf));
// 	time( &raw_time );
// 	timeinfo = localtime( &raw_time );
// 	strftime(time_buf, sizeof(time_buf), "%Y%m%d%H", timeinfo);
// 	strcat(file_name_buf, time_buf);
// 	strcat(file_name_buf, ".txt");
// 	return file_name_buf;
// }

/* Fetch a flow from flow queue and process it */
int 
process_flow_queue(const char* dump_file){
	flow_t	*flow = NULL;
	char* file_name = NULL;
	while(1){
		file_name = NULL;
		if (dump_file != NULL) file_name = dump_file;

		flow = flow_queue_deq();
		if(flow != NULL){
			flow_extract_http(flow);
			if (file_name == NULL){
				static char file_name_buf[64];
				memset(file_name_buf, 0, sizeof(file_name_buf));
				time_t raw_time;
				struct tm *timeinfo = NULL;
				char time_buf[20];
				memset(time_buf, 0, sizeof(time_buf));
				time( &raw_time );
				timeinfo = localtime( &raw_time );
				strftime(time_buf, sizeof(time_buf), "%Y%m%d%H", timeinfo);
				strcat(file_name_buf, time_buf);
				strcat(file_name_buf, ".txt");
				file_name = file_name_buf;
			}
			flow_dump_file_json(flow, file_name);
			//flow_dump_file_plain(flow, file_name);	// Fault
			flow_print(flow);
			flow_free(flow);
			continue;
		} else if ( GP_CAP_FIN == 1 ) 
			break;
	}
	pthread_exit("Flow processing finished.\n");
	return 0;
}


/* Scrub flow hash table to forcely close dead flows.*/
void 
scrubbing_flow_htbl(void){
	int num = 0;
	while(1){
		sleep(1);
		if ( GP_CAP_FIN == 0 )
			num = flow_scrubber(60*10);	// seconds, flow timeout
		else {
			num = flow_scrubber(-1); // all flows
			break;
		}
	}
	pthread_exit(NULL);
}


void 
debugging_print(void){
	while(1){
		if ( GP_CAP_FIN == 1 ) break;
		else {
			//packet_queue_print();
			flow_hash_print();
			//flow_queue_print();
			sleep(1);
		}
	}
	pthread_exit(NULL);
}
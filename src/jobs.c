/*
 * jobs.c
 *
 *  Created on: Mar 25, 2012
 *      Author: chenxm
 *		Email: chen_xm@sjtu.edu.cn
 */
#include "packet.h"
#include "flow.h"

/* Fetch a packet from packet queue and add it to any flow. */
int 
process_packet_queue(void)
{
	packet_t *pkt = NULL;
	while(1)
	{
		pkt = packet_queue_deq();
		if (pkt != NULL)
		{
			flow_hash_add_packet(pkt);
			continue;
		}
	}
	pthread_exit("Packet processing finished.\n");
	return 0;
}

/* Scrub flow hash table to forcely close dead flows.*/
void 
scrubbing_flow_htbl(void)
{
	int num = 0;
	while(1){
		sleep(1);
		num = flow_scrubber(60);	/* seconds */
	}
	pthread_exit("Flow processing finished.\n");
	return 0;
}

char*
gen_filename(void){
	char file_name_buf[64];
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
	return file_name_buf;
}

/* Fetch a flow from flow queue and process it */
int 
process_flow_queue(const char* dump_file)
{
	flow_t	*flow = NULL;
	char* file_name = NULL;
	while(1)
	{
		file_name = NULL;
		if (dump_file != NULL){
			file_name = dump_file;
		}
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
		}
	}
	pthread_exit("Flow processing finished.\n");
	return 0;
}

/* for debugging */
void 
debugging_print(void)
{
	while(1)
	{
		sleep(3);
		printf("...................................\n");
		packet_queue_print();
		flow_hash_print();
		flow_queue_print();
	}
	pthread_exit("Flow processing finished.\n");
	return 0;
}

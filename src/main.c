/*
 * Main.c
 */
#include <stdio.h>
#include <time.h>
#include <unistd.h>		/* getopt() */

#include "pcap.h"
#include "queue.h"
#include "capture.h"
#include "flow.h"

// Stop signs for program 
static long limit_pktcnt = -1;
static long limit_filesize = -1;
static long limit_seconds = -1;
static long var_pktcnt = 0;
static long var_filesize = 0;
static long var_seconds = 0;
static time_t t_start, t_now;

static void print_usage(const char* pro_name)
{
	printf("This program is to sniff the HTTP messages from packets.\n");
	printf("Usage: %s [-h] [-c count] [-C file_size] [-G seconds]\n", pro_name);
	printf("          [-i interface | -f file | -d directory]\n");
	printf("          [-w file]\n\n");
	printf("      -h	print this message\n");
	printf("      -c	maximum packets sniffed\n");
	printf("      -C	(Bytes) maximum output file size; new files will be created with -w value plus integer number\n");
	printf("      -G	maximum seconds to sniff\n");
	printf("      -i	interface from which to sniff live packets\n");
	printf("      -f	filename of single pcap file\n");
	printf("      -d	directory of pcap files\n");
	printf("      -w	output file name\n");
}

// Check time and pakcet count limitations
static int check_time_count(void){
	time(&t_now);
	var_seconds = (int)t_now-t_start;
	if(limit_pktcnt != -1 && var_pktcnt>=limit_pktcnt){
		return -1;
	}else if(limit_seconds != -1 && var_seconds>=limit_seconds){
		return -1;
	}else{
		return 0;
	}
}

static int check_filesize(void){
	if (limit_filesize == -1 || var_filesize <= limit_filesize)
		return 0;
	else
		return -1;
}

static int process_packet(const char **pkt, char **packetinfo, int *packetinfosize){
	*packetinfo = "one packet processed\n";
	*packetinfosize = strlen(*packetinfo);
	return 0;
}

int main(int argc, char *argv[]){
	char* interface = NULL;
	char* outputfile = NULL;
	char* inputfile = NULL;
	char* inputdir = NULL;
	int opt;
	while((opt = getopt(argc, argv, "c:C:G:i:f:d:w:h")) != -1)
	{
		switch(opt)
		{
			case 'h':
				print_usage(argv[0]);
				return 0;
			case 'c':
				limit_pktcnt = atoi(optarg);
				break;
			case 'C':
				limit_filesize = atoi(optarg);
				break;
			case 'G':
				limit_seconds = atoi(optarg);
				break;
			case 'i':
				interface = optarg;
				break;
			case 'f':
				inputfile = optarg;
				break;
			case 'd':
				inputdir = optarg;
				break;
			case 'w':
				outputfile = optarg;
				break;
			default:
				print_usage(argv[0]);
				return 0;
		}
	}
	
	if (interface == NULL && inputfile == NULL && inputdir == NULL){
		print_usage(argv[0]);
		exit(-1);
	}

	time(&t_start);		//Mark start time

	//Prepare pcap handler
	pcap_t *cap;
	char errbuf[PCAP_ERRBUF_SIZE];
	memset(errbuf, 0, PCAP_ERRBUF_SIZE);
	if (NULL != inputfile){
		cap = pcap_open_offline(inputfile, errbuf);
		if( cap == NULL){
			printf("%s\n", errbuf);
			exit(-1);
		}
	}else if(NULL != interface){
		cap = pcap_open_live(interface, 65535, 0, 1000, errbuf);
		if( cap == NULL){
			printf("%s\n", errbuf);
			exit(-1);
		}
	}
	
	//Prepare output filenames and file counter
	FILE* outputstream;
	char fname[128];
	int file_cnt = 0;
	if(NULL != outputfile){
		strcpy(fname, outputfile);
		outputstream = fopen(fname, "wb");
	}else{
		strcpy(fname, "stdout");
		outputstream = stdout;
	}
	file_cnt++;
	
	//Prepare Queues: packet queue and flow queue
	Queue __GLOBAL_PACKET_QUEUE, __GLOBAL_FLOW_QUEUE;
	QueueInit(&__GLOBAL_PACKET_QUEUE);
	QueueInit(&__GLOBAL_FLOW_QUEUE);
	
	//Prepare flow hash table
	if(FHTInit() != 0){
		printf("Create flow hash table failed; exit now.");
		exit (-1);
	}
	
	//Do the packet processing work
	int res = 0;
	struct pcap_pkthdr *pkthdr = NULL;
	char *raw = NULL;
	char *packetinfo = NULL;
	int packetinfosize = 0;
	while(0 == check_time_count()){
		res = pcap_next_ex(cap, &pkthdr, &raw);
		if( -1 == res){
			//an error occurred while reading the packet
			continue;
		}else if(0 == res){
			//packets are being read from a live capture, and the timeout expired
			continue;
		}else if(-2 == res){
			printf("Packets are being read from a 'savefile', and there are no more packets to read from the savefile\n");
			return (-1);
		}else{
			int res;
			CaptureProcessRawPacket(raw, pkthdr);
			
			res = CaptureProcessPacketQueue();
			if(res == 0)
				printf("Flow hash table items: %d\n", FHTItemCount());
			
			flow_t *new_flow;
			if(QueueSize(&__GLOBAL_FLOW_QUEUE) > 0){
				new_flow = QueueFront(&__GLOBAL_FLOW_QUEUE);
				FlowPrint(new_flow);
			}
			// process_packet(&raw, &packetinfo, &packetinfosize);
			// if(NULL == packetinfo)
			// 	continue;
			// var_pktcnt++;	// valid packets processed
			// 
			// if(outputstream == stdout){
			// 	printf("stdout message: %s\n", packetinfo);
			// }else{
			// 	var_filesize += packetinfosize;
			// 	if (check_filesize() == 0)
			// 		fwrite(packetinfo, sizeof(char), packetinfosize, outputstream);
			// 	else{
			// 		fflush(outputstream);
			// 		fclose(outputstream);
			// 		sprintf(fname, "%s.%d", outputfile, ++file_cnt);
			// 		outputstream = fopen(fname, "wb");
			// 		fwrite(packetinfo, sizeof(char), packetinfosize, outputstream);
			// 		var_filesize = packetinfosize;
			// 	}
			// }
		}	
	}
	
	printf("%d packets captured, time elapsed %d seconds\n", var_pktcnt, var_seconds);
	return 0;
}

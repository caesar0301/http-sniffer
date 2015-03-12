/*
 * Main.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>  /* getopt() */
#include <netinet/in.h>
#include <pcap.h>

#include "packet.h"
#include "flow.h"
#include "util.h"

int GP_CAP_FIN = 0; /* Flag for offline pcap sniffing */

#ifndef DEBUGGING
#define DEBUGGING 0
void
debugging_print(void){
	while(1){
		if ( GP_CAP_FIN == 1 ){
			break;
		}else{
			packet_queue_print();
			flow_hash_print();
			flow_queue_print();
			sleep(1);
		}
	}
	pthread_exit(NULL);
}
#endif

/**
 * Help function to print usage information.
 */
void
print_usage(const char* pro_name){
	printf("Usage: %s -i interface [-f tracefile] [-o dumpfile]\n", pro_name);
}

/**
 * Parse packets' header information and return a packet_t object
 */
packet_t*
packet_preprocess(const char *raw_data, const struct pcap_pkthdr *pkthdr)
{
	packet_t	*pkt = NULL;	/* new packet */
	char 	*cp = raw_data;
	ethhdr	*eth_hdr = NULL;
	iphdr	*ip_hdr = NULL;
	tcphdr	*tcp_hdr = NULL;

	/* Parse libpcap packet header */
	pkt = packet_new();
	pkt->cap_sec = pkthdr->ts.tv_sec;
	pkt->cap_usec = pkthdr->ts.tv_usec;
	pkt->raw_len = pkthdr->caplen;

	/* Parse ethernet header */
	eth_hdr = packet_parse_ethhdr(cp);
	/* Is IP...? */
	if(eth_hdr->ether_type != 0x0800){
		free_ethhdr(eth_hdr);
		packet_free(pkt);
		return NULL;
	}

	/* Parse IP header */
	cp = cp+sizeof(ethhdr);
	ip_hdr = packet_parse_iphdr(cp);
	pkt->saddr = ip_hdr->saddr;
	pkt->daddr = ip_hdr->daddr;
	pkt->ip_hl = (ip_hdr->ihl) << 2;	/* bytes */
	pkt->ip_tol = ip_hdr->tot_len;
	pkt->ip_pro = ip_hdr->protocol;
	/* Is TCP...? */
	if(pkt->ip_pro != 0x06){
		free_ethhdr(eth_hdr);
		free_iphdr(ip_hdr);
		packet_free(pkt);
		return NULL;
	}

	/* Parse TCP header */
	cp = cp + pkt->ip_hl;
	tcp_hdr = packet_parse_tcphdr(cp);
	pkt->sport = tcp_hdr->th_sport;
	pkt->dport = tcp_hdr->th_dport;
	pkt->tcp_seq = tcp_hdr->th_seq;
	pkt->tcp_ack = tcp_hdr->th_ack;
	pkt->tcp_flags = tcp_hdr->th_flags;
	pkt->tcp_win = tcp_hdr->th_win;
	pkt->tcp_hl = tcp_hdr->th_off << 2;		/* bytes */
	pkt->tcp_dl = pkt->ip_tol - pkt->ip_hl - pkt->tcp_hl;
	pkt->http = 0; /* default */

	/* Check the TCP ports to identify if the packet carries HTTP data
	   We only consider normal HTTP traffic without encryption */
	if( !(tcp_hdr->th_sport == 80 || tcp_hdr->th_dport == 80 || \
		tcp_hdr->th_sport == 8080 || tcp_hdr->th_dport == 8080 || \
		tcp_hdr->th_sport == 8000 || tcp_hdr->th_dport == 8000)){
		free_ethhdr(eth_hdr);
		free_iphdr(ip_hdr);
		free_tcphdr(tcp_hdr);
		packet_free(pkt);
		return NULL;
	}

	/* Process packets of flows which carry HTTP traffic */
	if(pkt->tcp_dl != 0){
		cp = cp + pkt->tcp_hl;
		if( !IsHttpPacket(cp, pkt->tcp_dl) ){
			/* If the packet is not HTTP, we erase the payload. */
			pkt->tcp_odata = NULL;
			pkt->tcp_data = pkt->tcp_odata;
		}else{
			/* Yes, it's HTTP packet */
			char *head_end = NULL;
			int hdl = 0;
			head_end = IsRequest(cp, pkt->tcp_dl);
			if( head_end != NULL ){
				/* First packet of request. */
				hdl = head_end - cp + 1;
				pkt->http = HTTP_REQ;
				/* Fake TCP data length with only HTTP header. */
				pkt->tcp_dl = hdl;
			}
			head_end = IsResponse(cp, pkt->tcp_dl);
			if( head_end != NULL ){
				/* First packet of response. */
				hdl = head_end - cp + 1;
				pkt->http = HTTP_RSP;
				/* Fake TCP data length with only HTTP header. */
				pkt->tcp_dl = hdl;
			}
			/* Allocate memory to store HTTP header. */
			pkt->tcp_odata = MALLOC(char, pkt->tcp_dl + 1);
			pkt->tcp_data = pkt->tcp_odata;
			memset(pkt->tcp_odata, 0, pkt->tcp_dl + 1);
			memcpy(pkt->tcp_odata, cp, pkt->tcp_dl);
		}
	}else{
		pkt->tcp_odata = NULL;
		pkt->tcp_data = pkt->tcp_odata;
	}
	free_ethhdr(eth_hdr);
	free_iphdr(ip_hdr);
	free_tcphdr(tcp_hdr);
	return pkt;
}

/**
 * Fetch a packet from packet queue and add it to any flow.
 */
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

/**
 * Fetch a flow from flow queue and process it
 */
int
process_flow_queue(const char* dump_file){
	flow_t	*flow = NULL;
	char* file_name = NULL;

	while(1){
		file_name = NULL;
		if (dump_file != NULL)
			file_name = dump_file;

		flow = flow_queue_deq();
		if(flow != NULL){
			flow_extract_http(flow);

			if (file_name == NULL){
				/* Generate time-dependent output file */
				static char file_name_buf[64];
				struct tm *timeinfo = NULL;
				time_t raw_time;
				char time_buf[20];

				memset(file_name_buf, 0, sizeof(file_name_buf));
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
		} else if (GP_CAP_FIN == 1) {
			break;
		}
	}
	pthread_exit("Flow processing finished.\n");
	return 0;
}

/**
 * Scrub flow hash table to forcely close dead flows.
 */
void
scrubbing_flow_htbl(void){
	int num = 0;
	while(1){
		sleep(1);
		if (GP_CAP_FIN == 0){
			num = flow_scrubber(60*10);	/* flow timeout in seconds */
		}else{
			num = flow_scrubber(-1); /* cleanse all flows */
			break;
		}
	}
	pthread_exit(NULL);
}

/**
 * Main capture function
 */
int
capture_main(const char* interface, void (*pkt_handler)(void*), int livemode){

	char errbuf[PCAP_ERRBUF_SIZE];
	memset(errbuf, 0, PCAP_ERRBUF_SIZE);
	char *raw = NULL;
	pcap_t *cap = NULL;
	struct pcap_pkthdr pkthdr;
	packet_t *packet = NULL;
	extern int GP_CAP_FIN;

	// printf("%s mode ...\n", livemode==1 ? "Online" : "Offline");

	if ( livemode==1 ) {
		cap = pcap_open_live(interface, 65535, 0, 1000, errbuf);
	} else {
		cap = pcap_open_offline(interface, errbuf);
	}

	if( cap == NULL) {
		printf("%s\n",errbuf); exit(1);
	}

	while(1){
		raw = pcap_next(cap, &pkthdr);
		if( NULL != raw){
			packet = packet_preprocess(raw, &pkthdr);
			if( NULL != packet ){
				pkt_handler(packet);
			}
		} else if ( livemode==0 ) {
			GP_CAP_FIN = 1;
			break;
		}
	}

	if( cap != NULL)
		pcap_close(cap);

	return 0;
}

/**
 * Main portal of http-sniffer
 */
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
		default:
			print_usage(argv[0]); return (1);
		}
	}

	// Check interfaces
	if (interface == NULL && tracefile == NULL){
		print_usage(argv[0]); return (1);
	}

	time_t start, end;
	time(&start);

	void *thread_result;
	pthread_t job_pkt_q;
	pthread_t job_flow_q;
	pthread_t job_scrb_htbl;
#if DEBUGGING == 1
	pthread_t job_debug_p;
#endif

	printf("Http-sniffer started: %s", ctime(&start));

	/* Initialization of packet and flow data structures */
	packet_queue_init();
	flow_init();

	/* Start packet receiving thread */
	pthread_create(&job_pkt_q, NULL, (void*)process_packet_queue, NULL);

	/* Start dead flow cleansing thread */
	pthread_create(&job_scrb_htbl, NULL, (void*)scrubbing_flow_htbl, NULL);

	/* Start flow processing thread */
	pthread_create(&job_flow_q, NULL, (void*)process_flow_queue, dumpfile);

#if DEBUGGING == 1
	pthread_create(&job_debug_p, NULL, (void*)debugging_print, NULL);
#endif

	/* Start main capture in live or offline mode */
	if (interface != NULL)
		capture_main(interface, packet_queue_enq, 1);
	else
		capture_main(tracefile, packet_queue_enq, 0);
	
	// Wait for all threads to finish
	pthread_join(job_pkt_q, &thread_result);
	pthread_join(job_flow_q, &thread_result);
	pthread_join(job_scrb_htbl, &thread_result);

#if DEBUGGING == 1
	pthread_join(job_debug_p, &thread_result);
#endif
	
	time(&end);
	printf("Time elapsed: %d s\n", (int)(end - start));

	return 0;
}
/*
 * tracedump.c
 *
 *  Created on: Mar 31, 2012
 *      Author: chenxm
 *		 Email: chen_xm@sjtu.edu.cn
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>		/* getopt() */
#include <pcap.h>

static char *interface = NULL;
static char *file = NULL;
static int* pkt_counter = 0;
	
void
proc_pkt(u_char* user, const struct pcap_pkthdr *hp, const u_char* packet)
{
	printf("PKT: %d\r", pkt_counter++);
	pcap_dump(user, hp, packet);
}

int 
main(int argc, char *argv[])
{
	if(argc < 2)
	{
		printf("Usage: %s -i interface -f file\n", argv[0]);
		return (1);
	}
	int opt;
	while((opt = getopt(argc, argv, ":i:f:h")) != -1)
	{
		switch(opt)
		{
			case 'h':
				printf("Usage: %s -i interface -f file\n", argv[0]);
				return (1);
			case 'i':
				interface = optarg;
				break;
			case 'f':
				file = optarg;
				break;
			default:	/* '?' */
				printf("Usage: %s -i interface -f file\n", argv[0]);
				return (1);
		}
	}
	
	if(interface == NULL)
	{
		printf("Usage: %s -i interface -f file\n", argv[0]);
		return (1);
	}
	else if(file == NULL)
	{
		file = "trace.pcap";
	}
	
	printf("Beginning capturing...\n");
	
	char errbuf[PCAP_ERRBUF_SIZE];
	memset(errbuf, 0, PCAP_ERRBUF_SIZE);
	pcap_t *cap = NULL;
	struct pcap_pkthdr pkthdr;
	
	cap = pcap_open_live(interface, 65535, 0, 1000, errbuf);
	if( cap == NULL)
	{
		printf("%s\r",errbuf);
		exit(1);
	}
	
	pcap_dumper_t* dumper;
	dumper = pcap_dump_open(cap, file);
	pcap_loop(cap, -1, proc_pkt, dumper);
	pcap_dump_close(dumper);
	pcap_close(cap);
	return 0;
}

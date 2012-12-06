/* capture.h */
#ifndef __CAPTURE_H__
#define __CAPTURE_H__

#include "pcap.h"
#include "queue.h"

int CaptureProcessPacket(const char *raw_data, const struct pcap_pkthdr *pkthdr, Queue *packet_queue);


#endif /* __CAPTURE_H__ */
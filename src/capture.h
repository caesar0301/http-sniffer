/* capture.h */
#ifndef __CAPTURE_H__
#define __CAPTURE_H__

#include "pcap.h"
#include "queue.h"

int CaptureProcessRawPacket(const char *raw_data, const struct pcap_pkthdr *pkthdr);
int CaptureProcessPacketQueue(void);

#endif /* __CAPTURE_H__ */
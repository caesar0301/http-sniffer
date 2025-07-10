extern "C" {
#include "packet.h"
#include "util.h"
}
#include <gtest/gtest.h>
#include <arpa/inet.h>

class PacketTest : public ::testing::Test {
protected:
    void SetUp() override {
        packet_queue_init();
    }
    
    void TearDown() override {
        packet_queue_clr();
    }
};

TEST_F(PacketTest, PacketCreation) {
    packet_t* pkt = packet_new();
    
    ASSERT_NE(pkt, nullptr);
    EXPECT_EQ(pkt->next, nullptr);
    EXPECT_EQ(pkt->tcp_odata, nullptr);
    EXPECT_EQ(pkt->tcp_data, nullptr);
    
    packet_free(pkt);
}

TEST_F(PacketTest, PacketQueue) {
    // Test queue initialization
    EXPECT_EQ(packet_queue_len(), 0u);
    
    // Create test packets
    packet_t* pkt1 = packet_new();
    packet_t* pkt2 = packet_new();
    
    pkt1->saddr = 0x12345678;
    pkt1->daddr = 0x87654321;
    pkt1->sport = 8080;
    pkt1->dport = 80;
    
    pkt2->saddr = 0x11111111;
    pkt2->daddr = 0x22222222;
    pkt2->sport = 9090;
    pkt2->dport = 443;
    
    // Test enqueue
    EXPECT_TRUE(packet_queue_enq(pkt1));
    EXPECT_EQ(packet_queue_len(), 1u);
    
    EXPECT_TRUE(packet_queue_enq(pkt2));
    EXPECT_EQ(packet_queue_len(), 2u);
    
    // Test dequeue
    packet_t* dequeued1 = packet_queue_deq();
    ASSERT_NE(dequeued1, nullptr);
    EXPECT_EQ(dequeued1->saddr, 0x12345678u);
    EXPECT_EQ(packet_queue_len(), 1u);
    
    packet_t* dequeued2 = packet_queue_deq();
    ASSERT_NE(dequeued2, nullptr);
    EXPECT_EQ(dequeued2->saddr, 0x11111111u);
    EXPECT_EQ(packet_queue_len(), 0u);
    
    // Test empty queue
    packet_t* empty = packet_queue_deq();
    EXPECT_EQ(empty, nullptr);
    
    // Cleanup
    packet_free(dequeued1);
    packet_free(dequeued2);
}

TEST_F(PacketTest, EthernetHeaderParsing) {
    // Create a mock ethernet header
    unsigned char eth_data[14] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, // dest MAC
        0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, // src MAC
        0x08, 0x00  // type (IP)
    };
    
    ethhdr* eth = packet_parse_ethhdr((const char*)eth_data);
    ASSERT_NE(eth, nullptr);
    
    EXPECT_EQ(eth->ether_type, 0x0800);
    
    free_ethhdr(eth);
}

TEST_F(PacketTest, IPHeaderParsing) {
    // Create a mock IP header (minimal 20 bytes)
    unsigned char ip_data[20] = {
        0x45, 0x00, // version (4), IHL (5), TOS (0)
        0x00, 0x28, // total length (40 bytes)
        0x12, 0x34, // identification
        0x40, 0x00, // flags (DF), fragment offset (0)
        0x40, 0x06, // TTL (64), protocol (TCP=6)
        0x00, 0x00, // checksum (dummy)
        0xC0, 0xA8, 0x01, 0x01, // source IP (192.168.1.1)
        0xC0, 0xA8, 0x01, 0x02  // dest IP (192.168.1.2)
    };
    
    iphdr* ip = packet_parse_iphdr((const char*)ip_data);
    ASSERT_NE(ip, nullptr);
    
    EXPECT_EQ(ip->version, 4);
    EXPECT_EQ(ip->ihl, 5);
    EXPECT_EQ(ip->protocol, 6); // TCP
    
    free_iphdr(ip);
}

TEST_F(PacketTest, TCPHeaderParsing) {
    // Create a mock TCP header (minimal 20 bytes)
    unsigned char tcp_data[20] = {
        0x1F, 0x90, // source port (8080)
        0x00, 0x50, // dest port (80)
        0x12, 0x34, 0x56, 0x78, // sequence number
        0x87, 0x65, 0x43, 0x21, // ack number
        0x50, 0x18, // data offset (5), flags (PSH|ACK)
        0x20, 0x00, // window size
        0x00, 0x00, // checksum (dummy)
        0x00, 0x00  // urgent pointer
    };
    
    tcphdr* tcp = packet_parse_tcphdr((const char*)tcp_data);
    ASSERT_NE(tcp, nullptr);
    
    EXPECT_EQ(tcp->th_sport, 8080);
    EXPECT_EQ(tcp->th_dport, 80);
    EXPECT_EQ(tcp->th_off, 5);
    EXPECT_EQ(tcp->th_flags, 0x18); // PSH|ACK
    
    free_tcphdr(tcp);
}
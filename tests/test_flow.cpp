extern "C" {
#include "flow.h"
#include "packet.h"
#include "util.h"
}
#include <gtest/gtest.h>

class FlowTest : public ::testing::Test {
protected:
    void SetUp() override {
        flow_init();
        flow_hash_init();
        packet_queue_init();
    }
    
    void TearDown() override {
        flow_hash_clear();
        flow_queue_clear();
        packet_queue_clr();
    }
};

TEST_F(FlowTest, FlowCreation) {
    flow_t* flow = flow_new();
    
    ASSERT_NE(flow, nullptr);
    EXPECT_EQ(flow->next, nullptr);
    EXPECT_EQ(flow->prev, nullptr);
    EXPECT_EQ(flow->pkt_src_f, nullptr);
    EXPECT_EQ(flow->pkt_src_e, nullptr);
    EXPECT_EQ(flow->pkt_dst_f, nullptr);
    EXPECT_EQ(flow->pkt_dst_e, nullptr);
    EXPECT_EQ(flow->pkt_src_n, 0u);
    EXPECT_EQ(flow->pkt_dst_n, 0u);
    EXPECT_EQ(flow->http_f, nullptr);
    EXPECT_EQ(flow->http_e, nullptr);
    EXPECT_EQ(flow->http_cnt, 0u);
    EXPECT_FALSE(flow->http);
    
    flow_free(flow);
}

TEST_F(FlowTest, FlowSocketComparison) {
    flow_s socket1, socket2, socket3;
    
    // Same socket
    socket1.saddr = 0x12345678;
    socket1.daddr = 0x87654321;
    socket1.sport = 8080;
    socket1.dport = 80;
    
    socket2.saddr = 0x12345678;
    socket2.daddr = 0x87654321;
    socket2.sport = 8080;
    socket2.dport = 80;
    
    EXPECT_EQ(flow_socket_cmp(&socket1, &socket2), 0);
    
    // Different socket
    socket3.saddr = 0x11111111;
    socket3.daddr = 0x22222222;
    socket3.sport = 9090;
    socket3.dport = 443;
    
    EXPECT_NE(flow_socket_cmp(&socket1, &socket3), 0);
}

TEST_F(FlowTest, FlowPacketAddition) {
    // Create socket for flow creation
    flow_s socket;
    socket.saddr = 0x12345678;
    socket.daddr = 0x87654321;
    socket.sport = 8080;
    socket.dport = 80;
    
    // Create flow through hash table system (this properly initializes hmb)
    flow_t* flow = flow_hash_new(socket);
    ASSERT_NE(flow, nullptr);
    
    packet_t* pkt1 = packet_new();
    packet_t* pkt2 = packet_new();
    
    // Set up packet data
    pkt1->saddr = 0x12345678;
    pkt1->daddr = 0x87654321;
    pkt1->sport = 8080;
    pkt1->dport = 80;
    pkt1->tcp_dl = 100;
    pkt1->tcp_flags = 0; // No special flags
    pkt1->http = 0;      // No HTTP payload
    
    pkt2->saddr = 0x87654321;
    pkt2->daddr = 0x12345678;
    pkt2->sport = 80;
    pkt2->dport = 8080;
    pkt2->tcp_dl = 200;
    pkt2->tcp_flags = 0; // No special flags  
    pkt2->http = 0;      // No HTTP payload
    
    // Add packets to flow - these will be freed by flow_add_packet
    EXPECT_EQ(flow_add_packet(flow, pkt1, TRUE), 0);  // source packet
    EXPECT_EQ(flow->pkts_src, 1u);
    EXPECT_EQ(flow->payload_src, 100u);
    
    EXPECT_EQ(flow_add_packet(flow, pkt2, FALSE), 0); // destination packet
    EXPECT_EQ(flow->pkts_dst, 1u);
    EXPECT_EQ(flow->payload_dst, 200u);
    
    // Clean up - flow is still in hash table, need to remove it properly
    flow_t* removed_flow = flow_hash_delete(flow);
    flow_free(removed_flow);
}

TEST_F(FlowTest, FlowHashOperations) {
    // Test initial state
    EXPECT_EQ(flow_hash_fcnt(), 0);
    EXPECT_EQ(flow_hash_size(), 10007); // Hash table size is constant
    
    // Create a flow socket
    flow_s socket;
    socket.saddr = 0x12345678;
    socket.daddr = 0x87654321;
    socket.sport = 8080;
    socket.dport = 80;
    
    // Create new flow in hash table
    flow_t* flow = flow_hash_new(socket);
    ASSERT_NE(flow, nullptr);
    EXPECT_GT(flow_hash_fcnt(), 0);
    
    // Find existing flow
    flow_t* found = flow_hash_find(socket);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found, flow);
    
    // Try to find non-existent flow
    flow_s other_socket;
    other_socket.saddr = 0x11111111;
    other_socket.daddr = 0x22222222;
    other_socket.sport = 9090;
    other_socket.dport = 443;
    
    flow_t* not_found = flow_hash_find(other_socket);
    EXPECT_EQ(not_found, nullptr);
    
    // Delete flow
    flow_t* deleted = flow_hash_delete(flow);
    EXPECT_NE(deleted, nullptr);
    
    // Verify flow is no longer findable
    flow_t* should_be_null = flow_hash_find(socket);
    EXPECT_EQ(should_be_null, nullptr);
    
    // Clean up
    flow_free(deleted);
}

TEST_F(FlowTest, FlowQueue) {
    // Test initial state
    EXPECT_EQ(flow_queue_len(), 0);
    
    // Create test flows
    flow_t* flow1 = flow_new();
    flow_t* flow2 = flow_new();
    
    flow1->socket.saddr = 0x12345678;
    flow2->socket.saddr = 0x87654321;
    
    // Test enqueue
    EXPECT_EQ(flow_queue_enq(flow1), 0);
    EXPECT_EQ(flow_queue_len(), 1);
    
    EXPECT_EQ(flow_queue_enq(flow2), 0);
    EXPECT_EQ(flow_queue_len(), 2);
    
    // Test dequeue
    flow_t* dequeued1 = flow_queue_deq();
    ASSERT_NE(dequeued1, nullptr);
    EXPECT_EQ(dequeued1->socket.saddr, 0x12345678u);
    EXPECT_EQ(flow_queue_len(), 1);
    
    flow_t* dequeued2 = flow_queue_deq();
    ASSERT_NE(dequeued2, nullptr);
    EXPECT_EQ(dequeued2->socket.saddr, 0x87654321u);
    EXPECT_EQ(flow_queue_len(), 0);
    
    // Test empty queue
    flow_t* empty = flow_queue_deq();
    EXPECT_EQ(empty, nullptr);
    
    // Cleanup
    flow_free(dequeued1);
    flow_free(dequeued2);
}
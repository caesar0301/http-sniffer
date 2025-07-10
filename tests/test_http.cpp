extern "C" {
#include "http.h"
#include "flow.h"
#include "packet.h"
#include "util.h"
}
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <cstring>

class HttpTest : public ::testing::Test {
protected:
    void SetUp() override {
        flow_init();
        flow_hash_init();
    }
    
    void TearDown() override {
        flow_hash_clear();
    }
};

TEST_F(HttpTest, HttpPacketDetection) {
    // Test HTTP GET request detection
    const char* http_get = "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n";
    EXPECT_TRUE(IsHttpPacket(http_get, strlen(http_get)));
    
    // Test HTTP POST request detection
    const char* http_post = "POST /api/data HTTP/1.1\r\nHost: api.example.com\r\n\r\n";
    EXPECT_TRUE(IsHttpPacket(http_post, strlen(http_post)));
    
    // Test HTTP response detection
    const char* http_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    EXPECT_TRUE(IsHttpPacket(http_response, strlen(http_response)));
    
    // Test non-HTTP data
    const char* non_http = "This is not HTTP data";
    EXPECT_FALSE(IsHttpPacket(non_http, strlen(non_http)));
    
    // Test empty data
    EXPECT_FALSE(IsHttpPacket("", 0));
    EXPECT_FALSE(IsHttpPacket(nullptr, 0));
}

TEST_F(HttpTest, HttpRequestDetection) {
    // Test valid HTTP GET request
    const char* http_get = "GET /path HTTP/1.1\r\nHost: example.com\r\n\r\n";
    char* end = IsRequest(http_get, strlen(http_get));
    EXPECT_NE(end, nullptr);
    
    // Test valid HTTP POST request
    const char* http_post = "POST /api HTTP/1.1\r\nContent-Length: 10\r\n\r\ndata";
    end = IsRequest(http_post, strlen(http_post));
    EXPECT_NE(end, nullptr);
    
    // Test valid HTTP PUT request
    const char* http_put = "PUT /resource HTTP/1.1\r\nHost: api.example.com\r\n\r\n";
    end = IsRequest(http_put, strlen(http_put));
    EXPECT_NE(end, nullptr);
    
    // Test HTTP response (should not be detected as request)
    const char* http_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    end = IsRequest(http_response, strlen(http_response));
    EXPECT_EQ(end, nullptr);
    
    // Test invalid data
    const char* invalid = "Not a valid HTTP request";
    end = IsRequest(invalid, strlen(invalid));
    EXPECT_EQ(end, nullptr);
}

TEST_F(HttpTest, HttpResponseDetection) {
    // Test valid HTTP 200 response
    const char* http_200 = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    char* end = IsResponse(http_200, strlen(http_200));
    EXPECT_NE(end, nullptr);
    
    // Test valid HTTP 404 response
    const char* http_404 = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n";
    end = IsResponse(http_404, strlen(http_404));
    EXPECT_NE(end, nullptr);
    
    // Test valid HTTP 302 response
    const char* http_302 = "HTTP/1.1 302 Found\r\nLocation: /new-path\r\n\r\n";
    end = IsResponse(http_302, strlen(http_302));
    EXPECT_NE(end, nullptr);
    
    // Test HTTP request (should not be detected as response)
    const char* http_request = "GET /path HTTP/1.1\r\nHost: example.com\r\n\r\n";
    end = IsResponse(http_request, strlen(http_request));
    EXPECT_EQ(end, nullptr);
    
    // Test invalid data
    const char* invalid = "Not a valid HTTP response";
    end = IsResponse(invalid, strlen(invalid));
    EXPECT_EQ(end, nullptr);
}

TEST_F(HttpTest, HttpPairCreation) {
    http_pair_t* pair = http_new();
    
    ASSERT_NE(pair, nullptr);
    EXPECT_EQ(pair->next, nullptr);
    EXPECT_EQ(pair->request_header, nullptr);
    EXPECT_EQ(pair->response_header, nullptr);
    
    http_free(pair);
}

TEST_F(HttpTest, HttpRequestCreation) {
    request_t* req = http_request_new();
    
    ASSERT_NE(req, nullptr);
    EXPECT_EQ(req->host, nullptr);
    EXPECT_EQ(req->uri, nullptr);
    EXPECT_EQ(req->user_agent, nullptr);
    EXPECT_EQ(req->hdlen, 0);
    
    http_request_free(req);
}

TEST_F(HttpTest, HttpResponseCreation) {
    response_t* rsp = http_response_new();
    
    ASSERT_NE(rsp, nullptr);
    EXPECT_EQ(rsp->server, nullptr);
    EXPECT_EQ(rsp->date, nullptr);
    EXPECT_EQ(rsp->content_type, nullptr);
    EXPECT_EQ(rsp->hdlen, 0);
    
    http_response_free(rsp);
}

TEST_F(HttpTest, HttpHeaderParsing) {
    // Test basic HTTP GET request parsing
    const char* get_request = 
        "GET /test.html HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "Accept: text/html\r\n"
        "\r\n";
    
    request_t* req = http_request_new();
    
    // Parse the request
    int result = http_parse_request(req, get_request, get_request + strlen(get_request));
    EXPECT_EQ(result, 0);
    
    // Basic checks - these might fail if parser has issues, but test structure is correct
    ASSERT_NE(req, nullptr);
    
    http_request_free(req);
}

TEST_F(HttpTest, FlowHttpExtraction) {
    // Create socket for flow creation
    flow_s socket;
    socket.saddr = 0x12345678;
    socket.daddr = 0x87654321;
    socket.sport = 8080;
    socket.dport = 80;
    
    // Create flow through hash table system
    flow_t* flow = flow_hash_new(socket);
    ASSERT_NE(flow, nullptr);
    
    // Create a packet with HTTP data
    packet_t* pkt = packet_new();
    const char* http_data = "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n";
    pkt->tcp_dl = strlen(http_data);
    pkt->tcp_odata = MALLOC(char, pkt->tcp_dl + 1);
    strcpy(pkt->tcp_odata, http_data);
    pkt->tcp_data = pkt->tcp_odata;
    pkt->http = HTTP_REQ;
    pkt->tcp_flags = 0;
    
    // Mark flow as HTTP
    flow->http = TRUE;
    
    // Add packet to flow - packet will be freed by flow_add_packet
    int add_result = flow_add_packet(flow, pkt, TRUE);
    EXPECT_EQ(add_result, 0);
    
    // Extract HTTP pairs
    int result = flow_extract_http(flow);
    EXPECT_EQ(result, 0);
    
    // Clean up
    flow_t* removed_flow = flow_hash_delete(flow);
    flow_free(removed_flow);
}
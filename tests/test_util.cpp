extern "C" {
#include "util.h"
}
#include <gtest/gtest.h>
#include <cstring>
#include <arpa/inet.h>

class UtilTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }
    
    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(UtilTest, MemoryAllocation) {
    // Test memory allocation
    int* int_ptr = MALLOC(int, 10);
    ASSERT_NE(int_ptr, nullptr);
    
    // Test that memory is writable
    for (int i = 0; i < 10; i++) {
        int_ptr[i] = i;
    }
    
    // Verify values
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(int_ptr[i], i);
    }
    
    free(int_ptr);
}

TEST_F(UtilTest, IPAddressConversion) {
    // Test network to string conversion
    u_int32_t ip_net = htonl(0xC0A80101); // 192.168.1.1 in network byte order
    char* ip_str = ip_ntos(ip_net);
    
    ASSERT_NE(ip_str, nullptr);
    EXPECT_STREQ(ip_str, "192.168.1.1");
    
    free(ip_str);
}

TEST_F(UtilTest, IPStringToNetwork) {
    // Test string to network conversion
    char ip_str[] = "10.0.0.1";
    u_int32_t ip_net = ip_ston(ip_str);
    
    // Convert back to verify
    char* back_to_str = ip_ntos(ip_net);
    ASSERT_NE(back_to_str, nullptr);
    EXPECT_STREQ(back_to_str, "10.0.0.1");
    
    free(back_to_str);
}

TEST_F(UtilTest, BooleanMacros) {
    // Test boolean macro definitions
    EXPECT_EQ(TRUE, 1);
    EXPECT_EQ(FALSE, 0);
    
    BOOL test_bool = TRUE;
    EXPECT_TRUE(test_bool);
    
    test_bool = FALSE;
    EXPECT_FALSE(test_bool);
}

TEST_F(UtilTest, LargeMemoryAllocation) {
    // Test larger memory allocation
    char* buffer = MALLOC(char, 1024);
    ASSERT_NE(buffer, nullptr);
    
    // Fill with pattern
    memset(buffer, 0xAA, 1024);
    
    // Verify pattern
    for (int i = 0; i < 1024; i++) {
        EXPECT_EQ((unsigned char)buffer[i], 0xAA);
    }
    
    free(buffer);
}
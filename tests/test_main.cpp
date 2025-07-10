#include <gtest/gtest.h>
#include <glog/logging.h>

int main(int argc, char **argv) {
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize Google Logging
    google::InitGoogleLogging(argv[0]);
    google::SetStderrLogging(google::GLOG_INFO);
    
    // Run tests
    int result = RUN_ALL_TESTS();
    
    // Cleanup
    google::ShutdownGoogleLogging();
    
    return result;
}
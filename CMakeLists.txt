cmake_minimum_required(VERSION 3.10)
project(http-sniffer C CXX)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w -g")

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Find Google Test (optional for unit tests)
find_package(GTest QUIET)
# Find Google Logging (optional for unit tests)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(GLOG QUIET libglog)
endif()

if(GTest_FOUND AND GLOG_FOUND)
    message(STATUS "Google Test and Google Logging found - unit tests enabled")
    set(ENABLE_TESTS ON)
else()
    message(STATUS "Google Test or Google Logging not found - unit tests will be disabled")
    set(ENABLE_TESTS OFF)
endif()

# Find pcap library
find_library(PCAP_LIBRARY pcap)
if(NOT PCAP_LIBRARY)
    message(FATAL_ERROR "pcap library not found. Please install libpcap-dev")
endif()

# Find json-c library
find_library(JSON_C_LIBRARY json-c)
if(NOT JSON_C_LIBRARY)
    message(FATAL_ERROR "json-c library not found. Please install libjson-c-dev")
endif()

# Find json-c headers
find_path(JSON_C_INCLUDE_DIR json-c/json.h
    PATHS
    /usr/include
    /usr/local/include
    /opt/homebrew/include
    /opt/local/include
)

if(NOT JSON_C_INCLUDE_DIR)
    message(FATAL_ERROR "json-c headers not found. Please install libjson-c-dev")
endif()

# Check for NFM libraries (optional, controlled by debug flag)
option(ENABLE_NFM "Enable NFM libraries for network processor card driver" OFF)

if(ENABLE_NFM)
    message(STATUS "NFM enabled.")
    set(NFM_LIBRARIES 
        nfm 
        nfm_framework 
        nfm_error 
        nfm_packet 
        nfm_rules 
        nfm_platform 
        nfe 
        nfp
    )
    
    # Add NFM include and library paths
    list(APPEND CMAKE_PREFIX_PATH "/opt/netronome")
    include_directories("/opt/netronome/nfm/include")
    link_directories("/opt/netronome/lib")
endif()

# Set include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${JSON_C_INCLUDE_DIR})

# Collect all source files
file(GLOB SOURCES "src/*.c")

# Remove main.c from sources for the library
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/src/main.c")

# Create static library from C sources (excluding main.c)
add_library(http-sniffer-lib STATIC ${SOURCES})

# Link libraries to the static library
target_link_libraries(http-sniffer-lib 
    ${PCAP_LIBRARY}
    ${JSON_C_LIBRARY}
    Threads::Threads
)

# Add NFM libraries if enabled
if(ENABLE_NFM)
    target_link_libraries(http-sniffer-lib ${NFM_LIBRARIES})
endif()

# Create executable with just main.c
add_executable(http-sniffer "src/main.c")

# Link the executable against the static library
target_link_libraries(http-sniffer http-sniffer-lib)

# Set output directory
set_target_properties(http-sniffer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

# Unit tests
if(ENABLE_TESTS)
    enable_testing()
    
    # Collect test source files
    file(GLOB TEST_SOURCES "tests/*.cpp")
    
    # Create test executable (C++ for Google Test)
    add_executable(unit-tests ${TEST_SOURCES})
    set_target_properties(unit-tests PROPERTIES
        LINKER_LANGUAGE CXX
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED ON
    )
    
    # Link test libraries
    set(TEST_LIBS
        http-sniffer-lib
        GTest::gtest
        GTest::gtest_main
    )
    if(GLOG_FOUND)
        list(APPEND TEST_LIBS ${GLOG_LIBRARIES})
    endif()
    target_link_libraries(unit-tests ${TEST_LIBS})
    
    # Include directories for tests
    target_include_directories(unit-tests PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
        ${JSON_C_INCLUDE_DIR}
    )
    if(GLOG_FOUND)
        target_include_directories(unit-tests PRIVATE ${GLOG_INCLUDE_DIRS})
    endif()
    
    # Set output directory for tests
    set_target_properties(unit-tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    )
    
    # Add test
    add_test(NAME UnitTests COMMAND unit-tests)
    
    message(STATUS "Unit tests enabled")
else()
    message(STATUS "Unit tests disabled - missing dependencies")
endif()

# Print status messages
message(STATUS "Checking pthread ... true")
message(STATUS "Checking pcap ... true")
message(STATUS "Checking json-c ... true")

if(ENABLE_NFM)
    message(STATUS "NFM libraries enabled")
endif() 
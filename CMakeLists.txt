cmake_minimum_required(VERSION 3.10)
project(http-sniffer C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w -g")

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

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

# Create executable
add_executable(http-sniffer ${SOURCES})

# Link libraries
target_link_libraries(http-sniffer 
    ${PCAP_LIBRARY}
    ${JSON_C_LIBRARY}
    Threads::Threads
)

# Add NFM libraries if enabled
if(ENABLE_NFM)
    target_link_libraries(http-sniffer ${NFM_LIBRARIES})
endif()

# Set output directory
set_target_properties(http-sniffer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

# Print status messages
message(STATUS "Checking pthread ... true")
message(STATUS "Checking pcap ... true")
message(STATUS "Checking json-c ... true")

if(ENABLE_NFM)
    message(STATUS "NFM libraries enabled")
endif() 
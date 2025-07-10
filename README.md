# http-sniffer

A multi-threading tool to sniff HTTP header records beyond TCP flow statistics.

**MIT licensed.**

## Features

* Live network interface and offline PCAP file capture
* Multi-threading for high-performance traffic analysis
* TCP flow statistics export
* HTTP request/response pair extraction
* JSON and CSV output formats

## Quick Start

```bash
# Install dependencies
sudo apt-get install cmake libpcap-dev libjson-c-dev build-essential  # Ubuntu/Debian
brew install cmake libpcap json-c  # macOS

# Build and run
make
./bin/http-sniffer -i <interface>
```

## Build

```bash
make                # Standard build
make debug          # Debug build
make nfm            # With NFM support
make clean-build    # Clean then build
make test           # Run unit tests
make test-debug     # Build debug and run tests
```

## Usage

```bash
# Live capture
./bin/http-sniffer -i en0

# PCAP file analysis
./bin/http-sniffer -r capture.pcap

# Save to JSON
./bin/http-sniffer -i en0 -o output.json
```

## Output

### CSV Format
```csv
[20120921 16:40:09]10.187.179.28:53196-->180.149.134.229:80 1335164797.208360 0.0 0.0 167 5/3 0/0 0 0
```

### JSON Format
```json
{
    "t_r": "2025-07-10T11:25:05",
    "sa": "1.2.3.4",
    "da": "4.3.2.1",
    "sp": 54180,
    "dp": 80,
    "synt": 1752117904.8173649,
    "fbt": 1752117904.830142,
    "lbt": 1752117904.8638189,
    "rtt": 162,
    "spkts": 9,
    "dpkts": 24,
    "spl": 52,
    "dpl": 30028,
    "fc": 0,
    "pcnt": 1,
    "pairs": [
        {
            "req": {
                "fbt": 1752117904.830142,
                "lbt": 1752117904.830142,
                "totlen": 52,
                "bdylen": 0,
                "ver": 1,
                "mth": "GET",
                "host": "www.baidu.com",
                "uri": "\/",
                "accept": "*\/*"
            },
            "res": {
                "fbt": 1752117904.8458209,
                "lbt": 1752117904.863394,
                "totlen": 30497,
                "bdylen": 29506,
                "ver": 1,
                "sta": 200,
                "server": "BWS\/1.1",
                "dat": "Thu, 10 Jul 2025 03:25:04 GMT",
                "accept_ranges": "bytes",
                "contyp": "text\/html",
                "conlen": "29506"
            }
        }
    ]
}
```

## Dependencies

* [libpcap](http://www.tcpdump.org/) - Packet capture
* [json-c](https://github.com/json-c/json-c) - JSON parsing
* [CMake](https://cmake.org/) - Build system
* [Google Test](https://github.com/google/googletest) - Unit testing (optional)

**Note:** If you encounter `json-c` unused params errors on Linux, try the [alternative version](https://github.com/phalcon/json-c).

## Author

Xiaming Chen <chenxm35@gmail.com>  
SJTU, Shanghai, China
2012-04-01

http-sniffer
==========

[![Build Status](https://travis-ci.org/caesar0301/http-sniffer.svg)](https://travis-ci.org/caesar0301/http-sniffer)

A multi-threading tool to sniff HTTP header records beyond TCP flow statistics.

**MIT licensed.**

Features
------------

* Support both offline PCAP file and live NIC sniffing
* Perform multi-threading process, whereby the tool gets higher performance in face of big tarffic volume
* Export statistics of TCP flows
* Export HTTP request/response pairs if they are present in the TCP flow
* Support JSON format output


Dependencis
------------

The `http-sniffer` depends on other utilities:

  * [libpcap](http://www.tcpdump.org/) to extract traffic packet,
  * [json-c](https://github.com/json-c/json-c) to support json parsing, and
  * [scons](http://www.scons.org/) to build the project.


A known issue about `json-c` is the unused params errors on linux platform,
you can use an [alternative](https://github.com/phalcon/json-c) to solve it.
You may also need to export the library path where `libjson-c.*` locate, by

    export LD_LIBRARY_PATH=/usr/local/lib


How to Use
----------

Run `scons` in root folder to compile:

    $ cd http-sniffer
    $ scons

Get your live interface with `ifconfig` in terminal, e.g. `en0`, then

    $ ./bin/http-sniffer -i en0

Or store output flows as json

    $ ./bin/http-sniffer -i en0 -o output.json


Output
------

* In brief CSV format:

```csv
[20120921 16:40:09]10.187.179.28:53196-->180.149.134.229:80 1335164797.208360 0.0 0.0 167 5/3 0/0 0 0
[20120921 16:40:09]10.187.179.28:53160-->58.63.234.206:80 1335164789.893109 0.0 0.0 21 4/2 0/0 0 0
[20120921 16:40:09]10.187.179.28:53161-->58.63.234.206:80 1335164789.893219 0.0 0.0 225 4/2 0/0 0 0
[20120921 16:40:09]10.187.179.28:53158-->58.63.234.198:80 1335164789.769004 0.0 0.0 118 4/2 0/0 0 0
[20120921 16:40:09]10.187.179.28:53164-->113.108.216.252:80 1335164790.179680 0.0 0.0 12 4/2 0/0 0 0
[20120921 16:40:09]10.187.179.28:53189-->180.149.134.221:80 1335164797.961918 0.0 0.0 111 3/1 0/0 0 0
```

* In full JSON format: each line records **one** TCP flow with piggybacked HTTP messages, e.g.

```json
{
   "time_local": "2012-04-23T13:50:09",
   "saddr": "192.168.1.4", 
   "daddr": "192.168.1.5", 
   "sport": 45753, 
   "dport": 80, 
   "time_syn": 1335160209.417475, 
   "time_first_byte": 1335160209.452336, 
   "time_last_byte": 1335160209.488276, 
   "rtt": 248, 
   "src_packets": 4, 
   "dst_packets": 4, 
   "src_bytes": 521, 
   "dst_bytes": 257, 
   "http_pair_count": 1, 
   "force_closed": 0, 
   "http_pairs": [ 
    { 
     "request": { 
      "time_first_byte": 1335160209.452336, 
      "time_last_byte": 1335160209.452336, 
      "bytes_transfered": 521, 
      "http_version": 1, 
      "method": 1, 
      "host": "s1.bdstatic.com", 
      "uri": "\/r\/www\/img\/i-1.0.0.png", 
      "referer": "http:\/\/www.baidu.com\/", 
      "user_agent": "Mozilla\/5.0", 
      "accept": "image\/png,image\/*;q=0.8,*\/*;q=0.5", 
      "accept_encoding": "gzip,deflate", 
      "accept_language": "en-us,en;q=0.5", 
      "accept_charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7"}, 
     "response": {
      "time_first_byte": 1335160209.488260, 
      "time_last_byte": 1335160209.488260, 
      "bytes_transfered": 257, 
      "http_version": 1, 
      "status": 304, 
      "server": "JSP\/1.0.18", 
      "date": "Mon, 23 Apr 2012 06:02:23 GMT", 
      "expires": "Tue, 29 Mar 2022 09:34:06 GMT", 
      "etag": "\"25f-4a6ebc21c42c0\"", 
      "last_modified": "Thu, 30 Jun 2011 10:56:51 GMT"} 
    }] 
}
```


About Author
------------

Xiaming Chen <chen@xiaming.me>

SJTU, Shanghai, China

2012-04-01

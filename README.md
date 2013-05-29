http-sniffer
==========

A tool to sniff HTTP header records beyond TCP flow info. 

Each line represents a TCP flow and its HTTP messages in json format.

Support both live capturing or offline trace.

Use ./bin/weblogger -h to get more usage info.



About Author
------------

Xiaming Chen, SJTU, Shanghai, China

chenxm35@gmail.com

2012-04-01




Dependencis
------------

libpcap, scons, json-c



How to Use
------------

* Run 'scons' to compile the program: debug=0 for normal usage; debug=1 for support NFM library.
* The excutive files lie in foder bin/.



Output Data Format
------------

Each line (like below) is encoded by JSON format which convinient for furture processing.

   { "time_local": "20120423 13:50:09", 
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
    } ] 
   }

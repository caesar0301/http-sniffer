/*
 * http.h
 *
 *  Created on: Jun 17, 2011
 *      Author: chenxm
 */
#ifndef __HTTP_H__
#define __HTTP_H__

#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include "queue.h"

// HTTP response status number.
typedef enum _http_status http_status;
enum _http_status
{
    HTTP_ST_100=100,   /**< Continue */
    HTTP_ST_101,       /**< Switching Protocols */
    HTTP_ST_102,       /**< Processing */
    HTTP_ST_199=199,   /**< Informational - Others */

    HTTP_ST_200,       /**< OK */
    HTTP_ST_201,       /**< Created */
    HTTP_ST_202,       /**< Accepted */
    HTTP_ST_203,       /**< Non-authoritative Information */
    HTTP_ST_204,       /**< No Content */
    HTTP_ST_205,       /**< Reset Content */
    HTTP_ST_206,       /**< Partial Content */
    HTTP_ST_207,       /**< Multi-Status */
    HTTP_ST_299=299,   /**< Success - Others */

    HTTP_ST_300,       /**< Multiple Choices */
    HTTP_ST_301,       /**< Moved Permanently */
    HTTP_ST_302,       /**< Found */
    HTTP_ST_303,       /**< See Other */
    HTTP_ST_304,       /**< Not Modified */
    HTTP_ST_305,       /**< Use Proxy */
    HTTP_ST_307,       /**< Temporary Redirect */
    HTTP_ST_399=399,   /**< Redirection - Others */

    HTTP_ST_400,       /**< Bad Request */
    HTTP_ST_401,       /**< Unauthorized */
    HTTP_ST_402,       /**< Payment Required */
    HTTP_ST_403,       /**< Forbidden */
    HTTP_ST_404,       /**< Not Found */
    HTTP_ST_405,       /**< Method Not Allowed */
    HTTP_ST_406,       /**< Not Acceptable */
    HTTP_ST_407,       /**< Proxy Authentication Required */
    HTTP_ST_408,       /**< Request Time-out */
    HTTP_ST_409,       /**< Conflict */
    HTTP_ST_410,       /**< Gone */
    HTTP_ST_411,       /**< Length Required */
    HTTP_ST_412,       /**< Precondition Failed */
    HTTP_ST_413,       /**< Request Entity Too Large */
    HTTP_ST_414,       /**< Request-URI Too Long */
    HTTP_ST_415,       /**< Unsupported Media Type */
    HTTP_ST_416,       /**< Requested Range Not Satisfiable */
    HTTP_ST_417,       /**< Expectation Failed */
    HTTP_ST_422=422,   /**< Unprocessable Entity */
    HTTP_ST_423,       /**< Locked */
    HTTP_ST_424,       /**< Failed Dependency */
    HTTP_ST_499=499,   /**< Client Error - Others */

    HTTP_ST_500,       /**< Internal Server Error */
    HTTP_ST_501,       /**< Not Implemented */
    HTTP_ST_502,       /**< Bad Gateway */
    HTTP_ST_503,       /**< Service Unavailable */
    HTTP_ST_504,       /**< Gateway Time-out */
    HTTP_ST_505,       /**< HTTP Version not supported */
    HTTP_ST_507=507,   /**< Insufficient Storage */
    HTTP_ST_599=599,   /**< Server Error - Others */
    HTTP_ST_NONE
};

// HTTP request methods
typedef enum _http_mthd http_mthd;
enum _http_mthd
{
    HTTP_MT_OPTIONS = 0, /* RFC2616 */
    HTTP_MT_GET,
    HTTP_MT_HEAD,
    HTTP_MT_POST,
    HTTP_MT_PUT,
    HTTP_MT_DELETE,
    HTTP_MT_TRACE,
    HTTP_MT_CONNECT,
    HTTP_MT_PATCH,
    HTTP_MT_LINK,
    HTTP_MT_UNLINK,
    HTTP_MT_PROPFIND,    /* RFC2518 */
    HTTP_MT_MKCOL,
    HTTP_MT_COPY,
    HTTP_MT_MOVE,
    HTTP_MT_LOCK,
    HTTP_MT_UNLOCK,
    HTTP_MT_POLL,        /* Outlook Web Access */
    HTTP_MT_BCOPY,
    HTTP_MT_BMOVE,
    HTTP_MT_SEARCH,
    HTTP_MT_BDELETE,
    HTTP_MT_PROPPATCH,
    HTTP_MT_BPROPFIND,
    HTTP_MT_BPROPPATCH,
    HTTP_MT_LABEL,             /* RFC 3253 8.2 */
    HTTP_MT_MERGE,             /* RFC 3253 11.2 */
    HTTP_MT_REPORT,            /* RFC 3253 3.6 */
    HTTP_MT_UPDATE,            /* RFC 3253 7.1 */
    HTTP_MT_CHECKIN,           /* RFC 3253 4.4, 9.4 */
    HTTP_MT_CHECKOUT,          /* RFC 3253 4.3, 9.3 */
    HTTP_MT_UNCHECKOUT,        /* RFC 3253 4.5 */
    HTTP_MT_MKACTIVITY,        /* RFC 3253 13.5 */
    HTTP_MT_MKWORKSPACE,       /* RFC 3253 6.3 */
    HTTP_MT_VERSION_CONTROL,   /* RFC 3253 3.5 */
    HTTP_MT_BASELINE_CONTROL,  /* RFC 3253 12.6 */
    HTTP_MT_NOTIFY,            /* uPnP forum */
    HTTP_MT_SUBSCRIBE,
    HTTP_MT_UNSUBSCRIBE,
    HTTP_MT_ICY,               /* Shoutcast client (forse) */
    HTTP_MT_NONE
};

// HTTP versions
typedef enum _http_ver http_ver;
enum _http_ver
{
    HTTP_VER_1_0,
    HTTP_VER_1_1,
    HTTP_VER_NONE
};

// Self-defined strcture of HTTP status code number and status name
typedef struct _http_st_code http_st_code;
struct _http_st_code
{
    int num;          	/* status code number */
    http_status st;   /* status */
};

// Self-defined structure of HTTP request
typedef struct _request_t request_t;
struct _request_t
{
	http_ver 	version;
	http_mthd	method;
	char*		host;
	char*		uri;
	char*		user_agent;
	char*		referer;
	char*		connection;
	char*		accept;
	char*		accept_encoding;
	char*		accept_language;
	char*		accept_charset;
	char*		cookie;
	/* HTTP_MT_POST */
	char*		content_type;
	char*		content_encoding;
	char*		content_length;
	int			hdlen;	// Header length
};


// Self-defined structure of HTTP response
typedef struct _response_t response_t;
struct _response_t
{
	http_status 	status;
	http_ver	 	version;
	char*			server;
	char*			date;
	char*			expires;
	char*			location;
	char*			etag;
	char*			accept_ranges;
	char*			last_modified;
	char*			content_type;
	char*			content_encoding;
	char* 			content_length;
	char*		 	age;
	int				hdlen;	// Header length
};

// Self-defined HTTP message pair
typedef struct _http_pair_t	http_pair_t;
struct _http_pair_t
{
	request_t	*request;
	response_t	*response;
	time_t	req_fb_sec;
	time_t	req_fb_usec;
	time_t	req_lb_sec;
	time_t	req_lb_usec;
	time_t	rsp_fb_sec;
	time_t	rsp_fb_usec;
	time_t	rsp_lb_sec;
	time_t	rsp_lb_usec;
	u_int32_t	req_total_len;
	u_int32_t	rsp_total_len;
	u_int32_t	req_body_len;
	u_int32_t	rsp_body_len;
};

u_int8_t HttpMessageType(const char *p, const int datalen, const char **hdend);	/* If the packet is carrying HTTP(request or response) data */
http_pair_t *HTTPPairNew(void);					/* Create a new http_pair_t object */
request_t *HTTPReqNew(void);					/* Create a new request_t object */
response_t *HTTPRspNew(void);					/* Create a new response_t object */

void HTTPReqFree(request_t *req);				/* Free a request_t object */
void HTTPRspFree(response_t *rsp);				/* Free a response_t object */
void HTTPPairFree(http_pair_t *h);				/* Free a http_pair_t object */
void HTTPPairQueueFree(Queue *q);					/* Fee a queue whose data is http pair type */

int HTTPParseReq(request_t *request, const char *data, const char *dataend);		/* Parse the packet and store in a request_t object */
int HTTPParseRsp(response_t *response, const char *data, const char *dataend);		/* Parse the packet and store in a response_t object */

#endif /* __HTTP_H__ */

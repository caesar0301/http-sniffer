/*
 * http.h
 *
 *  Created on: Jun 17, 2011
 *      Author: chenxm
 */
#ifndef __HTTP_H__
#define __HTTP_H__

#include <sys/types.h>
#include <time.h>

typedef enum _http_status http_status;
typedef enum _http_mthd http_mthd;
typedef enum _http_ver http_ver;
typedef struct _http_st_code http_st_code;
typedef struct _request_t request_t;
typedef struct _response_t response_t;
typedef struct _http_pair_t	http_pair_t;

extern http_st_code HTTP_STATUS_CODE_ARRAY[] = {
    {100, HTTP_ST_100},
    {101, HTTP_ST_101},
    {102, HTTP_ST_102},
    {199, HTTP_ST_199},

    {200, HTTP_ST_200},
    {201, HTTP_ST_201},
    {202, HTTP_ST_202},
    {203, HTTP_ST_203},
    {204, HTTP_ST_204},
    {205, HTTP_ST_205},
    {206, HTTP_ST_206},
    {207, HTTP_ST_207},
    {299, HTTP_ST_299},

    {300, HTTP_ST_300},
    {301, HTTP_ST_301},
    {302, HTTP_ST_302},
    {303, HTTP_ST_303},
    {304, HTTP_ST_304},
    {305, HTTP_ST_305},
    {307, HTTP_ST_307},
    {399, HTTP_ST_399},

    {400, HTTP_ST_400},
    {401, HTTP_ST_401},
    {402, HTTP_ST_402},
    {403, HTTP_ST_403},
    {404, HTTP_ST_404},
    {405, HTTP_ST_405},
    {406, HTTP_ST_406},
    {407, HTTP_ST_407},
    {408, HTTP_ST_408},
    {409, HTTP_ST_409},
    {410, HTTP_ST_410},
    {411, HTTP_ST_411},
    {412, HTTP_ST_412},
    {413, HTTP_ST_413},
    {414, HTTP_ST_414},
    {415, HTTP_ST_415},
    {416, HTTP_ST_416},
    {417, HTTP_ST_417},
    {422, HTTP_ST_422},
    {423, HTTP_ST_423},
    {424, HTTP_ST_424},
    {499, HTTP_ST_499},

    {500, HTTP_ST_500},
    {501, HTTP_ST_501},
    {502, HTTP_ST_502},
    {503, HTTP_ST_503},
    {504, HTTP_ST_504},
    {505, HTTP_ST_505},
    {507, HTTP_ST_507},
    {599, HTTP_ST_599}
};

extern char *HTTP_METHOD_STRING_ARRAY[] = {
    "OPTIONS",
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "TRACE",
    "CONNECT",
    "PATCH",
    "LINK",
    "UNLINK",
    "PROPFIND",
    "MKCOL",
    "COPY",
    "MOVE",
    "LOCK",
    "UNLOCK",
    "POLL",
    "BCOPY",
    "BMOVE",
    "SEARCH",
    "BDELETE",
    "PROPPATCH",
    "BPROPFIND",
    "BPROPPATCH",
    "LABEL",
    "MERGE",
    "REPORT",
    "UPDATE",
    "CHECKIN",
    "CHECKOUT",
    "UNCHECKOUT",
    "MKACTIVITY",
    "MKWORKSPACE",
    "VERSION-CONTROL",
    "BASELINE-CONTROL",
    "NOTIFY",
    "SUBSCRIBE",
    "UNSUBSCRIBE",
    "ICY",
    "NONE"
};

/*
 * HTTP response status number.
 */
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

enum _http_ver
{
    HTTP_VER_1_0,
    HTTP_VER_1_1,
    HTTP_VER_NONE
};


struct _http_st_code
{
    int num;          	/* status code number */
    http_status st;   /* status */
};

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

struct _http_pair_t
{
	request_t	*request_header;
	response_t	*response_header;
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
	http_pair_t	*next;
};


char *IsRequest(const char *p, const int datalen);			/* If the packet is carrying HTTP request data */
char *IsResponse(const char *p, const int datalen);			/* If the packet is carrying HTTP response data */
u_int8_t IsHttpPacket(const char *p, const int datalen);	/* If the packet is carrying HTTP(request or response) data */

http_pair_t *HTTPPairNew(void);					/* Create a new http_pair_t object */
request_t *HTTPReqNew(void);					/* Create a new request_t object */
response_t *HTTPRspNew(void);					/* Create a new response_t object */
void HTTPReqFree(request_t *req);				/* Free a request_t object */
void HTTPRspFree(response_t *rsp);				/* Free a response_t object */
void HTTPPairFree(http_pair_t *h);				/* Free a http_pair_t object */
int HTTPReqAdd(http_pair_t *h, request_t *req);	/* Add a request_t object to http_pair_t request chain */
int HTTPRspAdd(http_pair_t *h, response_t *rsp);		/* Add a response_t object to http_pair_t response chain */
int HTTPParseReq(request_t *request, const char *data, const char *dataend);		/* Parse the packet and store in a request_t object */
int HTTPParseRsp(response_t *response, const char *data, const char *dataend);		/* Parse the packet and store in a response_t object */

#endif /* __HTTP_H__ */

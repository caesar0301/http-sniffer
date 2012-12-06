/*
 * http.c
 *
 *  Created on: Jun 17, 2011
 *      Author: chenxm
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "http.h"
#include "util.h"

extern http_st_code HTTP_STATUS_CODE_ARRAY[];
extern char *HTTP_METHOD_STRING_ARRAY[];

static char* is_http_request(const char *ptr, const int datalen);
static char* is_http_response(const char *ptr, const int datalen);
static char* find_line_end(const char *data, const char *dataend, const char **eol);
static int get_token_len(const char *linep, const char *lineend, const char **next_token);
static char* find_header_end(const char *data, const char *dataend, int *line_cnt);
static http_mthd http_request_method(const char *data, int linelen);
static char* http_request_uri(const char *line, int len);
static http_ver http_request_version(const char *line, int len);
static http_ver http_response_version(const char *line, int len);
static http_status http_response_status(const char *line, int len);


u_int8_t HttpMessageType(const char *p, const int datalen, const char **hdend)
{
	char *req_head_end = is_http_request(ptr, datalen);
	if(req_head_end != NULL){
		*hdend = req_head_end;
		return 0x01;	//request
	}
	
	char *rsp_head_end = is_http_response(ptr, datalen);
	if (rsp_head_end != NULL){
		*hdend = rsp_head_end;
		return 0x10;	//response
	}

	*hdend = NULL;
	return 0x00;		//non-http
	}
}

http_pair_t *HTTPPairNew(void)
{
	http_pair_t	*h = NULL;
	h = MALLOC(http_pair_t, 1);
	memset(h, 0, sizeof(http_pair_t));
	return h;
}

request_t *HTTPReqNew(void)
{
	request_t *r = NULL;
	r = MALLOC(request_t, 1);
	memset(r, 0, sizeof(request_t));
	return r;
}

response_t *HTTPRspNew(void)
{
	response_t *r = NULL;
	r = MALLOC(response_t, 1);
	memset(r, 0, sizeof(response_t));
	return r;
}

void HTTPReqFree(request_t *req)
{
	if(r->host != NULL)
		free(r->host);
	if(r->uri != NULL)
		free(r->uri);
	if(r->user_agent != NULL)
		free(r->user_agent);
	if(r->referer != NULL)
		free(r->referer);
	if(r->content_type != NULL)
		free(r->content_type);
	if(r->content_encoding != NULL)
		free(r->content_encoding);
	free(r);
}

void HTTPRspFree(response_t *rsp)
{
	if(r->server != NULL)
		free(r->server);
	if(r->date != NULL)
		free(r->date);
	if(r->expires != NULL)
		free(r->expires);
	if(r->location != NULL)
		free(r->location);
	if(r->etag != NULL)
		free(r->etag);
	if(r->accept_ranges != NULL)
		free(r->accept_ranges);
	if(r->last_modified != NULL)
		free(r->last_modified);
	if(r->content_type != NULL)
		free(r->content_type);
	if(r->content_encoding != NULL)
		free(r->content_encoding);

	free(r);
}

void HTTPPairFree(http_pair_t *h)
{
	if(h->request_header != NULL)
		http_request_free(h->request_header);
	if(h->response_header != NULL)
		http_response_free(h->response_header);
	free(h);
}

int HTTPReqAdd(http_pair_t *h, request_t *req)
{
	if(h->request_header == NULL){
		h->request_header = req;
		return 0;
	}else{
		return 1;
	}
}

int HTTPRspAdd(http_pair_t *h, response_t *rsp)
{
	if(h->response_header == NULL){
		h->response_header = rsp;
		return 0;
	}else{
		return 1;
	}
}

/*
 * Extract request message from data.
 * But only the header fields are extracted.
 */
int HTTPParseReq(request_t *request, const char *data, const char *dataend)
{
	char *eoh, *eol, *linesp, *lineep;
	int line_cnt = 0, lnl = 0, hdl = 0;

	eoh = find_header_end(data, dataend, &line_cnt);
	hdl = eoh - data + 1;
	request->hdlen = hdl;

	/* Parse first line of http request */
	linesp = data;
	lineep = find_line_end(linesp, eoh, &eol);
	lnl = lineep - linesp + 1;
	request->method = http_request_method(linesp, lnl);
	if ( request->method == HTTP_MT_NONE){
		return -1;
	}

	request->uri = http_request_uri(linesp, lnl);
	request->version = http_request_version(linesp, lnl);
	request->host = http_header_param(data, hdl, "Host:");
	request->referer = http_header_param(data, hdl, "Referer:");
	request->user_agent = http_header_param(data, hdl, "User-Agent:");
	request->connection = http_header_param(data, hdl, "Connection:");
	request->accept = http_header_param(data, hdl, "Accept:");
	request->accept_encoding = http_header_param(data, hdl, "Accept-Encoding:");
	request->accept_language = http_header_param(data, hdl, "Accept-Language:");
	request->accept_charset = http_header_param(data, hdl, "Accept-Charset:");
	request->cookie = http_header_param(data, hdl, "Cookie:");

	if (request->method == HTTP_MT_POST ){
		request->content_type = http_header_param(data, hdl, "Content-Type:");
		request->content_encoding = http_header_param(data, hdl, "Content-Encoding:");
		request->content_length = http_header_param(data, hdl, "Content-Length:");
	}
	return 0;
}

/*
 * Extract response message from data.
 * But only the header fields are extracted.
 */
int HTTPParseRsp(response_t *response, const char *data, const char *dataend)
{
	char *eoh, *eol, *linesp, *lineep;
	int line_cnt = 0, i = 0, dim = sizeof(HTTP_STATUS_CODE_ARRAY), lnl = 0, hdl = 0;
	http_status status;

	eoh = find_header_end(data, dataend, &line_cnt);
	hdl = eoh -data + 1;
	response->hdlen = hdl;

	/* first line */
	linesp = data;
	lineep = find_line_end(linesp, eoh, &eol);
	lnl = lineep - linesp + 1;

	response->status = http_response_status(linesp, lnl);
	if (response->status == HTTP_ST_NONE){
		return -1;
	}

	response->version = http_response_version(linesp, lnl);
	response->server = http_header_param(data, hdl, "Server:");

	/* Cache */
	response->date = http_header_param(data, hdl, "Date:");
	response->expires = http_header_param(data, hdl, "Expires:");
	response->last_modified = http_header_param(data, hdl, "Last-Modified:");
	response->etag = http_header_param(data, hdl, "ETag:");
	response->age = http_header_param(data, hdl, "Age:");
	
	/* Content */
	response->accept_ranges = http_header_param(data, hdl, "Accept-Ranges:");
	response->content_type = http_header_param(data, hdl, "Content-Type:");
	response->content_encoding = http_header_param(data, hdl, "Content-Encoding:");
	response->content_length = http_header_param(data, hdl, "Content-Length:");
	response->location = http_header_param(data, hdl, "Location:");

	return 0;
}


/*
 * To identify if the packet is carrying HTTP request message.
 * If it's true, the head end char pointer will be returned, else NULL.
 */
static char* is_http_request(const char *ptr, const int datalen)
{
	http_mthd method = HTTP_MT_NONE;
	char *head_end = NULL;

	method = http_request_method(ptr, datalen);
	if (method == HTTP_MT_NONE){
		return NULL;
	}
	else{
		int line_cnt = 0;
		head_end = find_header_end(ptr, (ptr+datalen-1), &line_cnt);
		return head_end;
	}
}
/*
 * To identify if the packet is carrying HTTP response message.
 * If it's true, the head end char pointer will be returned, else NULL.
 */
static char* is_http_response(const char *ptr, const int datalen)
{
	http_ver version = HTTP_VER_NONE;
	char *head_end = NULL;

	if (datalen < 8)
		return NULL;

	version = http_response_version(ptr, datalen);
	if (version == HTTP_VER_NONE)
		return NULL;
	else{
		int line_cnt = 0;
		head_end = find_header_end(ptr, (ptr+datalen-1), &line_cnt);
		return head_end;
	}
}


/*
 * From xplico.
 * Given a pointer into a data buffer, and to the end of the buffer,
 * find the end of the line at that position in the data
 * buffer.
 * Return a pointer to the EOL character(s) in "*eol", which is the first of
 * EOL character(s).
 */
static char* 
find_line_end(const char *data, const char *dataend, const char **eol)
{
	const char *lineend;

	lineend = memchr(data, '\n', dataend - data + 1);

	if (lineend == NULL) {
		/*
		 * No LF - line is probably continued in next TCP segment.
		 */
		lineend = dataend;
		*eol = dataend;
	} else {
		/*
		 * Is the LF at the beginning of the line?
		 */
		if (lineend > data) {
			/*
			 * No - is it preceded by a carriage return?
			 * (Perhaps it's supposed to be, but that's not guaranteed....)
			 */
			if (*(lineend - 1) == '\r') {
				/*
				 * Yes.  The EOL starts with the CR.
				 */
				*eol = lineend - 1;

			} else {
				/*
				 * No.  The EOL starts with the LF.
				 */
				*eol = lineend;

				/*
				 * I seem to remember that we once saw lines ending with LF-CR
				 * in an HTTP request or response, so check if it's *followed*
				 * by a carriage return.
				 */
				if (lineend < (dataend - 1) && *(lineend + 1) == '\r') {
					/*
					 * It's <non-LF><LF><CR>; say it ends with the CR.
					 */
					lineend++;
				}
			}
		} else {

			/*
			 * Yes - the EOL starts with the LF.
			 */
			*eol = lineend;
		}
	}
	return lineend;
}

/*
 * From xplico.
 * Get the length of the next token in a line, and the beginning of the
 * next token after that (if any).
 * Return 0 if there is no next token.
 */
static int 
get_token_len(const char *linep, const char *lineend, const char **next_token)
{
    const char *tokenp;
    int token_len;

    tokenp = linep;

    /*
     * Search for a blank, a CR or an LF, or the end of the buffer.
     */
    while (linep < lineend && *linep != ' ' && *linep != '\r' && *linep != '\n')
        linep++;
    token_len = linep - tokenp;

    /*
     * Skip trailing blanks.
     */
    while (linep < lineend && *linep == ' ')
        linep++;

    *next_token = linep;

    return token_len;
}

/*
 * From xplico.
 * Given a pointer into a data buffer and the length of buffer,
 * find the header end.
 * Return a pointer to the end character of header
 */
static char* 
find_header_end(const char *data, const char *dataend, int *line_cnt)
{
    const char *lf, *nxtlf, *end;

    end = NULL;
    lf =  memchr(data, '\n', (dataend - data + 1));
    if (lf == NULL)
        return NULL;
    (*line_cnt)++;
    lf++; /* next charater */
    nxtlf = memchr(lf, '\n', (dataend - lf + 1));
    while (nxtlf != NULL) {
        if (nxtlf-lf < 2) {
            end = nxtlf;
            break;
        }
        (*line_cnt)++;
        nxtlf++;
        lf = nxtlf;
        nxtlf = memchr(nxtlf, '\n', dataend - nxtlf + 1);
    }
    return (char *)end;
}

/*
 * From xplico.
 * Get HTTP request method by parsing header line.
 */
static http_mthd 
http_request_method(const char *data, int linelen)
{
    const char *ptr;
    int	index = 0;
    int prefix_len = 0;
    char *unkn;

    /*
     * From RFC 2774 - An HTTP Extension Framework
     *
     * Support the command prefix that identifies the presence of
     * a "mandatory" header.
     */
    if (linelen >= 2) {
        if (strncmp(data, "M-", 2) == 0 || strncmp(data, "\r\n", 2) == 0) { /* \r\n necesary for bug in client POST */
            data += 2;
            linelen -= 2;
            prefix_len = 2;
        }
    }

    /*
     * From draft-cohen-gena-client-01.txt, available from the uPnP forum:
     *	NOTIFY, SUBSCRIBE, UNSUBSCRIBE
     *
     * From draft-ietf-dasl-protocol-00.txt, a now vanished Microsoft draft:
     *	SEARCH
     */
    ptr = (const char *)data;
    /* Look for the space following the Method */
    while (index < linelen) {
        if (*ptr == ' ')
            break;
        else {
            ptr++;
            index++;
        }
    }

    /* Check the methods that have same length */
    switch (index) {
    case 3:
        if (strncmp(data, "GET", index) == 0) {
            return HTTP_MT_GET;
        }
        else if (strncmp(data, "PUT", index) == 0) {
            return HTTP_MT_PUT;
        }
#if 0
	else if (strncmp(data, "ICY", index) == 0) {
            return HTTP_MT_ICY;
        }
#endif
        break;

    case 4:
        if (strncmp(data, "COPY", index) == 0) {
            return HTTP_MT_COPY;
        }
        else if (strncmp(data, "HEAD", index) == 0) {
            return HTTP_MT_HEAD;
        }
        else if (strncmp(data, "LOCK", index) == 0) {
            return HTTP_MT_LOCK;
        }
        else if (strncmp(data, "MOVE", index) == 0) {
            return HTTP_MT_MOVE;
        }
        else if (strncmp(data, "POLL", index) == 0) {
            return HTTP_MT_POLL;
        }
        else if (strncmp(data, "POST", index) == 0) {
            return HTTP_MT_POST;
        }
        break;

    case 5:
        if (strncmp(data, "BCOPY", index) == 0) {
            return HTTP_MT_BCOPY;
        }
        else if (strncmp(data, "BMOVE", index) == 0) {
            return HTTP_MT_BMOVE;
        }
        else if (strncmp(data, "MKCOL", index) == 0) {
            return HTTP_MT_MKCOL;
        }
        else if (strncmp(data, "TRACE", index) == 0) {
            return HTTP_MT_TRACE;
        }
        else if (strncmp(data, "LABEL", index) == 0) {  /* RFC 3253 8.2 */
            return HTTP_MT_LABEL;
        }
        else if (strncmp(data, "MERGE", index) == 0) {  /* RFC 3253 11.2 */
            return HTTP_MT_MERGE;
        }
        break;

    case 6:
        if (strncmp(data, "DELETE", index) == 0) {
            return HTTP_MT_DELETE;
        }
        else if (strncmp(data, "SEARCH", index) == 0) {
            return HTTP_MT_SEARCH;
        }
        else if (strncmp(data, "UNLOCK", index) == 0) {
            return HTTP_MT_UNLOCK;
        }
        else if (strncmp(data, "REPORT", index) == 0) {  /* RFC 3253 3.6 */
            return HTTP_MT_REPORT;
        }
        else if (strncmp(data, "UPDATE", index) == 0) {  /* RFC 3253 7.1 */
            return HTTP_MT_UPDATE;
        }
        else if (strncmp(data, "NOTIFY", index) == 0) {
            return HTTP_MT_NOTIFY;
        }
        break;

    case 7:
        if (strncmp(data, "BDELETE", index) == 0) {
            return HTTP_MT_BDELETE;
        }
        else if (strncmp(data, "CONNECT", index) == 0) {
            return HTTP_MT_CONNECT;
        }
        else if (strncmp(data, "OPTIONS", index) == 0) {
            return HTTP_MT_OPTIONS;
        }
        else if (strncmp(data, "CHECKIN", index) == 0) {  /* RFC 3253 4.4, 9.4 */
            return HTTP_MT_CHECKIN;
        }
        break;

    case 8:
        if (strncmp(data, "PROPFIND", index) == 0) {
            return HTTP_MT_PROPFIND;
        }
        else if (strncmp(data, "CHECKOUT", index) == 0) { /* RFC 3253 4.3, 9.3 */
            return HTTP_MT_CHECKOUT;
        }
        /*
        else if (strncmp(data, "CCM_POST", index) == 0) {
            return HTTP_MT_CCM_POST;
        }
        */
        break;

    case 9:
        if (strncmp(data, "SUBSCRIBE", index) == 0) {
            return HTTP_MT_SUBSCRIBE;
        }
        else if (strncmp(data, "PROPPATCH", index) == 0) {
            return HTTP_MT_PROPPATCH;
        }
        else  if (strncmp(data, "BPROPFIND", index) == 0) {
            return HTTP_MT_BPROPFIND;
        }
        break;

    case 10:
        if (strncmp(data, "BPROPPATCH", index) == 0) {
            return HTTP_MT_BPROPPATCH;
        }
        else if (strncmp(data, "UNCHECKOUT", index) == 0) {  /* RFC 3253 4.5 */
            return HTTP_MT_UNCHECKOUT;
        }
        else if (strncmp(data, "MKACTIVITY", index) == 0) {  /* RFC 3253 13.5 */
            return HTTP_MT_MKACTIVITY;
        }
        break;

    case 11:
        if (strncmp(data, "MKWORKSPACE", index) == 0) {  /* RFC 3253 6.3 */
            return HTTP_MT_MKWORKSPACE;
        }
        else if (strncmp(data, "UNSUBSCRIBE", index) == 0) {
            return HTTP_MT_UNSUBSCRIBE;
        }
        /*
        else if (strncmp(data, "RPC_CONNECT", index) == 0) {
            return HTTP_MT_RPC_CONNECT;
        }
        */
        break;

    case 15:
        if (strncmp(data, "VERSION-CONTROL", index) == 0) {  /* RFC 3253 3.5 */
            return HTTP_MT_VERSION_CONTROL;
        }
        break;

    case 16:
        if (strncmp(data, "BASELINE-CONTROL", index) == 0) {  /* RFC 3253 12.6 */
            return HTTP_MT_BASELINE_CONTROL;
        }
        break;

    default:
        break;
    }

    return HTTP_MT_NONE;
}

/*
 * From xplico.
 * Get HTTP request URI by parsing header line.
 * Return NULL if no URI found.
 */
static char* 
http_request_uri(const char *line, int len)
{
    const char *next_token;
    const char *lineend;
    int tokenlen;
    char *uri;

    lineend = line + len;

    /* The first token is the method. */
    tokenlen = get_token_len(line, lineend, &next_token);
    if (tokenlen == 0 || line[tokenlen] != ' ') {
        return NULL;
    }
    line = next_token;

    /* The next token is the URI. */
    tokenlen = get_token_len(line, lineend, &next_token);
    if (tokenlen == 0 || line[tokenlen] != ' ')
        return NULL;

    uri = MALLOC(char, tokenlen+1);
    if (uri != NULL) {
        memcpy(uri, line, tokenlen);
        uri[tokenlen] = '\0';
    }

    return uri;
}

/*
 * From xplico.
 * Get HTTP request version by parsing header line.
 */
static http_ver 
http_request_version(const char *line, int len)
{
    const char *next_token;
    const char *lineend;
    int tokenlen;

    lineend = line + len;

    /* The first token is the method. */
    tokenlen = get_token_len(line, lineend, &next_token);
    if (tokenlen == 0 || line[tokenlen] != ' ') {
        return HTTP_VER_NONE;
    }
    line = next_token;

    /* The next token is the URI. */
    tokenlen = get_token_len(line, lineend, &next_token);
    if (tokenlen == 0 || line[tokenlen] != ' ')
        return HTTP_VER_NONE;
    line = next_token;

    /* Everything to the end of the line is the version. */
    tokenlen = lineend - line;
    if (tokenlen == 0)
        return HTTP_VER_NONE;

    if (strncmp(line, "HTTP/1.0", 8) == 0)
        return HTTP_VER_1_0;

    if (strncmp(line, "HTTP/1.1", 8) == 0)
        return HTTP_VER_1_1;

    return HTTP_VER_NONE;
}


/*
 * From xplico.
 * Get HTTP response version by parsing header line.
 */
static http_ver 
http_response_version(const char *line, int len)
{
    if (strncmp(line, "HTTP/1.0", 8) == 0)
        return HTTP_VER_1_0;

    if (strncmp(line, "HTTP/1.1", 8) == 0)
        return HTTP_VER_1_1;

    return HTTP_VER_NONE;
}

/*
 * From xplico.
 * Get the HTTP response status code by parsing header line.
 */
static http_status 
http_response_status(const char *line, int len)
{
    const char *next_token;
    const char *lineend;
    http_status status;
    int tokenlen, val;
    int i, dim = sizeof(HTTP_STATUS_CODE_ARRAY)/sizeof(http_st_code);

    lineend = line + len;
    status = HTTP_ST_NONE;

    /* The first token is the protocol and version */
    tokenlen = get_token_len(line, lineend, &next_token);
    if (tokenlen == 0 || line[tokenlen] != ' ') {
        return status;
    }

    line = next_token;
    /* The next token is status value. */
    tokenlen = get_token_len(line, lineend, &next_token);
    if (tokenlen == 0 || (line[tokenlen] != ' ' && line[tokenlen] != '\r' && line[tokenlen] != '\n')) {
        return status;
    }

    /*
     * Parse response status value.
     */
    if (sscanf(line, "%i", &val) != 1) {
		return status;
	}

    /* search enum */
    for (i=0; i<dim; i++) {
        if (HTTP_STATUS_CODE_ARRAY[i].num == val) {
            status = HTTP_STATUS_CODE_ARRAY[i].st;

            break;
        }
    }
    return status;
}

/*
 * From xplico.
 * Parse header parameter from HTTP header fields.
 * Return the pointer to the parameter value if found;
 * else return NULL.
 */
static char* 
http_header_param(const char *header, int hlen, const char *param)
{
    const char *line, *eol, *lineend, *hend, *c;
    char *ret;
    int len, host_len, param_len;

    hend = header + hlen - 1;
    line = header;
    len = hlen;
    ret = NULL;
    lineend = NULL;
    param_len = strlen(param);
    while (lineend < hend) {
        lineend = find_line_end(line, line+len-1, &eol);
        if (lineend != hend && (*eol == '\r' || *eol == '\n')) {
            if (strncasecmp(line, param, param_len) == 0) {
                c = line + param_len;
                while (*c == ' ' && c < lineend)
                    c++;
                /*
                 * Move the EOL pointer to the last none-LFCR character.
                 */
                while ( (*eol == '\r' || *eol == '\n') && eol > c)
                	eol--;
                host_len = eol - c + 1;
                ret = MALLOC(char, host_len + 1);
                memset(ret, '\0', host_len + 1);
                memcpy(ret, c, host_len);
                break;
            }
        }
        line = lineend + 1;
        len = hend - lineend;
    }
    return ret;
}
/*
 * io.c
 *
 *  Created on: Mar. 12, 2015
 *     Author: chenxm
 */
#include <json-c/json.h>
#include <unistd.h>

#include "flow.h"
#include "io.h"


/* Output flow's brief to stdout */
void
flow_print(const flow_t *flow){
	time_t raw_time;
	struct tm *timeinfo = NULL;
	char time_buf[20];
	/*Convert IP addr */
	char *saddr = malloc(sizeof("aaa.bbb.ccc.ddd"));
	char *daddr = malloc(sizeof("aaa.bbb.ccc.ddd"));
	strncpy(saddr, ip_ntos(flow->socket.saddr), sizeof("aaa.bbb.ccc.ddd"));
	strncpy(daddr, ip_ntos(flow->socket.daddr), sizeof("aaa.bbb.ccc.ddd"));
	/* Get local time */
	time( &raw_time );
	timeinfo = localtime( &raw_time );
	memset(time_buf, 0, sizeof(time_buf));
	strftime(time_buf, sizeof(time_buf), "%Y%m%d %H:%M:%S", timeinfo);
	/* Print flow information. */
	printf("\n[%s]%s:%d-->%s:%d %d.%d %d.%d %d.%d %d %d/%d %d/%d %d %d\n",
			time_buf,
			saddr,
			flow->socket.sport,
			daddr,
			flow->socket.dport,
			(int)flow->syn_sec,
			(int)flow->syn_usec,
			(int)flow->fb_sec,
			(int)flow->fb_usec,
			(int)flow->lb_sec,
			(int)flow->lb_usec,
			(int)flow->rtt,
			flow->pkts_src,
			flow->pkts_dst,
			flow->payload_src,
			flow->payload_dst,
			flow->http_cnt,
			(flow->close == FORCED_CLOSE) ? 1 : 0);
	free(saddr);
	free(daddr);
}


/**
 * Dump a flow_t object into a file
 */
void
save_flow_json(const flow_t *flow, const char* file){

	// Filter flows with out HTTP pairs.
	if(flow->http_cnt == 0){
		return;
	}

	struct json_object *new_flow;
	struct json_object *http_pairs, *new_http_pair, *new_request, *new_response;

	new_flow = json_object_new_object();

	/* Timestamp */
	char time_buf[20];
	time_t raw_time;
	struct tm *timeinfo = NULL;

	memset(time_buf, 0, sizeof(time_buf));
	time( &raw_time );
	timeinfo = localtime( &raw_time );

	// ISO 8601 time format
	strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%S", timeinfo);
	json_object_object_add(new_flow, "t_r", json_object_new_string(time_buf));

	/*Convert IP addr */
	char *saddr = malloc(sizeof("aaa.bbb.ccc.ddd"));
	char *daddr = malloc(sizeof("aaa.bbb.ccc.ddd"));
	strncpy(saddr, ip_ntos(flow->socket.saddr), sizeof("aaa.bbb.ccc.ddd"));
	strncpy(daddr, ip_ntos(flow->socket.daddr), sizeof("aaa.bbb.ccc.ddd"));
	json_object_object_add(new_flow, "sa", json_object_new_string(saddr));
	json_object_object_add(new_flow, "da", json_object_new_string(daddr));
	json_object_object_add(new_flow, "sp", json_object_new_int(flow->socket.sport));
	json_object_object_add(new_flow, "dp", json_object_new_int(flow->socket.dport));
	json_object_object_add(new_flow, "synt", json_object_new_double(flow->syn_sec + flow->syn_usec*0.000001));
	json_object_object_add(new_flow, "fbt", json_object_new_double(flow->fb_sec + flow->fb_usec*0.000001));
	json_object_object_add(new_flow, "lbt", json_object_new_double(flow->lb_sec + flow->lb_usec*0.000001));
	json_object_object_add(new_flow, "rtt", json_object_new_int(flow->rtt));
	json_object_object_add(new_flow, "spkts", json_object_new_int(flow->pkts_src));
	json_object_object_add(new_flow, "dpkts", json_object_new_int(flow->pkts_dst));
	json_object_object_add(new_flow, "spl", json_object_new_int(flow->payload_src));
	json_object_object_add(new_flow, "dpl", json_object_new_int(flow->payload_dst));
	json_object_object_add(new_flow, "fc", json_object_new_int((flow->close == FORCED_CLOSE) ? 1 : 0));
	json_object_object_add(new_flow, "pcnt", json_object_new_int(flow->http_cnt));

	/* Print flow information. */
	http_pairs = json_object_new_array();
	if(flow->http_f != NULL){
		http_pair_t *http;
		http = flow->http_f;

		while(http != NULL){
			new_http_pair = json_object_new_object();
			if(http->request_header != NULL){
				request_t *req = http->request_header;
				new_request = json_object_new_object();
				json_object_object_add(new_request, "fbt", json_object_new_double(http->req_fb_sec + http->req_fb_usec*0.000001));
				json_object_object_add(new_request, "lbt", json_object_new_double(http->req_lb_sec + http->req_lb_usec*0.000001));
				json_object_object_add(new_request, "totlen", json_object_new_int(http->req_total_len));
				json_object_object_add(new_request, "bdylen", json_object_new_int(http->req_body_len));	// ****
				json_object_object_add(new_request, "ver", json_object_new_int(req->version));
				json_object_object_add(new_request, "mth", json_object_new_string(HTTP_METHOD_STRING_ARRAY[req->method]));
				if(req->host != NULL)
					json_object_object_add(new_request, "host", json_object_new_string(req->host));
				if(req->uri != NULL)
					json_object_object_add(new_request, "uri", json_object_new_string(req->uri));
				if(req->referer != NULL)
					json_object_object_add(new_request, "ref", json_object_new_string(req->referer));
				if(req->user_agent != NULL)
					json_object_object_add(new_request, "ua", json_object_new_string(req->user_agent));
				if(req->accept != NULL)
					json_object_object_add(new_request, "accept", json_object_new_string(req->accept));
				if(req->accept_encoding != NULL)
					json_object_object_add(new_request, "accept_encoding", json_object_new_string(req->accept_encoding));
				if(req->accept_language != NULL)
					json_object_object_add(new_request, "accept_language", json_object_new_string(req->accept_language));
				if(req->accept_charset != NULL)
					json_object_object_add(new_request, "accept_charset", json_object_new_string(req->accept_charset));
				if(req->cookie != NULL)
					json_object_object_add(new_request, "cookie", json_object_new_string(req->cookie));
				if(req->content_type != NULL)
					json_object_object_add(new_request, "contyp", json_object_new_string(req->content_type));
				if(req->content_encoding != NULL)
					json_object_object_add(new_request, "conenc", json_object_new_string(req->content_encoding));
				if(req->content_length != NULL)
					json_object_object_add(new_request, "conlen", json_object_new_string(req->content_length));
				json_object_object_add(new_http_pair, "req", new_request);
			}

			if(http->response_header != NULL){
				response_t	*rsp = http->response_header;
				new_response = json_object_new_object();
				json_object_object_add(new_response, "fbt", json_object_new_double(http->rsp_fb_sec + http->rsp_fb_usec*0.000001));
				json_object_object_add(new_response, "lbt", json_object_new_double(http->rsp_lb_sec + http->rsp_lb_usec*0.000001));
				json_object_object_add(new_response, "totlen", json_object_new_int(http->rsp_total_len));
				json_object_object_add(new_response, "bdylen", json_object_new_int(http->rsp_body_len));
				json_object_object_add(new_response, "ver", json_object_new_int(rsp->version));
				json_object_object_add(new_response, "sta", json_object_new_int(rsp->status));
				if(rsp->server != NULL)
					json_object_object_add(new_response, "server", json_object_new_string(rsp->server));
				if(rsp->location != NULL)
					json_object_object_add(new_response, "loc", json_object_new_string(rsp->location));
				if(rsp->date != NULL)
					json_object_object_add(new_response, "dat", json_object_new_string(rsp->date));
				if(rsp->expires != NULL)
					json_object_object_add(new_response, "exp", json_object_new_string(rsp->expires));
				if(rsp->etag != NULL)
					json_object_object_add(new_response, "etag", json_object_new_string(rsp->etag));
				if(rsp->accept_ranges != NULL)
					json_object_object_add(new_response, "accept_ranges", json_object_new_string(rsp->accept_ranges));
				if(rsp->last_modified != NULL)
					json_object_object_add(new_response, "lmod", json_object_new_string(rsp->last_modified));
				if(rsp->content_type != NULL)
					json_object_object_add(new_response, "contyp", json_object_new_string(rsp->content_type));
				if(rsp->content_encoding != NULL)
					json_object_object_add(new_response, "conenc", json_object_new_string(rsp->content_encoding));
				if(rsp->content_length != NULL)
					json_object_object_add(new_response, "conlen", json_object_new_string(rsp->content_length));
				if(rsp->age != NULL)
					json_object_object_add(new_response, "age", json_object_new_string(rsp->age));
				json_object_object_add(new_http_pair, "res", new_response);
			}

			json_object_array_add(http_pairs, new_http_pair);
			http = http->next;
		}
	}

	json_object_object_add(new_flow, "pairs", http_pairs);

	// Dump to file
	FILE *f = fopen(file, "ab");
	fputs(json_object_to_json_string(new_flow), f);
	fputs("\n", f);
	fclose(f);

	json_object_put(new_flow);
	free(saddr);
	free(daddr);
}
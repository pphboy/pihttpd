#ifndef P_FUND_H
#define P_FUND_H

// 此处放置头文件内容
#include <sys/types.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include "p_sys.h"

#endif // P_SYS_H
void start_server(int port,int threads_num, int threads_queue_size); 

void handle_conn(int sockfd);

void handle_request(void * argument);

void get_info_from_conn(int connfd,  http_request *hr,char *req);

/*
split path and request param2
 */
void is_had_param(http_request *hr);

void set_content_info(http_request *hr,char *req);

int init_server(int port);

void PrintServerInfo(int port);

int is_cgi(char *path);


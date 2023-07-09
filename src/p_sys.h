#ifndef P_SYS_H
#define P_SYS_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "p_http.h"

#endif

void error_exit(char *msg);

void display_http(http_request *hr);

void send_static_file(http_request *hr);

void run_cgi(http_request *hr);

void get_cgi_req_param(int argc, char *argv[],http_request *hr);


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
#include <pthread.h>
#include <time.h>
#include "p_http.h"

#endif

typedef struct {
  void (*function)(void*); // Task method pointer
  void* argument; // Task arguments
} Task;

typedef struct {
  pthread_mutex_t lock;// mutex lock
  pthread_cond_t notify;// conndition val
  pthread_t* threads; // thread array pointer
  Task *queue;
  int thread_count;
  int queue_size;
  // this structure is queue
  int head;
  int tail;
  int current_count;
  int shutdown; // target of shutdown
  int started; // target of start
    
} ThreadPool;

void threadpool_init(ThreadPool* pool, int threads_num);

int threadpool_add_task(ThreadPool* pool,void (*function)(void*), void *argument);

void * threadpool_worker(void * threadpool);

void error_exit(char *msg);

void display_http(http_request *hr);


void display_multipart_form(multipart_form *mf);

void send_static_file(http_request *hr);

void add_http_file_type(http_request *hr);

char* get_postfix_from_path(char *path); 

void run_cgi(http_request *hr);

void get_cgi_req_param(int argc, char *argv[],http_request *hr);

char* map_get(STR_MAP* map, char *key);

void get_sys_time(char* time);

// void map_set(STR* map,char* key, char* value);

void send_recv_to_cgi(http_request *hr, int pipefd);

void show_bytes_line_feed(char *buf);


int get_boundary_end(http_request *hr,char *buf);

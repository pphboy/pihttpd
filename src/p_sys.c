#include "p_sys.h"

/* 
   intialize threadpool.
 */
void threadpool_init(ThreadPool* pool, int threads_num) {
  int i;
  pool->thread_count = 0;
  pool->head = pool->tail = pool->current_count = 0;
  pool->started  = pool->shutdown = 0;

  // intialize mutex and conndition
  pthread_mutex_init(&(pool->lock), NULL);
  pthread_cond_init(&(pool->notify),NULL);

  pool->threads = (pthread_t*) malloc(sizeof(pthread_t)* threads_num);
  pool->queue = (Task*) malloc(sizeof(Task) * pool->queue_size);
  
  if(pool->threads == NULL){
    error_exit("SYSTEM INTIALIZE FAILED");
  }

  for(i = 0; i < threads_num; i++) {

    pthread_create(&(pool->threads[i]), NULL, threadpool_worker, (void*)pool);
    pool->thread_count++;
    pool->started++;
  }
  
}

int threadpool_add_task(ThreadPool* pool,void (*function)(void*), void* argument) {
  pthread_mutex_lock(&(pool->lock)); // locked mutex

  printf("POOL FUNC\n");
  // queue is full, and waiting
  while(pool->current_count == pool->queue_size && !pool->shutdown) {
    pthread_cond_wait(&(pool->notify), &(pool->lock));
  }
  printf("POOL FUNC1\n");
  // if set shutdown, must unlocked
  if(pool->shutdown) {
    pthread_mutex_unlock(&(pool->lock));
    return -1;
  }

  printf("POOL FUNC2 pool->tail[%d] pool->queue_size[%d]\n",pool->tail,pool->queue_size);

    // 添加任务到队列
  pool->queue[pool->tail].function = function;
  pool->queue[pool->tail].argument = argument;

  printf("POOL FUNC3\n");
  
  pool->tail = (pool->tail + 1) % pool->queue_size;
  pool->current_count++;

  // rouse one thread
  pthread_cond_signal(&(pool->notify));

  // unlock
  pthread_mutex_unlock(&(pool->lock));
    
  return 0;
}

void * threadpool_worker(void * threadpool) {
  ThreadPool* pool = (ThreadPool*) threadpool;

  while(1) {
    // locked
    pthread_mutex_lock(&(pool->lock));

    while (pool->current_count == 0 && !pool->shutdown) {
      pthread_cond_wait(&(pool->notify),&(pool->lock));
    }

    // close thread pool
    if(pool->shutdown) {
      pthread_mutex_unlock(&(pool->lock));
      pthread_exit(NULL);
    }

    printf("TAKE_TASK\n");
    // take task
    void (*function)(void*) = pool->queue[pool->head].function;
    void* argument = pool->queue[pool->head].argument;
    
    pool->head = (pool->head+1) % pool->queue_size;
    pool->current_count--;

    // rouse another thread
    pthread_cond_broadcast(&(pool->notify));

    // unlock
    pthread_mutex_unlock(&(pool->lock));

    (*function)(argument);
    
  }
  
  pthread_exit(NULL);
}

/*
  Print Error Information , And Exit System
 */
void error_exit(char* msg) {
  perror(msg);
  exit(1);
}

void display_http(http_request *hr) {
  printf("\t\tHTTP REQUEST\n");
  printf("CONNFD: %d\n",hr->connfd);
  printf("METHOD: %s\n",hr->method);
  printf("PATH: %s\n",hr->path);
  printf("REQ_PARAM: %s\n",hr->req_param);
  printf("CONTENT_TYPE: %s\n",hr->content_type);
  printf("CONTENT_LEGNTH: %d\n",hr->content_len);
  //  printf("CONTENT_DISPOSITION: %s\n",hr->disposition);
  printf("CONTENT: %s\n\n",hr->content);
}

/* 
  read a static file into connfd
 */
void send_static_file(http_request *hr) {
  
  int connfd = hr->connfd;
  
  headers(connfd, HTTP_OK);
  
  // check file type
  //  add_header_value(connfd,CTYPE_HTML);
  add_http_file_type(hr);
  add_header_value(connfd,"\r\n");
  printf("%s\n",hr->path+1);
  
  int staticfd = open(hr->path+1, O_RDONLY);
  printf("staticfd: %d\n",staticfd);
  if(staticfd > 0){
    char buf[1024];
    int len = 1;
    
    while(len > 0){
      /*  once read a byte array like 1024 size  */
      len = read(staticfd, buf, sizeof(buf));
      //      printf("[%d]:%s",len,buf);
      write(connfd, buf, len);
    }

    close(staticfd);
    close(connfd);
    
  } else {
    /* solve Exception */
    send_error(connfd,500);
  }
}

/* 
   file postfix of hr->path
  set the http file type
 */
void add_http_file_type(http_request *hr) {

  char* postfix = get_postfix_from_path(hr->path)+1;
  char* content_type = map_get((STR_MAP*)&CONTENT_TYPE_ARR,postfix);
  
  printf("POSTFIX: %s %d\nMAP_GET:%s\n",postfix,strcmp(postfix,".html"),content_type);

  if(content_type == NULL){
    content_type =  map_get((STR_MAP*)&CONTENT_TYPE_ARR,"file");
  }

  printf("ADD_HEADER_VALUE %s \n",content_type);

  add_header_value(hr->connfd, content_type);
}

/* 
   char *strrchr(const char *s, int c);
   The strrchr() function returns a pointer to the last  occurrence  of
   the character c in the string s.
 */
char* get_postfix_from_path(char *path) {
  return strrchr(path, '.');
}

void run_cgi(http_request *hr) {
  printf("RUN_CGI: %s\n",hr->path+1);
  // using child process execute cgi program
  int pid = -1,status = -1;
  int in_pipe[2],out_pipe[2];
  int len;

    
  if(pipe(in_pipe) == -1) {
    error_exit("IN_PIPE");
  }
  if(pipe(out_pipe) == -1) {
    error_exit("OUT_PIPE");
  }

  if((pid = fork()) <0) {
    send_error(hr->connfd,500);
    error_exit("RUNCGI_FORK");
  }
    
  // child pid number equal zero
  if(pid == 0) {
    char a[10],b[10];
    printf("CHILD RUN_CGI: %s\n",hr->path+1);
    printf("PATH:%s\n",hr->path+1);

    close(in_pipe[1]);
    close(out_pipe[0]);

    dup2(out_pipe[1],1);
    dup2(in_pipe[0],0);

    sprintf((char*)&a,"%d",hr->connfd);
    sprintf((char*)&b,"%d",hr->content_len);
  
    int i = -1;

    if(hr->form == NULL ) {

      i =  execl(hr->path+1,
                     a,
                     hr->method,
                     hr->path,
                     hr->req_param,
                     hr->content_type,
                     b,
                     hr->content
                     ,NULL);      
    } else {

      // upload file
      i =  execl(hr->path+1,
                     a,
                     hr->method,
                     hr->path,
                     hr->req_param,
                     hr->content_type,
                     b,
                     hr->content,
                     hr->form->filename
                     ,NULL);      
    }

    printf("STATUS:%d\n",i);
    if(i == -1) perror("EXE");
    
    exit(0);
  } else {
    close(in_pipe[0]);
    close(out_pipe[1]);

    char buf[1024] = "abc";

    if(hr->form != NULL) {
      send_recv_to_cgi(hr, in_pipe[1]);
    } else {
      write(in_pipe[1],hr->content, strlen(hr->content));
    }

    display_multipart_form(hr->form);

    
    len = 1;
    while(len > 0){
      
      bzero(&buf,sizeof(buf));
      // zero is stdout
      // one is stdin
      len = read(out_pipe[0], &buf, sizeof(buf));
      
      write(hr->connfd, &buf, len);
      // send info to connfd
      // the decide send what by child process
      printf("%d {\n%s}\n",len,buf);
    }

    close(in_pipe[1]);
    close(out_pipe[0]);

    bzero(&buf,sizeof(buf));
    
    printf("CGI_RUN_OK\n");

    int t = waitpid(pid,&status, 0);
    printf("WAITPID STATUS:%d  PID:%d\n",status,t);
    
    close(hr->connfd);
  }
    
}

/* 

   take all of content, send to cgi process
 */
void send_recv_to_cgi(http_request *hr, int pipefd) {
  printf("SEND_RECV_TO_CGI_START123\n");

  display_multipart_form(hr->form);
  
  // write content of hr pointer into pipefd

  // send into file
  // write(in_pipe[1],&buf,sizeof(buf));
  // 1 explained unfnished
  if(strcmp(hr->method, "POST") == 0 && hr->form != NULL ){
    char buf[1024];
    int len = -1;

    printf("CONTENT_WRITE:%s\n",hr->content);

    show_bytes_line_feed(hr->content);

    // why substract one number, because it will write '\0' that the end byte into stream, and file will be inserted extra byte '^D' 
    write(pipefd,hr->content, strlen(hr->content) -1 );

    printf("FINISHED:%d\n",hr->form->finished);
    perror("FINISH");
        
    if(hr->form->finished == 1 ) {
      len = 1024;

      // if size not equal 1024 , must be in end
      while(len == 1024) {
        bzero(&buf, sizeof(buf));
        
        len = recv(hr->connfd, &buf, sizeof(buf), 0);
        perror("SBCe");
        printf("RECV_BUF[%d]:%s\n",len,buf);

        show_bytes_line_feed(buf);
        
        if(len < 1024 && get_boundary_end(hr,buf) == 0) {
          // determine boundary end
          // delete boundary str
          char *start = (char*)&buf;
          int clen = strlen(buf) - strlen(hr->form->boundary) - strlen("----") - strlen("\r\n\r\n");
          
          char *bb = (char*)malloc(clen);
          
          bzero(bb, clen);
          
          strncpy(bb,start,clen);

          write(pipefd,bb,clen);
                    
          printf("bb BUF CONTENT:%s\n123\n BUF[%d] [%d]\n",bb, strlen(bb), clen);
          printf("BUF_+_+_+_+_=OVER\n");

          // out writing
          break;
        }

        // 
        write(pipefd,&buf,len);
      }
      // success
      hr->form->finished = 0;
    }
    
    // over , and set upload finished
    hr->form->finished = 0;
  }

  printf("SEND_RECV_TO_CGI_END\n");
}

void get_cgi_req_param(int argc, char *argv[],http_request *hr) {

  int i;
  bzero(hr,sizeof(http_request));
  /* printf("PARAM_NUM:%d\n",argc); */
  /* printf("PARAM:%d\n",atoi(argv[0])); */
  /* printf("%s\n",argv[1]); */
  /* printf("CGI SET OVER 123"); */  
  hr->connfd = -1;

  strcpy(hr->method, argv[1]);
  strcpy(hr->path, argv[2]);
  strcpy(hr->req_param, argv[3]);
  strcpy(hr->content_type,argv[4]);
  hr->content_len  = atoi(argv[5]);
  
  // if have content, need created a memory that size equal content_length
  if(hr->content_len > 0) {
    hr->content = malloc(hr->content_len);
    strcpy(hr->content, argv[6]);
  }

}

/* 
  get value from STR_MAP
 */
char* map_get(STR_MAP* map, char *key) {
  int i = 0;
  
  for(; map[i].key != NULL;i++) {
    if(strcmp(key,map[i].key) == 0){
      return map[i].value;
    }
  }

  return NULL;
}
/* 
   get localtime of system
 */
void get_sys_time(char *timep) {
  time_t current_time = time(NULL);
  struct tm * local_time = localtime(&current_time);
  char timestr[50];
  strftime(timestr,sizeof(timestr),"%Y-%m-%d %H:%M:%S",local_time);
  strcpy(timep,timestr);
}

/* 
typedef struct {
  char* filename;
  char* disposition;
  int finished;
} multipart_form;
 */
void display_multipart_form(multipart_form *mf) {
  printf("**********MULTIPART_FORM_START*********\n");
  printf("FILENAME:\t\t%s\n", mf->filename);
  printf("DISPOSITION:\t\t%s\n",mf->disposition);
  printf("BOUNDARY:\t\t%s\n",mf->boundary);
  printf("FINISHED:\t\t%d\n",mf->finished);
  printf("***********MULTIPART_FORM_END*********\n");
}

void show_bytes_line_feed(char *buf) {
  printf("---SHOW_BUF----START\n");
  int i = 0;
  for(; i < strlen(buf);i++) {
    if(buf[i] == '\n') {
      printf("\\n");
    } else if(buf[i] == '\r') {
      printf("\\r");
    } else if(buf[i] == '\0') {
      printf("\\0");
    }else {
      printf("%c",buf[i]);      
    }

  }
  printf("---SHOW_BUF----END\n");
}

/* 

   Finded 0
   Not Found 1

 */
int get_boundary_end(http_request *hr,char *buf){
  char* end = strstr(hr->form->boundary,buf);
  printf("GET_BOUNDARY_END:%s\n",hr->form->boundary);
  
  if(end != NULL) {
    return 1;
  }else {
    return 0;
  }
}

#include "p_sys.h"
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
  printf("CONTENT: %s\n\n",hr->content);
}

/* 
  read a static file into connfd
 */
void send_static_file(http_request *hr) {
  
  int connfd = hr->connfd;
  
  headers(connfd, HTTP_OK);
  // check file type
  add_header_value(connfd,CTYPE_HTML);
  add_header_value(connfd,"\r\n");

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


void run_cgi(http_request *hr) {

  printf("RUN_CGI: %s\n",hr->path+1);
  // using child process execute cgi program
  int pid = -1,status = -1;
  int in_pipe[2],out_pipe[2];

    
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
    printf("SEND CONNFD:%s\n",a);
    printf("SEND CONTENT_LENGTH:%s\n",b);
    printf("PATH:%s\n",hr->path+1);

    close(in_pipe[1]);
    close(out_pipe[0]);

    dup2(out_pipe[1],1);
    dup2(in_pipe[0],0);

    sprintf((char*)&a,"%d",hr->connfd);
    sprintf((char*)&b,"%d",hr->content_len);
  
    int i =  execl(hr->path+1,
                   a,
                   hr->method,
                   hr->path,
                   hr->req_param,
                   hr->content_type,
                   b,
                   hr->content
                   ,NULL);

    printf("STATUS:%d\n",i);
    if(i == -1) perror("EXE");
    
    exit(0);
  } else {
    close(in_pipe[0]);
    close(out_pipe[1]);

    char buf[1024] = "abc";
    // send into file
    // write(in_pipe[1],&buf,sizeof(buf));
    
    int len = 1;
    
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
#include "p_fund.h"

#define BUF_SIZE 1024

// key of request header

/*
  
*/

static ThreadPool pool; // thread pool

void start_server(int port,int threads_num, int threads_queue_size){

  int i = -1;
  // set queue size of threadpool
  pool.queue_size = threads_queue_size;

  // initialize threadpool
  threadpool_init(&pool, threads_num);
  
  int sockfd = init_server(port);

  PrintServerInfo(port);  

  handle_conn(sockfd);
  
}

/*
  hanle every connection of client request.
*/
void handle_conn(int sockfd) {
  struct sockaddr_in connaddr;
  int addr_len = sizeof(connaddr);
  while(1) {
    int connfd = accept(sockfd, (struct sockaddr *)&connaddr,(socklen_t*)&addr_len);
    printf("Handle Connfd:%d\n",connfd);
    char buf[10];
    sprintf((char*)&buf,"%d",connfd);
    threadpool_add_task(&pool,(void*)handle_request,(void*)buf);
  }
}

/*
  handle every request content of connection
*/
void handle_request(void * argument) {

  int connfd = atoi((char*)argument);
  char time[50];
  get_sys_time((char*)&time);
  // print the handle time
  printf("-----------------\n\t\t\t\t\tSYSTEM_TIME:%s\n\t\t\t\t\tCONNFD:%d\n------------------\n",time,connfd);
  
  http_request hr;
  char req[BUF_SIZE];
  bzero(req, BUF_SIZE);
  bzero(&hr, sizeof(hr));
  
  // set fd into hr.connfd
  hr.connfd = connfd; // CRAZY!!!
  get_info_from_conn(connfd, &hr, (char *)&req);

  display_http(&hr);

  bzero(req, BUF_SIZE);

  // the main path, need determine in the access
  if(strcmp(hr.path,"/") == 0) {
    printf("[MAIN_PATH]");
    strcpy(hr.path,"/index.html");
  }
  
  // get file exist status
  if(access(hr.path+1,F_OK) != 0) {
    // file not found
    // close connfd
    send_error(connfd, 404);
    return;
  }

  // in this section, file is exsiting
  // determine '/cgi' in the path
  if(is_cgi(hr.path) == 0) {
    printf("[RUN_CGI]\n");
    // run cgi
    // this method had close(connfd) operation
    // hr attached connfd
    run_cgi(&hr);
    
  } else {

    printf("[STATIC_FILE]\n");
    send_static_file(&hr);    // return static file    
  }

  // TODO: you must free memory
  // all operation about memory must free in here
  //  free(hr.content);
}

/*  */
int is_cgi(char *path) {
  return strstr(path, "/cgi/") == 0 ? -1 : 0;
}


/*
  analyze request data into http_request struct
 */
void get_info_from_conn(int connfd, http_request *hr,char *req){
  int recv_len = recv(connfd, req, BUF_SIZE, 0);
  int i,j;
  printf("Recv Length:%d\n",recv_len);
  printf("Request Info START ======================\n\n%s",req);
  printf("Request Info END ======================\n");

  // first loop , get method
  for(i = 0;i < recv_len && req[i] != ' ';i++) {
    hr->method[i] = req[i];
  }
  
  bzero(hr->path,sizeof(hr->path));
  // send loop , get path, but need jump a byte, so i++
  for(j=0,i++;i < recv_len && req[i] != ' ';i++,j++) {
    hr->path[j] = req[i];
  }

  // success get path,param
  is_had_param(hr);

  // success get content_type,content_length
  set_content_info(hr,req);
  
  // we can get the path,method,and content-length in data between 0 and 1024
  // after '\r\n\r\n' all the content body 

}

char *CONTENT_LENGTH =  "Content-Length";
char* CONTENT_TYPE =  "Content-Type";
char* CONTENT_DISPOSITION = "Content-Disposition";

void set_content_info(http_request *hr,char *reqdata) {
  
  printf("CONTENT_INFO\n");

  int i,j;
  char *line;
  char *saveptr,*tmp,*tmp2;
  char buf[64];
  char req[1024];
  bzero((char*)&req,sizeof(req));
  strcpy((char*)&req,reqdata);
  
    line = strtok_r(req, "\r\n", &saveptr);

  
  while(line != NULL) {
    //    printf("%s\n",line);

    tmp2 = strtok_r(line,": ",&tmp);
    tmp+=1; // cause value has an extra space byte

    if(strlen(hr->content_type) == 0 && strcmp(CONTENT_TYPE,tmp2) == 0) {
      strcpy(hr->content_type,tmp);
      //      printf("%s\n",hr->content_type);
    }

    if(strcmp(CONTENT_LENGTH,tmp2) == 0 ) {
      hr->content_len = atoi(tmp);
      //      printf("%s\nlen:%d\n",tmp,hr->content_len);
    }
        
    line = strtok_r(NULL, "\r\n", &saveptr);
  }


  // method POST ,set content
  // file dont implemented
  printf("POST_DATA\n");

  // print request data
  //  printf("%s\n",reqdata);
  
  // just in upload file and POST method that  we need get all of content

  if(strcmp("POST",hr->method) == 0) {
    set_http_content(hr, reqdata);
  }

  printf("SET_ END \n");

}

/* set content of http_request */
void set_http_content(http_request *hr,char *buf) {
  // create content memory space 
  hr->content = (char*)malloc(1024);
    
  if(is_file_upload(hr) == 0) {
    printf("FILE UPLOAD \n");

    // create form memory
    hr->form = (multipart_form*)(malloc(sizeof(multipart_form)));
    
    set_file_content(hr, buf);
  }else {
    char *body_start = strstr(buf,"\r\n\r\n");
    body_start+=4;
    
    strcpy(hr->content, body_start);
  }  
}

/* 
   get file content of previous http_request before 1024 bytes size
 */
void set_file_content(http_request *hr,char *buf) {
  int i, len, d_ok, start_ok, size;
  char* boundary = strstr(hr->content_type,"boundary=")+9;


  printf("boundary___len:%d\n",strlen(boundary));
  
  hr->form->boundary = (char*) malloc(128);

  strcpy(hr->form->boundary, boundary);
    
  if(strlen(boundary) <= 0) {
    error_exit("BOUNDARY ERROR");
  }
  
  printf("Boundary: %s[%d]\n",boundary,strlen(boundary));
  
  int boundary_status = 0;
  char tmp_buf[1024];
  char *line,*rest,*tmp,*tmp2;
  
  bzero(&tmp_buf,sizeof(tmp_buf));
  
  strcpy((char *)&tmp_buf,buf);
  printf("TEMP_BUF_LEN:%d\n",strlen(tmp_buf));

  set_multipart_disposition(hr,buf);

  i = 0;
  
  line = strtok_r(tmp_buf, "\r\n",&rest);
  //  printf("[%d,i=%d]FILE_LINE: %s\n",strlen(line),i,line);

  d_ok = 1;
  start_ok = 1;
    
  for(i = strlen(line); i < strlen(buf); ) {
    line = strtok_r(NULL, "\r\n",&rest);
    if(line == NULL) break;

    i+=strlen(line);
    printf("[%d,i=%d]FILE_LINE: %s\n",strlen(line),i,line);
      
    // start
    printf("STRCMP:%d\n",strcmp(line,boundary));
    
    if(strstr(line, boundary) != NULL && strlen(line) == (strlen(boundary) + 2)) {
      printf("CONTENT_START\n");
      
      start_ok = 0;
      hr->form->finished = 1;

      if(hr->content_len > 1024) {
        line = strtok_r(NULL,"\r\n",&rest);
        line = strtok_r(NULL,"\r\n",&rest);
        strcpy(hr->content, rest);

        hr->content += strlen("\n\r\n");
        
        printf("CONTENT_____________________REST3:\n%s\n",hr->content);
        
        // ok ok
        start_ok = 1;
      }
      
      continue;
      // set write content start status
    }

    // end
    // if line can find boundary, that is the end
    // end boundary just two more bytes "--"
    // if no bounadry, dont need to resolve
    if(!start_ok && strstr(line, boundary) != NULL && strlen(line) == (strlen(boundary) + 4)) {
      // break, and over
      start_ok = 1;
      hr->form->finished = 0;// cause we maybe cant finished in here
      printf("CONTENT_END\n");
      printf("CONTENT:\n%s\n",hr->content);
      break;
    }

    // start_ok = 0 , start write content
    if(!start_ok && strstr(line, CONTENT_TYPE) == NULL && strstr(line,CONTENT_DISPOSITION ) == NULL) {
      /* char *t = hr->content; */
      /* size = strlen(hr->content)+strlen(line)+64; */
      
      /* hr->content = (char*)malloc(size); */
      /* bzero(hr->content,size); */

      //      strcat(hr->content,t);
      //      strcat(hr->content,"\r\n");
      strcat(hr->content,line);
      //      strcat(hr->content,"\n");

      //      free(t);
    }
    
  }
  
  printf("--------------SET_FILE_CONTENT_END\n");
  
}


void set_multipart_disposition(http_request *hr, char*buf){
  char tmp_buf[1024];int len= -1;
  
  
  char *line,*rest,*tmp,*tmp2;
  strcpy((char *)&tmp_buf,buf);
  
  line = strtok_r((char*)&tmp_buf, "\r\n",&rest);
  while(1) {
    line = strtok_r(NULL, "\r\n",&rest);
    if(line == NULL) break;  
    // set disposition
    // use d_ok reduce determine order    

    tmp2 = strtok_r(line,": ",&tmp);
    tmp+=1; // cause value has an extra space byte

    
    // use d_ok reduce determine order    
    if( strcmp(CONTENT_DISPOSITION,tmp2) == 0) {
      //      printf("%s\n",hr->content_type);

      // create momery
      len = strlen(tmp);

      //      printf("LEN: %d\n",len);
      
      hr->form->disposition = (char*)malloc(len);
      strcpy(hr->form->disposition,tmp);

      char *start = strstr(hr->form->disposition, "name=\"");
      //      printf("FILENAME:%s[%d]\n",start,sizeof(start));

      start += strlen("name=\"");
      char *end = strstr(start, "\"");

      //      printf("Length:%d\n",end - start);

      hr->form->filename = (char *)malloc(end - start);

      // start is char pointer, you can get string with this pointer
      strncpy(hr->form->filename, start, end -start);
      // printf("FILENAME:%s[%d]\n",hr->form->filename,end - start);
      
      display_multipart_form(hr->form);
        
     }
  }

}

int is_file_upload(http_request *hr){
  char* len = strstr(hr->content_type,MULTIPART_FORM_DATA);
  
  printf("FILE STATUS: %d[%s]\n",len,len);

  return len == NULL ? 1 : 0;
}



/*
  split path and request param
 */
void is_had_param(http_request *hr) {
  bzero(hr->req_param,sizeof(hr->req_param));
  
  char *path = hr->path;
  //  printf("PATH:%s [%d]\n",path,strlen(path));
  int i,j;
  char *pre,*param;
  pre = strtok_r(path,"?",&param);
  
  /* printf("PRE: %s\n",pre); */
  /* printf("PARAM:%s\n",param); */

  strcpy(hr->path,pre);
  strcpy(hr->req_param,param);
}

/*
  port of server
  RETURNS: socket file descriptor
*/
int init_server(int port) {
  
  int sockfd = 0;
  sockfd = socket(PF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr;

  if (sockfd < -1)
    error_exit("Socket");

  // reuse system port
  int reuse = 1;
  if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse ) < 0) 
    error_exit("Setsockopt");

  // clean default info of memory( i dont know)
  bzero(&addr,sizeof addr);

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // binding
  if(bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) 
    error_exit("Binding");

  // start listening
  if(listen(sockfd, SOMAXCONN) < 0)
    error_exit("Listen");

  return sockfd;
}


void PrintServerInfo(int port) {
  printf("Server: http://127.0.0.1:%d\n",port);
}

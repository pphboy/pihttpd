#include "p_fund.h"

#define BUF_SIZE 1024

// key of request header

/*
  
*/
void start_server(int port) {
  
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
    handle_request(connfd);
  }
}

/*
  handle every request content of connection
*/
void handle_request(int connfd) {
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
  printf("Request Info:\n%s",req);

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

void set_content_info(http_request *hr,char *reqdata) {
  
  printf("\nSET_CONTENT_INFO\n");

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
    
    if(strcmp(CONTENT_TYPE,tmp2) == 0) {
      strcpy(hr->content_type,tmp);
      //      printf("%s\n",hr->content_type);
    }

    if(strcmp(CONTENT_LENGTH,tmp2) == 0) {
      hr->content_len = atoi(tmp);
      //      printf("%s\nlen:%d\n",tmp,hr->content_len);
    }
        
    line = strtok_r(NULL, "\r\n", &saveptr);
  }


  // method POST ,set content
  // file dont implemented
  printf("POST_DATA\n");

  printf("%s\n",reqdata);
  
  // just in upload file and POST method that  we need get all of content
  if(strcmp("POST",hr->method) == 0) {
    
    char *body_start = strstr(reqdata,"\r\n\r\n");
    body_start+=4;
    //    printf("BODY:%s\nLEN:%d\n",body_start,strlen(body_start));
    
    hr->content = malloc(hr->content_len);
    
    strcpy(hr->content, body_start);
    
  }

  printf("SET_ END \n");

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

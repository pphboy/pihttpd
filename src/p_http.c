/*
  About Http Information, Method, Utils
 */

#include "p_http.h"

/* 
   maybe need set Content-Length
   Atach Content Or Atach Static File
 */
void add_content(int connfd,char *content) {
  // add the '\r\n' before returned content
  send(connfd, CRLF, strlen(CRLF), 0);
  send(connfd, content, strlen(content), 0);
}

void headers(int connfd, char* http_status) {
  send(connfd, http_status, strlen(http_status), 0);
  // add server information
  send(connfd, SERVER_INFO, strlen(SERVER_INFO), 0);
}

/* set Content-Type */
void add_header_value(int connfd, char* type) {
  send(connfd, type, strlen(type), 0);
}


void send_error(int connfd,int http_status) {
  switch(http_status){
  case 404:
    headers(connfd, HTTP_NOT_FOUND);
    add_header_value(connfd,CTYPE_HTML);
    add_content(connfd, STR_NOT_FOUND);
    close(connfd);
    break;
  default:
    headers(connfd, HTTP_NOT_PROCESSED);
    add_header_value(connfd,CTYPE_HTML);
    add_content(connfd, STR_NOT_PROCESSED);
    close(connfd);
    break;
  }
}


/* 
   maybe need set Content-Length
   Atach Content Or Atach Static File
 */
void cgi_add_content(char *content) {
  // add the '\r\n' before returned content
  printf(CRLF);
  printf(content);
}

void cgi_headers(char* http_status) {
  printf(http_status);
  printf(SERVER_INFO);
}

/* set Content-Type */
void cgi_add_header_value(char* type) {
  printf(type);
}

void cgi_send_error(int http_status) {
  switch(http_status){
  case 404:
    cgi_headers(HTTP_NOT_FOUND);
    cgi_add_header_value(CTYPE_HTML);
    cgi_add_content(STR_NOT_FOUND);
    break;
  default:
    cgi_headers(HTTP_NOT_PROCESSED);
    cgi_add_header_value(CTYPE_HTML);
    cgi_add_content(STR_NOT_PROCESSED);
    break;
  }
}

void cgi_send_html(int http_status, char* content) {
  switch(http_status){
  case 200:
    cgi_headers(HTTP_OK);
    cgi_add_header_value(CTYPE_HTML);
    cgi_add_content(content);
    break;
  case 404:
    cgi_headers(HTTP_NOT_FOUND);
    cgi_add_header_value(CTYPE_HTML);
    cgi_add_content(content);
    break;
  default:
    cgi_headers(HTTP_NOT_PROCESSED);
    cgi_add_header_value(CTYPE_HTML);
    cgi_add_content(content);
    break;
  }
}



void cgi_send_json(int http_status, char* content) {
  switch(http_status){
  case 200:
    cgi_headers(HTTP_OK);
    cgi_add_header_value(CTYPE_JSON);
    cgi_add_content(content);
    break;
  case 404:
    cgi_headers(HTTP_NOT_FOUND);
    cgi_add_header_value(CTYPE_JSON);
    cgi_add_content(content);
    break;
  default:
    cgi_headers(HTTP_NOT_PROCESSED);
    cgi_add_header_value(CTYPE_JSON);
    cgi_add_content(content);
    break;
  }
}

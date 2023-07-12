#ifndef P_HTTP_H
#define P_HTTP_H

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#endif

/* 
   if request is uplaoding file, need create a multipart_form structure
 */
typedef struct {
  char* filename;
  char* disposition;
  int finished;                     /* Finished 0, Unfinished 1 */
} multipart_form;


/*
 data structure or method of the server system
*/

typedef struct  {
  int connfd; // connection file descriptor
  char method[10];
  char path[256];
  char req_param[1024]; // after '?'
  char content_type[128];
  multipart_form *form;
  int content_len;
  char *content;
} http_request;



// http method, http status

// Reponse http status
static char HTTP_OK[] =  "HTTP/1.0 200 OK\r\n";
static char HTTP_NOT_FOUND[] =  "HTTP/1.0 404 NOT FOUND\r\n";
static char HTTP_NOT_PROCESSED[] =  "HTTP/1.0 500 NOT PROCESSED\r\n";

// error content
static char STR_NOT_FOUND[] = "<html><head><title>404 Not Found</title></head><body><center><h1>404 Not Found</h1></center><hr><center>PHttpD</center></body></html>";

static char STR_NOT_PROCESSED[] = "<html><head><title>500 NOT PROCESSED</title></head><body><center><h1>500 NOT PROCESSED</h1></center><hr><center>PHttpD</center></body></html>";


// Server Information
static char SERVER_INFO[] =  "Server: PiHTTPD 0.1\r\n";

typedef struct {
  char *key; //profix
  char *value;
} STR_MAP;

/* key is postfix, value is ContentType Value */
static STR_MAP CONTENT_TYPE_ARR[] = {
  {"html","Content-Type: text/html\r\n"},
  {"xml","Content-Type: text/xml\r\n"},
  {"txt","Content-Type: text/plain\r\n"},
  {"css","Content-Type: text/css\r\n"},
  {"js","Content-Type: text/javascript\r\n"},
  {"png","Content-Type: image/png\r\n"},
  {"jpg","Content-Type: image/jpeg\r\n"},
  {"gif","Content-Type: image/gif\r\n"},
  {"json","Content-Type: application/json\r\n"},
  {"pdf","Content-Type: application/pdf\r\n"},
  {"file","Content-Type: application/octet-stream\r\n"},
  {NULL,NULL}
};

// Content-Type
/* text */
static char CTYPE_HTML[] = "Content-Type: text/html\r\n";
static char CTYPE_XML[] =  "Content-Type: text/xml\r\n";
static char CTYPE_TEXT[] = "Content-Type: text/plain\r\n";
static char CTYPE_CSS[] =  "Content-Type: text/css\r\n";
static char CTYPE_JS[] =   "Content-Type: text/javascript\r\n";
static char CTYPE_PNG[] =  "Content-Type: image/png\r\n";
static char CTYPE_JPG[] =  "Content-Type: image/jpeg\r\n";
static char CTYPE_GIF[] =  "Content-Type: image/gif\r\n";
static char CTYPE_JSON[] = "Content-Type: application/json\r\n";
static char CTYPE_PDF[] =  "Content-Type: application/pdf\r\n";
static char CTYPE_OCTET[] ="Content-Type: application/octet-stream\r\n";

static char CRLF[] = "\r\n";

void headers(int connfd,char *http_status);

void add_header_value(int conn, char* type);

void add_content(int connfd,char *hr);

void send_error(int connfd, int http_status);

/* 
   service for cgi running
 */
void cgi_headers(char *http_status);

void cgi_add_header_value(char* type);

void cgi_add_content(char *hr);

void cgi_send_error(int http_status);

void cgi_send_json(int http_status, char* content);

void cgi_send_html(int http_status, char* content);

//void cgi_send_file(int http_status, char* content);

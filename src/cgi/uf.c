#include <stdio.h>
#include <unistd.h>
#include "../p_sys.h"

int main(int argc, char *argv[]) {
  char buf[1024];
  int len = -1;
  http_request hr;
  get_cgi_req_param(argc,argv,&hr);
  
  len =  strlen(argv[argc-1])+strlen("file/");
  char *filename = (char*)malloc(len);

  cgi_send_json(200,"{\"cgi\":\"test.c\"}\n");
  
  sprintf(filename, "file/%s", argv[argc-1]);
  printf("FILENAME: %s\n",argv[argc-1]);
  printf("LOCAL:%s\n",filename);
    
  int fd =  open(filename, O_WRONLY | O_CREAT, 0644);
  int w;
  
  if(fd == -1) {
    error_exit("OPEN FILE");
  }
  display_http(&hr);
  
  perror("Child Running");

  printf("ABC\n");
  
  printf("CONTENT_LEN:%d\n",strlen(hr.content));
  
  
  //  write(fd, hr.content, strlen(hr.content));

  bzero((char*)&buf,sizeof(buf));  
  if(hr.content_len > 1024) {

    perror("START");
    len = 1024;
    while(len == 1024) {
      bzero(&buf,sizeof(buf));
      
      len = read(0, &buf, sizeof(buf));
      
      printf("LEN:%d\n",len);

      show_bytes_line_feed(buf);
      
      w = write(fd, &buf, len);
      if(w == -1) error_exit("WRITE ERROR");
      printf("READ[%d]:%s\n",len, buf);
    }
  }

  close(fd);

  return 0;
}

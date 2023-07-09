#include <stdio.h>
#include <unistd.h>
#include "../p_sys.h"

int main(int argc, char *argv[]) {
  cgi_send_json(200,"{\"cgi\":\"test.c\"}");
  char buf[1024];
  bzero(&buf, sizeof(buf));
  
  http_request hr;
  
  get_cgi_req_param(argc,argv,&hr);
  
  //  display_http(&hr);

  return 0;
}


#include <stdio.h>
#include "p_fund.h"


#define CGI_DIR "./cgi"

int main(int argc,char *argv[]){
  start_server(8082,20,40);
  return 0;
}

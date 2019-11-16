
//#include "http_request_module.h"
#include <unistd.h>
#include "HttpClient.h"


int main()
{
    //http_request_module_init();
    HttpClient aaa;

    aaa.start();
    printf("111\n");
    sleep(40);
    aaa.stop();
    sleep(10);
    return 0;
}

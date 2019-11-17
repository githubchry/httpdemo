
//#include "http_request_module.h"
#include <unistd.h>
#include "HttpClient.h"
#include "HttpServer.h"
#include "ryprint.h"

#include "ThreadPool.h"


static void thread_func(char * _uds_pack_ptr)
{
    ryDbg("%s\n", _uds_pack_ptr);
    sleep(10);
}

int main()
{
    //http_request_module_init();

    HttpClient aaa;
    HttpServer bbb("0.0.0.0", 7777);

    aaa.start();
    ryDbg("\n");
    bbb.start();
    ryDbg("\n");

    sleep(10);

    aaa.stop();
    ryDbg("\n");
    bbb.stop();
    ryDbg("\n");
    
    aaa.start();
    ryDbg("\n");
    bbb.start();

    ryDbg("\n");
    sleep(10);

    return 0;
}

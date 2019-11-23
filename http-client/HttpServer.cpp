#include "HttpServer.h"
#include "ryprint.h"
#include <signal.h>
#include <string.h>
#include <arpa/inet.h>
#include <event.h>      //for struct evkeyvalq
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/http_compat.h>

#include <cjson/cJSON.h>

HttpServer::HttpServer(const char *addr, int port)
{
    evbase = event_base_new();

    /**
     * 启动一个http server：
     * 
     * 根据socket创建一个event（同时回调函数设置为accept_socket，底层函数处理socket连接信号),
     * 然后将这个event关联到对应的event_base,之后插入到&http->sockets队列中,然后返回&http 
     */
    //httpd = evhttp_start(addr, port);
    httpd = evhttp_new(evbase);
    //创建一个非阻塞的socket，并绑定到ip/port,
	evhttp_bind_socket(httpd, addr, port);

    // 没有注册的URI会进入generic_cb回调
    evhttp_set_gencb(httpd, generic_cb, NULL);

}

HttpServer::~HttpServer()
{
    stop();
    evhttp_free(httpd);
    event_base_free(evbase);
}

void HttpServer::start()
{
    assert_param(nullptr == http_listen_thread);
    run_flag = true;
    http_listen_thread = std::make_shared<std::thread>([this]() {

        ryDbg("http server thread start.\n");

        while (run_flag)
        {
            //非阻塞模式下循环处理events
            event_base_loop(evbase, EVLOOP_NONBLOCK);
        }

        ryDbg("http server thread end..\n");
    });

}

void HttpServer::stop()
{
    assert_param(http_listen_thread && run_flag);

    if(http_listen_thread->joinable()) 
    {
        run_flag = false;
        http_listen_thread->join();
        http_listen_thread = nullptr;
    }
}

void HttpServer::generic_cb(struct evhttp_request *req, void *arg)
{
    //获取客户端请求的URI(使用evhttp_request_uri或直接req->uri)
    const char *uri = evhttp_request_uri(req);
    ryDbg("recv request %s\n", uri);

    //获取POST方法的数据
    size_t post_size = EVBUFFER_LENGTH(req->input_buffer);
    char *post_data = (char *)EVBUFFER_DATA(req->input_buffer);

    printf("[%s]\n len =%ld \n", post_data, post_size);

    char s[128] = "This is the generic buf";
    evbuffer_add(req->output_buffer, s, strlen(s));
    evhttp_send_reply(req, 200, "OK", NULL);
}

/**
 *  0 on success, -1 if the callback existed already, -2 on failure
*/
int HttpServer::addRequestHandle(const char *path, requestHandleFunc cb, void *cb_arg)
{
    assert_param_return(nullptr == http_listen_thread, -2);

    return evhttp_set_cb(httpd, path, cb, cb_arg);
}
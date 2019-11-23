#pragma once

#include <thread>
#include <atomic>

typedef void (*requestHandleFunc)(struct evhttp_request *req, void *arg);

class HttpServer
{
private:
    std::shared_ptr<std::thread> http_listen_thread = nullptr;
    std::atomic<bool> run_flag{false};
    struct evhttp *httpd = nullptr;
    struct event_base *evbase = nullptr;

    static void generic_cb(struct evhttp_request *req, void *arg);

public:
    HttpServer(const char *addr, int port);
    ~HttpServer();
    int addRequestHandle(const char *path, requestHandleFunc cb, void *cb_arg);

    void start();
    void stop();
};

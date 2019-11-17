#pragma once

#include <thread>
#include <atomic>
#include <sys/un.h>


typedef void (*udsPackHandleFunc)(char *pack_ptr, int length, struct sockaddr_un *client_addr);
    
static void uds_dgram_pack_handle_func(char * _uds_pack_ptr, size_t length, struct sockaddr_un *_client_addr);

class UdsServer
{
private:
    std::shared_ptr<std::thread> uds_listen_thread = nullptr;
    std::atomic<bool> run_flag{false};
    int sockfd = -1;
    struct sockaddr_un server_addr = { 0 };
    udsPackHandleFunc uds_handle_cb = NULL;
public:
    UdsServer(const char * path);
    ~UdsServer();
    void start(udsPackHandleFunc cb_func);
    void stop();
};


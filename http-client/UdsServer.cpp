#include "UdsServer.h"
#include "ryprint.h"
#include "rymacros.h"
#include "ThreadPool.h"

#include <sys/un.h>
#include <sys/socket.h>


#define THREAD_POOL_SIZE        4
#define REQUEST_ADDR_MAX_NUM    5
#define UDS_DGRAM_BUFFER_SIZE   1024
#define RESPONSE_DATA_LEN       1024

UdsServer::UdsServer(const char * path)
{
    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    assert_param(sockfd >= 0);

    struct sockaddr_un server_addr = { 0 };
    server_addr.sun_family = AF_UNIX;
    assert_param(strlen(path) < sizeof(server_addr.sun_path));
    strcpy(server_addr.sun_path, path);

    unlink(path);

    int ret = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret != 0)
    {
        close(sockfd);
        sockfd = -1;
        ryErr("uds bind %s fail!\n", server_addr.sun_path);
        return;
    }
    ryDbg("uds bind %s sucess!\n", server_addr.sun_path);
}

UdsServer::~UdsServer()
{
    stop();
    IF_TRUE_DO(sockfd >= 0, close(sockfd));
    
}

void UdsServer::start(udsPackHandleFunc cb_func)
{
    assert_param(false == run_flag);
    assert_param(nullptr == uds_listen_thread);
    assert_param(cb_func);

    uds_handle_cb = cb_func;
    run_flag = true;
    uds_listen_thread = std::make_shared<std::thread>([this]() {

        ryDbg("uds listen thread start.\n");

        char *recv_buf_ptr = NULL;
        recv_buf_ptr = (char *)malloc(UDS_DGRAM_BUFFER_SIZE);
        assert_param(recv_buf_ptr);
        int addr_size = -1;
        struct sockaddr_un client_addr = { 0 };

        // create thread pool with 4 worker threads
        ThreadPool pool(THREAD_POOL_SIZE);

        while (run_flag)
        {
            addr_size = sizeof(struct sockaddr_un);

            int ret = recvfrom(sockfd, recv_buf_ptr, UDS_DGRAM_BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, (socklen_t*)&addr_size);

            IF_TRUE_DO(ret <= 0, continue);

            pool.enqueue(uds_handle_cb, recv_buf_ptr, ret, &client_addr);

            usleep(1000*5); //稍作延时 确保子线程里面有时间把传进去的buf拷贝下来
        }
    });

}

void UdsServer::stop()
{
    assert_param(run_flag);
    assert_param(uds_listen_thread);

    run_flag = false;
    shutdown(sockfd,SHUT_RD);

    if(uds_listen_thread->joinable()) 
    {
        uds_listen_thread->join();
    }
    
}
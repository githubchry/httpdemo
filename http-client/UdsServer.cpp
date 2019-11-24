#include "UdsServer.h"
#include "ryprint.h"
#include "rymacros.h"
#include "ThreadPool.h"

#include <sys/socket.h>


#define THREAD_POOL_SIZE        4
#define REQUEST_ADDR_MAX_NUM    5
#define UDS_DGRAM_BUFFER_SIZE   1024
#define RESPONSE_DATA_LEN       1024

UdsServer::UdsServer(const char * path)
{
    server_addr.sun_family = AF_UNIX;
    assert_param(strlen(path) < sizeof(server_addr.sun_path));
    strcpy(server_addr.sun_path, path);
}

UdsServer::~UdsServer()
{
    stop();
}

void UdsServer::start(udsMsgHandleFunc cb_func)
{
    assert_param(false == run_flag);
    assert_param(nullptr == uds_listen_thread);
    assert_param(cb_func);
    assert_param(strlen(server_addr.sun_path) > 0);

    uds_handle_cb = cb_func;
    run_flag = true;
    uds_listen_thread = std::make_shared<std::thread>([this]() {

        ryDbg("uds listen thread start.\n");

        sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
        assert_param(sockfd >= 0);

        unlink(server_addr.sun_path);

        int ret = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (ret != 0)
        {
            close(sockfd);
            sockfd = -1;
            ryErr("uds bind %s fail!\n", server_addr.sun_path);
            return;
        }
        ryDbg("uds bind %s sucess!\n", server_addr.sun_path);

        struct msghdr socket_msg;
        struct cmsghdr *ctrl_msg;
        struct iovec iov[1];
        char ctrl_data[CMSG_SPACE(sizeof(sockfd))];

        char *recv_buf_ptr = NULL;
        recv_buf_ptr = (char *)malloc(UDS_DGRAM_BUFFER_SIZE);
        assert_param(recv_buf_ptr);
        struct sockaddr_un client_addr = { 0 };

        socket_msg.msg_name = &client_addr;
        socket_msg.msg_namelen = sizeof(struct sockaddr_un);
        iov[0].iov_base = recv_buf_ptr;
        iov[0].iov_len = UDS_DGRAM_BUFFER_SIZE;
        socket_msg.msg_iov = iov;
        socket_msg.msg_iovlen = 1;
        socket_msg.msg_control = ctrl_data;
        socket_msg.msg_controllen = sizeof(ctrl_data);
        
         while (run_flag)
        {
            sm.lock();
            //大写加粗标红：每次recvmsg之后addr_size都会被改变为实际的sockaddr大小 所以必须重置一下
            socket_msg.msg_namelen = sizeof(struct sockaddr_un);

            ret = recvmsg(sockfd, &socket_msg, 0);
            IF_TRUE_DO(ret <= 0, continue);

            std::thread(uds_handle_cb, &socket_msg, ret, &sm).detach();
        }
        ryDbg("uds listen thread end.\n");
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
        uds_listen_thread = nullptr;
    }
    
    if (sockfd >= 0)
    {
        close(sockfd);
        sockfd = -1;
    }
}
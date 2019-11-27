#pragma once

#include <thread>
#include <atomic>
#include <sys/un.h>

// 自旋锁 要求在多核多线程中使用 效率比普通锁更快
// 其实使用C++11自带std::mutex互斥量就可以，这里纯粹为了炫技
class spin_mutex
{
    std::atomic_flag flag = ATOMIC_FLAG_INIT;

public:
    spin_mutex() = default;
    spin_mutex(const spin_mutex &) = delete;
    spin_mutex &operator=(const spin_mutex &) = delete;
    void lock()
    {
        while (flag.test_and_set(std::memory_order_acquire));
    }
    void unlock()
    {
        flag.clear(std::memory_order_release);
    }
};

typedef void (*udsMsgHandleFunc)(const struct msghdr *msg_ptr, int length, spin_mutex *sm_ptr);

class UdsServer
{
private:
    std::shared_ptr<std::thread> uds_listen_thread = nullptr;
    std::atomic<bool> run_flag{false};
    int sockfd = -1;
    struct sockaddr_un server_addr = { 0 };
    udsMsgHandleFunc uds_handle_cb = NULL;
    spin_mutex sm;

public : UdsServer(const char *path);
    ~UdsServer();
    void start(udsMsgHandleFunc cb_func);
    void stop();
};


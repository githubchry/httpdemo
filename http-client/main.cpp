
#include "ryprint.h"
#include "rymacros.h"
#include "ThreadPool.h"
#include "uds_pack_handle.h"

#include <curl/curl.h>

#include <sys/un.h>
#include <sys/socket.h>
#include <stdlib.h>

#define UDS_DGRAM_SERVER_PATH  "/home/chry/code/github/IPCDemo/build/uds-dgram-server"
#define THREAD_POOL_SIZE  4
#define REQUEST_ADDR_MAX_NUM    5

#define RESPONSE_DATA_LEN 4096

//用来接收服务器一个buffer C++语法
typedef struct response_data
{
    response_data() {
        memset(data, 0, RESPONSE_DATA_LEN);
        data_len = 0;
    }

    char data[RESPONSE_DATA_LEN];
    int data_len;

}response_data_t;


//处理从服务器返回的数据，将数据拷贝到arg中
static size_t deal_response(void *ptr, size_t n, size_t m, void *arg)
{
    int count = m*n;

    response_data_t *response_data = (response_data_t*)arg;

    if(response_data->data_len + count > RESPONSE_DATA_LEN)
    {
        printf("response data is too big!\n");
        return count;
    }

    memcpy(response_data->data+response_data->data_len, ptr, count);

    response_data->data_len += count;

    return count;
}


int init_uds_dgram_sock()
{
    int sockfd;
    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (-1 == sockfd)
    {
        ryDbg("create socket error!\n");
        exit(-1);
    }

    struct sockaddr_un server_addr = { 0 };
    if (strlen(UDS_DGRAM_SERVER_PATH) >= sizeof(server_addr.sun_path))
    {
        ryDbg("uds path length too long: %s!\n", UDS_DGRAM_SERVER_PATH);
        close(sockfd);
        exit(-2);
    }
    unlink(UDS_DGRAM_SERVER_PATH);
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, UDS_DGRAM_SERVER_PATH);

    if (0 != bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        ryDbg("uds bind fail!\n");
        close(sockfd);
        exit(-3);
    }

    return sockfd;
}

void destroy_uds_dgram_sock(int &sockfd)
{
    if (sockfd >= 0)
    {
        close(sockfd);
        sockfd = -1;
    }
    
    unlink(UDS_DGRAM_SERVER_PATH);
}

static char http_request_addr[REQUEST_ADDR_MAX_NUM][64] = 
{
    "http://10.6.1.39:8888",
    "http://" SERVER_ADDR ":" SERVER_PORT ,
};

static http_request_api_t http_request_api_table[] = 
{
    
    { ___test,      "/login" },

    //以下协议的发送方是平台系统，接收方是设备
    { _http_msg_type_get_access_token,      "/ATMAlarm/GetAccessToken" },
    { _http_msg_type_release_token,         "/ATMAlarm/ReleaseToken" },
    { _http_msg_type_set_center_url,        "/ATMAlarm/SetCenterURL" },
    { _http_msg_type_get_device_time,       "/ATMAlarm/GetDeviceTime" },
    { _http_msg_type_set_device_time,       "/ATMAlarm/SetDeviceTime" },
    { _http_msg_type_restart_device,        "/ATMAlarm/RestartDevice" },
    { _http_msg_type_arm_chn,               "/ATMAlarm/ArmChn"},
    { _http_msg_type_dis_arm_chn,           "/ATMAlarm/DisArmChn" },

    //以下协议的发送方是设备，接收方是平台系统
    { _http_msg_type_statu_monitor_url,     "/ATMAlarm/statuMonitorURL" },
    { _http_msg_type_record_monitor_url,    "/ATMAlarm/recordMonitorURL" },
    { _http_msg_type_get_device_all_statu,  "/ATMAlarm/GetAccessToken" },
    
};

void uds_dgram_pack_handle_func(char * _uds_pack_ptr, size_t length, struct sockaddr_un *_client_addr)
{
    struct sockaddr_un client_addr = { 0 };
    memcpy(&client_addr, _client_addr, sizeof(struct sockaddr_un));

    assert_param(_uds_pack_ptr && length > 0);

    char *recv_buf_ptr = (char *)malloc(length);
    assert_param(recv_buf_ptr);

    memcpy(recv_buf_ptr, _uds_pack_ptr, length);

    uds_pack_header_t *udspack_header_ptr = (uds_pack_header_t *)recv_buf_ptr;

    ryDbg("uds handle msg type %d from [%s]!\n", udspack_header_ptr->msgtype, client_addr.sun_path);

    const char * api_suffix = NULL;
    for (size_t i = 0; i < SIZE_OF_ARRAY(http_request_api_table); i++)
    {
        if (udspack_header_ptr->msgtype == http_request_api_table[i].msgtype)
        {
            /* code */
            api_suffix = http_request_api_table[i].api_suffix;
            ryDbg("msg type %d => api[%s]!\n", udspack_header_ptr->msgtype, api_suffix);
        }
        
    }
    
    assert_param(api_suffix);

    //初始化curl句柄
    CURL* curl = curl_easy_init();
    char http_request_url[128];
    for (size_t i = 0; i < REQUEST_ADDR_MAX_NUM; i++)
    {
        IF_FALSE_DO(strlen(http_request_addr[i]) > 0, continue);
        memset(http_request_url, 0, sizeof(http_request_url));

        strcat(http_request_url, http_request_addr[i]);
        strcat(http_request_url, api_suffix);
        
        ryDbg("http_request_url [%s]!\n", http_request_url);

        assert_param(curl);

        //设置curl url
        curl_easy_setopt(curl, CURLOPT_URL, http_request_url);

        //客户端忽略CA证书认证 用于https跳过证书认证
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

        //查看curl内部打印开关
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        //2 开启post请求开关
        curl_easy_setopt(curl, CURLOPT_POST, 1);

        //3 添加post数据
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, udspack_header_ptr->m_data);

        //4 设定一个处理服务器响应的回调函数
        //此函数读取libcurl发送数据后的返回信息，如果不设置此函数，那么返回值将会输出到控制台，影响程序性能
        //这个设置的回调函数的调用是在每次socket接收到数据之后，并不是socket接收了所有的数据，然后才调用设定的回调函数
        //如果读取的网页或数据特别大,那么这个函数会多次调用,所以数据必须累加起来.
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, deal_response);

        //5 给上一步的设置回调函数传递一个形参 用来存放从服务器返回的数据
        response_data_t responseData; 
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

        //6 向服务器发送请求,等待服务器的响应 在超时时间内阻塞，直到服务器有返回
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            printf("curl_easy_perform err %d\n", res);
            continue;
        }


        if(responseData.data_len > 0 && strlen(client_addr.sun_path) > 0)
        {
            ryDbg("responseData [%s]!\n", responseData.data);
            //把响应转发给 _client_addr
            int sockfd;
            sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
            if (-1 == sockfd)
            {
                ryDbg("create socket error!\n");
                continue;
            }
            ryDbg("sendto [%s]!\n", client_addr.sun_path);
            int ret = sendto(sockfd, responseData.data, responseData.data_len, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr_un));

            close(sockfd);
            ryDbg("sendto ret %d [%s]!\n", ret,  client_addr.sun_path);
        }


    }
    
    curl_easy_cleanup(curl);

}


int main()
{
    //当多个线程，同时进行curl_easy_init时，由于会调用非线程安全的curl_global_init，因此导致崩溃。
    //应该在主线程优先调用curl_global_init进行全局初始化。再在线程中使用curl_easy_init。
    curl_global_init(CURL_GLOBAL_ALL);

    // create thread pool with 4 worker threads
    ThreadPool pool(THREAD_POOL_SIZE);

    int sockfd = -1;
    sockfd = init_uds_dgram_sock();
    assert_param_return(sockfd >= 0, -1);

    char *recv_buf_ptr = NULL;
    recv_buf_ptr = (char *)malloc(UDS_DGRAM_BUFFER_SIZE);
    assert_param_return(recv_buf_ptr, -2);

    int ret = -1;
    int addr_size = -1;
    struct sockaddr_un client_addr = { 0 };

    while (1)
    {
        memset(&client_addr, 0, sizeof(struct sockaddr_un));
        addr_size = sizeof(struct sockaddr_un);

        ret = recvfrom(sockfd, recv_buf_ptr, UDS_DGRAM_BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, (socklen_t*)&addr_size);

        IF_TRUE_DO(ret <= 0, continue);

        pool.enqueue(uds_dgram_pack_handle_func, recv_buf_ptr, ret, &client_addr);

        usleep(1000*5); //稍作延时 确保子线程里面有时间把传进去的buf拷贝下来


    }

    destroy_uds_dgram_sock(sockfd);

    curl_global_cleanup();
    return 0;
}

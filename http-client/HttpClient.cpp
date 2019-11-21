#include "HttpClient.h"
#include "ThreadPool.h"
#include "ryprint.h"
#include "rymacros.h"

#include <curl/curl.h>
#include <sys/un.h>



#define THREAD_POOL_SIZE        4
#define REQUEST_ADDR_MAX_NUM    5
#define UDS_DGRAM_BUFFER_SIZE   1024
#define RESPONSE_DATA_LEN       1024

#define UDS_DGRAM_SERVER_PATH  "/home/chry/codes/httpdemo/build/uds-dgram-server"
#define UDS_DGRAM_BUFFER_SIZE  1024

#define SERVER_ADDR    "127.0.0.1"
#define SERVER_PORT    "7777"

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
//用来接收服务器一个buffer
typedef struct response_data
{
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

static void post_http_request(const char *api_suffix, const char *request_data, struct sockaddr_un *client_addr)
{
    //初始化curl句柄
    CURL* curl = curl_easy_init();
    assert_param(curl);

    //客户端忽略CA证书认证 用于https跳过证书认证
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

    //查看curl内部打印开关
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    //2 开启post请求开关
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    //3 添加post数据
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_data);

    //4 设定一个处理服务器响应的回调函数
    //此函数读取libcurl发送数据后的返回信息，如果不设置此函数，那么返回值将会输出到控制台，影响程序性能
    //这个设置的回调函数的调用是在每次socket接收到数据之后，并不是socket接收了所有的数据，然后才调用设定的回调函数
    //如果读取的网页或数据特别大,那么这个函数会多次调用,所以数据必须累加起来.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, deal_response);

    //5 给上一步的设置回调函数传递一个形参 用来存放从服务器返回的数据
    response_data_t responseData; 
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2);  // 设置连接超时
    
    char http_request_url[128];

    for (size_t i = 0; i < REQUEST_ADDR_MAX_NUM; i++)
    {
        IF_FALSE_DO(strlen(http_request_addr[i]) > 0, continue);
        memset(http_request_url, 0, sizeof(http_request_url));

        strcat(http_request_url, http_request_addr[i]);
        strcat(http_request_url, api_suffix);
        
        ryDbg("http_request_url [%s]!\n", http_request_url);

        //设置curl url
        curl_easy_setopt(curl, CURLOPT_URL, http_request_url);

        responseData.data_len = 0;

        //6 向服务器发送请求,等待服务器的响应 在超时时间内阻塞，直到服务器有返回
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            printf("curl_easy_perform err %d\n", res);
            continue;
        }


        if(responseData.data_len > 0 && strlen(client_addr->sun_path) > 0)
        {
            ryDbg("responseData [%s]!\n", responseData.data);
            //把响应转发给 client_addr
            int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
            IF_TRUE_DO(sockfd < 0, continue);
            
            int ret = sendto(sockfd, responseData.data, responseData.data_len, 0, (struct sockaddr *)client_addr, sizeof(struct sockaddr_un));
            ryDbg("sendto ret %d [%s]!\n", ret,  client_addr->sun_path);
            close(sockfd);
        }
    }
    
    curl_easy_cleanup(curl);

}

HttpClient::HttpClient(/* args */)
{
    //当多个线程，同时进行curl_easy_init时，由于会调用非线程安全的curl_global_init，因此导致崩溃。
    //应该在主线程优先调用curl_global_init进行全局初始化。再在线程中使用curl_easy_init。
    curl_global_init(CURL_GLOBAL_ALL);

    udsServer = new UdsServer(UDS_DGRAM_SERVER_PATH);
}

HttpClient::~HttpClient()
{
    curl_global_cleanup();
}

void HttpClient::start()
{
    udsServer->start(uds_dgram_pack_handle_func);
}

void HttpClient::stop()
{
    ryDbg("\n");
    udsServer->stop();
    ryDbg("\n");
}

void HttpClient::uds_dgram_pack_handle_func(char * _uds_pack_ptr, int length, struct sockaddr_un *_client_addr)
{
    struct sockaddr_un client_addr;
    memcpy(&client_addr, _client_addr, sizeof(struct sockaddr_un));

    assert_param(_uds_pack_ptr && length > 0);

    char *uds_pack_ptr = (char *)malloc(length);
    assert_param(uds_pack_ptr);
    memcpy(uds_pack_ptr, _uds_pack_ptr, length);

    uds_pack_header_t *udspack_header_ptr = (uds_pack_header_t *)uds_pack_ptr;

    ryDbg("uds handle msg type %d from [%s]!\n", udspack_header_ptr->msgtype, client_addr.sun_path);

    const char * api_suffix = NULL;
    for (size_t i = 0; i < SIZE_OF_ARRAY(http_request_api_table); i++)
    {
        if (udspack_header_ptr->msgtype == http_request_api_table[i].msgtype)
        {
            api_suffix = http_request_api_table[i].api_suffix;
        }
    }
    
    assert_param(api_suffix);

    ryDbg("msg type %d => api[%s]!\n", udspack_header_ptr->msgtype, api_suffix);

    post_http_request(api_suffix, udspack_header_ptr->m_data, &client_addr);
}

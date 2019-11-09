//
// Created by ldw on 2018/11/8.
//
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <string.h>

#define SERVER_ADDR    "127.0.0.1"
#define SERVER_PORT    "7777"

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
size_t deal_response(void *ptr, size_t n, size_t m, void *arg)
{
    int count = m*n;

    response_data_t *response_data = (response_data_t*)arg;

    memcpy(response_data->data+response_data->data_len, ptr, count);

    response_data->data_len += count;

    printf("response_data %d\n", response_data->data_len);

    return response_data->data_len;
}

#define POSTDATA "{\"username\":\"gailun\",\"password\":\"123123\",\"driver\":\"yes\"}"

int main()
{
    //当多个线程，同时进行curl_easy_init时，由于会调用非线程安全的curl_global_init，因此导致崩溃。
    //应该在主线程优先调用curl_global_init进行全局初始化。再在线程中使用curl_easy_init。
    curl_global_init(CURL_GLOBAL_ALL);

    //初始化curl句柄
    CURL* curl = curl_easy_init();
    if(curl == NULL) 
    {
        return -1;
    }

    //封装一个数据协议
    /*
       ====给服务端的协议====
     http://ip:port/login [json_data]
    {
        username: "gailun",
        password: "123123",
        driver:   "yes"
    }
    */
    //（1）封装一个json字符串
    char *post_str = nullptr;
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "username", "ldw");
    cJSON_AddStringToObject(root, "password", "123123");
    cJSON_AddStringToObject(root, "driver", "yes");

    post_str = cJSON_Print(root);
    cJSON_Delete(root);
    root = NULL;

    //(2) 向服务器发送http请求 其中post数据 json字符串
    //1 设置curl url
    curl_easy_setopt(curl, CURLOPT_URL, "http://" SERVER_ADDR ":" SERVER_PORT "/login");

    //客户端忽略CA证书认证 用于https跳过证书认证
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

    //查看curl内部打印开关
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    //线程使用libcurl访问时，如果设置了超时时间，而libcurl库不会为这个超时信号做任何处理
    //信号产生而没有信号句柄处理，可能导致程序退出。用以下选项禁止访问超时的时候抛出超时信号。
    //curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);   //可用于开启毫秒级的超时设置
    //curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 5000);      //整个通信周期最长时间，包括了下面的连接超时
    //curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 2000);  // 设置连接超时

    //设置连接超时
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);      //整个通信周期最长时间，包括了下面的连接超时
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2);  // 设置连接超时

    //2 开启post请求开关
    curl_easy_setopt(curl, CURLOPT_POST, 1);

    //3 添加post数据
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_str);

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
        return 1;
    }
    //服务器返回CURLE_OK表示一切OK，数据已经完整的发过来了，可以去处理，如果数据量很大，注意此前回调里面需要拼包


    curl_easy_cleanup(curl);
    curl_global_cleanup();
    //（3）  处理服务器响应的数据 此刻的responseData就是从服务器获取的数据
    /*
      //成功
    {
        result: "ok",
    }
    //失败
    {
        result: "error",
        reason: "why...."
    }
     *
     * */
    //(4) 解析服务器返回的json字符串
    //cJSON *root;
    root = cJSON_Parse(responseData.data);

    cJSON *result = cJSON_GetObjectItem(root, "result");
    if(result && strcmp(result->valuestring, "ok") == 0) {
            printf("data:%s\n",responseData.data);
        //登陆成功
        return 0;

    }
    else {
        //登陆失败
        cJSON* reason = cJSON_GetObjectItem(root, "reason");
        if (reason) {
            //已知错误
           return 1;

        }
        else {
            //未知的错误
          return 1;
        }

        return 1;
    }

    return 0;
}

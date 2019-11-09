#include <stdio.h>
#include <stdlib.h>
#include <string.h>     //for strcat
#include <signal.h>
#include <unistd.h>     //for sleep

#include <event.h>      //for struct evkeyvalq
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/http_compat.h>

#include <cjson/cJSON.h>

#define HTTPD_SIGNATURE   "chry httpd v0.1"
#define ADDR    "0.0.0.0"
#define PORT    7777
#define TIMEOUT 120


//处理模块
void httpd_handler(struct evhttp_request *req, void *arg) 
{
    char output[2048] = "\0";
    char tmp[1024];

    //获取客户端请求的URI(使用evhttp_request_uri或直接req->uri)
    const char *uri;
    uri = evhttp_request_uri(req);
#if 0
    sprintf(tmp, "uri=%s\n", uri);//  /data?cmd=new...
    strcat(output, tmp);
#endif

    sprintf(tmp, "uri=%s\n", req->uri);
    strcat(output, tmp);

    //decoded uri
    char *decoded_uri;
    decoded_uri = evhttp_decode_uri(uri);
    sprintf(tmp, "decoded_uri=%s\n", decoded_uri);// /data?cmd= newFile ...
    strcat(output, tmp);

    //http://127.0.0.1:8080/username=gailun&passwd=123123

    //解析URI的参数(即GET方法的参数)
    struct evkeyvalq params;//key ---value, key2--- value2//  cmd --- newfile  fromId == 0
    //将URL数据封装成key-value格式,q=value1, s=value2
    evhttp_parse_query(decoded_uri, &params);

    //得到q所对应的value
    sprintf(tmp, "username=%s\n", evhttp_find_header(&params, "username"));
    strcat(output, tmp);
    //得到s所对应的value
    sprintf(tmp, "passwd=%s\n", evhttp_find_header(&params, "passwd"));
    strcat(output, tmp);

    free(decoded_uri);

    //获取POST方法的数据
    char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);
    sprintf(tmp, "post_data=%s\n", post_data);
    strcat(output, tmp);


    /*
       具体的：可以根据GET/POST的参数执行相应操作，然后将结果输出
       ...
     */
    //入库



    /* 输出到客户端 */

    //HTTP header
    evhttp_add_header(req->output_headers, "Server", HTTPD_SIGNATURE);
    evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=UTF-8");
    evhttp_add_header(req->output_headers, "Connection", "close");

    //输出的内容
    struct evbuffer *buf;
    buf = evbuffer_new();
    evbuffer_add_printf(buf, "It works!\n%s\n", output);

    //将封装好的evbuffer 发送给客户端
    evhttp_send_reply(req, HTTP_OK, "OK", buf);

    evbuffer_free(buf);

}



void login_handler(struct evhttp_request *req, void *arg) 
{
    printf("got connection \n");

    char request_data[4096] = {0};

    //获取POST方法的数据
    size_t post_size = EVBUFFER_LENGTH(req->input_buffer);
    char *post_data = (char *) EVBUFFER_DATA(req->input_buffer);
    memcpy(request_data, post_data, post_size);
    printf("post_data = [%s]\nlen =%ld\n", post_data, post_size);

    //根据协议解析json数据
    /*
    ====给服务端的协议====
    http://ip:port/login [json_data]
    {
        username: "gailun",
        password: "123123",
        driver:   "yes"
    }
    */
    char username[256] = {0};
    char password[256] = {0};
    char isDriver[10] = {0};

    cJSON *root = cJSON_Parse(request_data);

    cJSON* username_obj = cJSON_GetObjectItem(root, "username");
    strcpy(username, username_obj->valuestring);

    cJSON* password_obj = cJSON_GetObjectItem(root, "password");
    strcpy(password, password_obj->valuestring);

    cJSON* isDriver_obj = cJSON_GetObjectItem(root, "driver");
    strcpy(isDriver, isDriver_obj->valuestring);

    printf("username = %s, password = %s, isDriver = %s\n", username, password, isDriver);
    cJSON_Delete(root);
    printf("----\n");


    //查询数据库 得到查询结果


    //给客户端回复一个响应结果:{"result":"ok"}
    cJSON*response_root = cJSON_CreateObject();
    cJSON_AddStringToObject(response_root, "result", "ok");
    char *response_str = cJSON_Print(response_root);


    /* 响应给客户端 */

    //HTTP header
    evhttp_add_header(req->output_headers, "Server", HTTPD_SIGNATURE);
    evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=UTF-8");
    evhttp_add_header(req->output_headers, "Connection", "close");

    //输出的内容
    struct evbuffer *buf;
    buf = evbuffer_new();
    evbuffer_add_printf(buf, "%s", response_str);

    //将封装好的evbuffer 发送给客户端
    evhttp_send_reply(req, HTTP_OK, "OK", buf);

    evbuffer_free(buf);

    cJSON_Delete(response_root);
    free(response_str);
}

void generic_cb(struct evhttp_request* req, void* arg)
{
    char s[128] = "This is the generic buf";
    evbuffer_add(req->output_buffer, s, strlen(s));
    evhttp_send_reply(req, 200, "OK", NULL);
}


//当向进程发出SIGTERM/SIGHUP/SIGINT/SIGQUIT的时候，终止event的事件侦听循环
void signal_handler(int sig) 
{
    switch (sig)
    {
    case SIGTERM:
    case SIGHUP:
    case SIGQUIT:
    case SIGINT:
        // 终止侦听event_dispatch()的事件侦听循环，执行之后的代码
        event_loopbreak();  
        break;
    
    default:
        break;
    }
}

int main(int argc, char *argv[]) 
{
    /* 对会导致进程结束的四个信号进行捕获处理 */
    //SIGHUP 信号在用户终端连接(正常或非正常)结束时发出, 系统对SIGHUP信号的默认处理是终止收到该信号的进程。可以用daemon方式解决
    signal(SIGHUP, signal_handler);
    //SIGTERM是不带参数时kill发送的信号，意思是要进程终止运行，但执行与否还得看进程是否支持。但是SIGKILL信号不同，它可以被捕获和解释（或忽略）。
    signal(SIGTERM, signal_handler);    
    //SIGINT中断信号，终端在用户按下CTRL+C发送到前台进程。默认行为是终止进程。
    signal(SIGINT, signal_handler);
    //SIGQUIT中断信号，终端在用户按下CTRL+\发送到前台进程。默认情况下进程因收到SIGQUIT退出时会产生core文件，在这个意义上类似于一个程序错误信号。
    signal(SIGQUIT, signal_handler);

    /* 使用libevent创建HTTP Server */

    //初始化event API 底层会生成一个反应堆实例 event_base
    event_init();

    /**
     * 启动一个http server：
     * 创建一个非阻塞的socket，并绑定到ip/port,
     * 根据socket创建一个event（同时回调函数设置为accept_socket，底层函数处理socket连接信号),
     * 然后将这个event关联到对应的event_base,之后插入到&http->sockets队列中,然后返回&http 
     */
    struct evhttp *httpd = evhttp_start(ADDR, PORT);

    //设置请求超时时间(秒)
    evhttp_set_timeout(httpd, TIMEOUT);

    //也可以为特定的URI指定callback
    evhttp_set_cb(httpd, "/", httpd_handler, NULL);
    evhttp_set_cb(httpd, "/login", login_handler, NULL);

    // 没有注册的URI会进入generic_cb回调
    evhttp_set_gencb(httpd, generic_cb, NULL);

    //循环处理events
    event_dispatch();

    evhttp_free(httpd);

    return 0;
}

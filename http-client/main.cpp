
//#include "http_request_module.h"
#include <unistd.h>
#include "HttpClient.h"
#include "HttpServer.h"
#include "ryprint.h"

#include <event.h> 
#include <event2/http.h>
#include <event2/http_struct.h>

#include <cjson/cJSON.h>

#include <sys/stat.h>
#include <fcntl.h>


void login_handler(struct evhttp_request *req, void *arg)
{
    printf("got connection \n");

    char request_data[4096] = {0};

    //获取POST方法的数据
    size_t post_size = EVBUFFER_LENGTH(req->input_buffer);
    char *post_data = (char *)EVBUFFER_DATA(req->input_buffer);
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

    cJSON *username_obj = cJSON_GetObjectItem(root, "username");
    strcpy(username, username_obj->valuestring);

    cJSON *password_obj = cJSON_GetObjectItem(root, "password");
    strcpy(password, password_obj->valuestring);

    cJSON *isDriver_obj = cJSON_GetObjectItem(root, "driver");
    strcpy(isDriver, isDriver_obj->valuestring);

    printf("username = %s, password = %s, isDriver = %s\n", username, password, isDriver);
    cJSON_Delete(root);
    printf("----\n");

    //查询数据库 得到查询结果

    //给客户端回复一个响应结果:{"result":"ok"}
    cJSON *response_root = cJSON_CreateObject();
    cJSON_AddStringToObject(response_root, "result", "ok");
    char *response_str = cJSON_Print(response_root);

    /* 响应给客户端 */

    //HTTP header
    evhttp_add_header(req->output_headers, "Server", __FILE__);
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

void small_file_handler(struct evhttp_request *req, void *arg)
{
    /* 响应给客户端 */

    int fd = -1;
    struct stat st;
    if ((fd = open("1.jpg", O_RDONLY)) < 0)
    {
        ryErr("open 1.jpg faile\n");
        evhttp_send_error(req, HTTP_NOTFOUND, "File was not found");
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        /* Make sure the length still matches, now that we opened the file :/ */
        evhttp_send_error(req, HTTP_NOTFOUND, "File was not found");
        return;
    }

    //输出的内容
    struct evbuffer *buf = evbuffer_new();
    assert_param(buf);

    evhttp_add_header(req->output_headers, "Content-Type", "image/jpeg");
    evbuffer_add_file(buf, fd, 0, st.st_size);

    evhttp_send_reply(req, 200, "OK", buf);

    close(fd);
    evbuffer_free(buf);
}
/*
//	video/mpeg4
void big_file_handler(struct evhttp_request *req, void *arg)
{
    char request_data[4096] = {0};

    /响应给客户端

    //HTTP header
    evhttp_add_header(req->output_headers, "Server", __FILE__);
    evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=UTF-8");
    evhttp_add_header(req->output_headers, "Connection", "close");

    //输出的内容
    struct evbuffer *buf;
    buf = evbuffer_new();
    evbuffer_add_printf(buf, "%s", response_str);

    //将封装好的evbuffer 发送给客户端
    evhttp_send_reply_start(req, HTTP_OK, "OK");

    evhttp_send_reply_chunk_with_cb(req, buf);

    evhttp_send_reply_end(req);

    evbuffer_free(buf);
}
*/
int main()
{
    //http_request_module_init();

    HttpClient aaa;
    HttpServer bbb("0.0.0.0", 7777);

    bbb.addRequestHandle("/login", login_handler, nullptr);
    bbb.addRequestHandle("/1.jpg", small_file_handler, nullptr);
    //bbb.addRequestHandle("/chrome.mp4", big_file_handler, nullptr);

    aaa.start();
    bbb.start();

    sleep(100);


    return 0;
}

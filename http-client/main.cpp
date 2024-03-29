
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
    ("got connection \n");
// 解析http协议头 从里面获取文件的名称和大小  然后保存到本地
    struct evkeyvalq *headers = evhttp_request_get_input_headers(req);
    struct evkeyval *header;

    
    for (header = headers->tqh_first; header;
         header = header->next.tqe_next)
    {
        ryDbg("  %s: %s\n", header->key, header->value);
    }


    char request_data[4096] = {0};

    //获取POST方法的数据
    size_t post_size = EVBUFFER_LENGTH(req->input_buffer);
    char *post_data = (char *)EVBUFFER_DATA(req->input_buffer);
    memcpy(request_data, post_data, post_size);
    ryDbg("post_data = [%s]\nlen =%ld\n", post_data, post_size);

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

    ryDbg("username = %s, password = %s, isDriver = %s\n", username, password, isDriver);
    cJSON_Delete(root);
    ryDbg("----\n");

    //查询数据库 得到查询结果

    //给客户端回复一个响应结果:{"result":"ok"}
    bool sucess = false;
    if (0 == strcmp(password, "123123"))
    {
        sucess = true;
    }
    
    cJSON *response_root = cJSON_CreateObject();
    cJSON_AddStringToObject(response_root, "result", sucess?"ok":"fail");
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

void send_small_file_handler(struct evhttp_request *req, void *arg)
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

struct chunk_req_state
{
    struct evhttp_request *req;
    FILE *fp;
    char *seg_ptr;
    int seg_len;
};

static void http_chunked_trickle_cb(evutil_socket_t fd, short events, void *arg)
{
    struct evbuffer *evb = evbuffer_new();
    struct chunk_req_state *state = (chunk_req_state*)arg;
    struct timeval when = {0, 0};

    int ret = fread(state->seg_ptr, 1, state->seg_len, state->fp);
    ryErr("%d\n", ret);
    if (ret > 0)
    {
        evbuffer_add(evb, state->seg_ptr, ret);
        evhttp_send_reply_chunk(state->req, evb);
        evbuffer_free(evb);

        event_base_once(evhttp_connection_get_base(state->req->evcon), -1, EV_TIMEOUT,
                        http_chunked_trickle_cb, state, &when);
    }
    else
    {
        evbuffer_free(evb);
        evhttp_send_reply_end(state->req);

        free(state->seg_ptr);

        fclose(state->fp);

        free(state);
    }
}
//*
// https://www.jianshu.com/p/5111cfb4b137
//	video/mpeg4
void send_big_file_handler(struct evhttp_request *req, void *arg)
{
    struct chunk_req_state *state = (struct chunk_req_state *)malloc(sizeof(struct chunk_req_state));

    //HTTP header
    evhttp_add_header(req->output_headers, "Server", __FILE__);
    evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=UTF-8");
    evhttp_add_header(req->output_headers, "Connection", "close");

    FILE *fp = NULL;
    fp = fopen("chrome.mp4", "rb");
    if (NULL == fp)
    {
        ryErr("can't not open %s\n", "chrome.mp4");
        evhttp_send_error(req, HTTP_NOTFOUND, "File was not found");
        return;
    }
    ryErr(" \n");

    int seg_len = 1024;
    char *seg_ptr = (char*)malloc(seg_len);
    assert_param(seg_ptr);

    state->req = req;

    state->fp = fp;
    state->seg_len = seg_len;
    state->seg_ptr = seg_ptr;

    //将封装好的evbuffer 发送给客户端
    evhttp_send_reply_start(req, HTTP_OK, "OK");

    /* but trickle it across several iterations to ensure we're not
	 * assuming it comes all at once */
    struct timeval when = {0, 0};
    event_base_once(evhttp_connection_get_base(req->evcon), -1, EV_TIMEOUT, http_chunked_trickle_cb, state, &when);
    
}
//*/


void recv_file_handler(struct evhttp_request *req, void *arg)
{
    // 解析http协议头 从里面获取文件的名称和大小  然后保存到本地
    struct evkeyvalq *headers = evhttp_request_get_input_headers(req);
    struct evkeyval *header;

    char file_name[128] = { 0 };
    int file_size = -1;
    
    for (header = headers->tqh_first; header;
         header = header->next.tqe_next)
    {
        if (0 == strcmp(header->key, "File name"))
        {
            strcpy(file_name, header->value);
        }
        else if (0 == strcmp(header->key, "File size"))
        {
            file_size = atoi(header->value);
        }
        
        
        ryDbg("  %s: %s\n", header->key, header->value);
    }

    //获取POST方法的数据
    unsigned int post_size = EVBUFFER_LENGTH(req->input_buffer);
    char *post_data = (char *)EVBUFFER_DATA(req->input_buffer);

    ryDbg("file name [%s], size %d post_size %u\n", file_name, file_size, post_size);

    FILE *fp = NULL;
    fp = fopen(file_name, "wb");
    if (NULL == fp)
    {
        ryErr("open %s failed!\n", file_name);
        evhttp_send_error(req, HTTP_EXPECTATIONFAILED, "File was not create!");
        return;
    }
    fwrite(post_data, 1, post_size, fp);

    fclose(fp); fp = NULL;
    //struct curl_slist *headers = NULL;
    //char header[128] = "";


    // 响应给客户端

    //HTTP header
    evhttp_add_header(req->output_headers, "Server", __FILE__);
    evhttp_add_header(req->output_headers, "Content-Type", "text/plain; charset=UTF-8");
    evhttp_add_header(req->output_headers, "Connection", "close");

    evhttp_send_reply(req, HTTP_OK, "OK", NULL);

    ryDbg("200 ok\n");

}

int main()
{
    //http_request_module_init();

    HttpClient aaa;
    aaa.start();



    HttpServer server("0.0.0.0", 7777);

    server.addRequestHandle("/login", login_handler, nullptr);
    server.addRequestHandle("/1.jpg", send_small_file_handler, nullptr);
    server.addRequestHandle("/chrome.mp4", send_big_file_handler, nullptr);
    server.addRequestHandle("/upload", recv_file_handler, nullptr);

    server.start();

    while (1)
    {
        sleep(100);
    }
    


    return 0;
}

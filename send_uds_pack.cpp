#include "ryprint.h"
#include "rymacros.h"
#include "http_request_module.h"

#include <sys/un.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>

#define UDS_DGRAM_CLIENT_PATH  "uds-dgram-client"

#define BIND_CLIENT_ADDR  1//不给套接字命名 服务器将无法获取客户端的地址信息

#define POSTDATA "{\"username\":\"gailun\",\"password\":\"123123\",\"driver\":\"yes\"}"


int main(int argc, char *argv[])
{
    int sockfd;
    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (-1 == sockfd)
    {
        ryDbg("create socket error!\n");
        exit(-1);
    }

#if BIND_CLIENT_ADDR
    // 如果想让对端服务器识别到本地客户端的addr信息 需要给套接字命名：bind
    struct sockaddr_un client_addr = { 0 };
    client_addr.sun_family = AF_UNIX;
    strcpy(client_addr.sun_path, UDS_DGRAM_CLIENT_PATH);

    if (strlen(UDS_DGRAM_CLIENT_PATH) >= sizeof(client_addr.sun_path))
    {
        ryDbg("uds path length too long: %s!\n", UDS_DGRAM_CLIENT_PATH);
        close(sockfd);
        exit(-2);
    }
    unlink(UDS_DGRAM_CLIENT_PATH);
    if (0 != bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)))
    {
        ryDbg("uds bind client addr fail!\n");
        close(sockfd);
        exit(-3);
    }
    ryDbg("uds bind %s sucess!\n", client_addr.sun_path);
#endif
    struct sockaddr_un server_addr = { 0 };
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, UDS_DGRAM_SERVER_PATH);


    char *recv_buf_ptr = NULL;
    char *send_buf_ptr = NULL;

    send_buf_ptr = (char *)malloc(UDS_DGRAM_BUFFER_SIZE);
    recv_buf_ptr = (char *)malloc(UDS_DGRAM_BUFFER_SIZE);

    assert_param_return(send_buf_ptr && recv_buf_ptr, -2);

    uds_pack_header_t *udspack_header_ptr = (uds_pack_header_t *)send_buf_ptr;

    int addr_size = sizeof(struct sockaddr_un);
    int ret = 0;

    while (1)
    {
        memset(send_buf_ptr, 0, sizeof(send_buf_ptr));
        memset(recv_buf_ptr, 0, sizeof(recv_buf_ptr));

        int msgtype = 0;
        ryDbg("input msg msgtype:");
        scanf("%d", &msgtype);
        char ch;while ((ch = getchar()) != '\n' && ch != EOF);
        
        if (0 == msgtype)
        {
            break;
        }

        udspack_header_ptr->msgtype = msgtype;
        memcpy(udspack_header_ptr->m_data, POSTDATA, strlen(POSTDATA));

        ret = sendto(sockfd, udspack_header_ptr, sizeof(uds_pack_header_t) + strlen(POSTDATA) - 1, 0, (struct sockaddr *)&server_addr, addr_size);
        if (ret < 0)
        {
            ryDbg("uds sendto error!\n");
            break;
        }
        ryDbg("uds send %d byte msg code %d to %s!\n", ret, udspack_header_ptr->msgtype, server_addr.sun_path);
        ryDbg(" %s \n", (char *)&udspack_header_ptr->m_data);

#if BIND_CLIENT_ADDR
        ret = recvfrom(sockfd, recv_buf_ptr, UDS_DGRAM_BUFFER_SIZE, 0, NULL, 0);
        if (ret < 0)
        {
            ryDbg("uds recvfrom error!\n");
            break;
        }
        ryDbg(" uds recv %d byte\n", ret);
        ryDbg("uds recv msg %s from %s!\n", recv_buf_ptr, server_addr.sun_path);

#endif
    }

    close(sockfd);
    unlink(UDS_DGRAM_CLIENT_PATH);
    
    return 0;
}
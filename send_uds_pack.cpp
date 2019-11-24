#include "ryprint.h"
#include "rymacros.h"

#include <sys/un.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h> //open

typedef struct _uds_pack_header_t
{
    unsigned int msgtype;
    char m_data[1];
} uds_pack_header_t;

#define BIND_CLIENT_ADDR  1//不给套接字命名 服务器将无法获取客户端的地址信息

#define POSTDATA "{\"username\":\"gailun\",\"password\":\"123123\",\"driver\":\"yes\"}"

#define UDS_DGRAM_SERVER_PATH "uds-dgram-server"
#define UDS_DGRAM_CLIENT_PATH "uds-dgram-client"
#define TEST_FILE_PATH "testfile"

#define BIND_CLIENT_ADDR 1 //不给套接字命名 服务器将无法获取客户端的地址信息

int main(int argc, char *argv[])
{
    int sockfd;
    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (-1 == sockfd)
    {
        ryErr("create socket error!\n");
        exit(-1);
    }

#if BIND_CLIENT_ADDR
    // 如果想让对端服务器识别到本地客户端的addr信息 需要给套接字命名：bind
    struct sockaddr_un client_addr = {0};
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
#endif
    struct sockaddr_un server_addr = {0};
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, UDS_DGRAM_SERVER_PATH);

    int filefd = -1;
    filefd = open(TEST_FILE_PATH, O_CREAT | O_RDWR, 0777);
    if (filefd < 0)
    {
        ryErr("open file_fd failed!\n");
        return -1;
    }

    ryErr("\n");
    char sendbuf[512] = {0};
    char recvbuf[512] = {0};

    uds_pack_header_t *udspack_header_ptr = (uds_pack_header_t *)sendbuf;

    struct iovec iov[1];
    // iov_base 和 iov_len 指向输入或输出缓冲区与大小，类似 recvfrom 和 sendto 的第二个和第三个参数。
    iov[0].iov_base = sendbuf;
    iov[0].iov_len = sizeof(sendbuf);

    ryDbg(" iov[0].iov_len [%ld]!\n", iov[0].iov_len);
    //该buf用于存放辅助数据 buf大小根据CMSG_SPACE宏计算出来
    char ctrl_data[CMSG_SPACE(sizeof(filefd))];

    struct msghdr socket_msg;
    // msg_name 和 msg_namelen 的作用类似 recvfrom 和 sendto 的第五个和第六个参数
    // 指向一个套接字地址结构，sendmsg的时候用于指定接收者的地址信息  recvmsg的时候用于保存发送者的地址信息
    socket_msg.msg_name = &server_addr;
    socket_msg.msg_namelen = sizeof(server_addr);
    // msg_iov 和 msg_iovlen 这两个成员指定输入或输出缓冲区数组（即iovec结构数组），类似 read 和 write 的第二个和第三个参数。
    // 可以设置多个缓冲区
    socket_msg.msg_iov = iov;
    socket_msg.msg_iovlen = 1;
    //这两个成员指定辅助数据的位置和大小。用于指向要传输的文件描述符
    socket_msg.msg_control = ctrl_data;
    socket_msg.msg_controllen = sizeof(ctrl_data);

    //msg_control中可能携带多个cmsghdr, 可以采用对应的宏协助处理:
    // CMSG_FIRSTHDR(), CMSG_NXTHDR(), CMSG_DATA()， CMSG_LEN()， CMSG_SPACE().
    //这里仅传输一个
    struct cmsghdr *ctrl_msg;
    ctrl_msg = CMSG_FIRSTHDR(&socket_msg);
    ctrl_msg->cmsg_len = CMSG_LEN(sizeof(int));
    //具体的协议标识 IPPROTO_IP(ipv), IPPROTO_IPV6(ipv6), SOL_SOCKET(unix domain).
    ctrl_msg->cmsg_level = SOL_SOCKET;
    //上述协议中的类型 比如SOL_SOCKET中主要包含:SCM_RIGHTS(发送接收描述字)， SCM_CREDS(发送接收用户凭证)
    ctrl_msg->cmsg_type = SCM_RIGHTS;
    *((int *)CMSG_DATA(ctrl_msg)) = filefd;

    ryErr("\n");
    socklen_t addr_size = sizeof(struct sockaddr_un);
    int ret = 0;

    while (1)
    {
        memset(sendbuf, 0, sizeof(sendbuf));
        memset(recvbuf, 0, sizeof(recvbuf));

        int msgtype = 0;
        ryDbg("input msg msgtype:");
        scanf("%d", &msgtype);
        char ch;
        while ((ch = getchar()) != '\n' && ch != EOF);

        if (0 == msgtype)
        {
            break;
        }

        udspack_header_ptr->msgtype = msgtype;
        memcpy(udspack_header_ptr->m_data, POSTDATA, strlen(POSTDATA));

        //这样 sendmsg的时候内核会对辅助数据进行特殊处理，使之可以传输给另一个进程
        //在发送完成以后，在发送进程即使关闭该描述字也不会影响接收进程的描述符，发送一个描述字导致该描述字的引用计数加1。
        ret = sendmsg(sockfd, &socket_msg, 0);
        if (ret < 0)
        {
            ryErr("uds sendto error! %d\n", ret);
            break;
        }
        ryDbg("uds send msg %s to %s!\n", udspack_header_ptr->m_data, server_addr.sun_path);

#if BIND_CLIENT_ADDR
        ret = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&server_addr, &addr_size);
        if (ret < 0)
        {
            ryDbg("uds recvfrom error!\n");
            break;
        }
        ryDbg("uds recv msg %s from %s!\n", recvbuf, server_addr.sun_path);
#endif
    }

    close(filefd);
    close(sockfd);
    unlink(UDS_DGRAM_CLIENT_PATH);

    return 0;
}

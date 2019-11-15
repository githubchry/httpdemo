
typedef enum _http_msg_type_t 
{
    _http_msg_type_st,
    ___test,
    //以下协议的发送方是平台系统，接收方是设备
    _http_msg_type_get_access_token,    //1)用户鉴权接口
    _http_msg_type_release_token,       //2)用户释放权限接口
    _http_msg_type_set_center_url,      //3)设置上传中心地址
    _http_msg_type_get_device_time,     //4)获取设备时间
    _http_msg_type_set_device_time,     //5)设置设备时间
    _http_msg_type_restart_device,      //6)控制设备重启
    _http_msg_type_arm_chn,             //7)控制通道布防
    _http_msg_type_dis_arm_chn,         //8)控制通道撤防

    //以下协议的发送方是设备，接收方是平台系统
    _http_msg_type_statu_monitor_url,       //9)状态上传
    _http_msg_type_record_monitor_url,      //10)事件上传
    _http_msg_type_get_device_all_statu,    //11)主动请求设备所有状态

    _http_msg_type_ed,
} http_msg_type_t;



typedef struct _uds_pack_header_t 
{
    unsigned int msgtype;
	char m_data[1];
} uds_pack_header_t;

typedef struct _http_request_api_t 
{
    http_msg_type_t msgtype;
	char api_suffix[64];
} http_request_api_t;

#define SERVER_ADDR    "127.0.0.1"
#define SERVER_PORT    "7777"

#define UDS_DGRAM_BUFFER_SIZE  1024
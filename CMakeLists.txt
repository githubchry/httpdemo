cmake_minimum_required(VERSION 3.4.1)

project(http-demo)

# 添加头文件目录和库目录
include_directories("/home/chry/codes/new-root/include")
link_directories("/home/chry/codes/new-root/lib")

set(CMAKE_EXE_LINKER_FLAGS "-pthread")
set(CMAKE_CXX_STANDARD 11)

# 指定生成目标
add_executable(simple_server simple/server.cpp)
add_executable(simple_client simple/client.cpp)

target_link_libraries (simple_server event cjson)
target_link_libraries (simple_client curl cjson)

# aux_source_directory(dir DIR_SRCS)查找目录下的所有源文件 并将名称保存到 DIR_SRCS 变量
#aux_source_directory(http-server HTTP_SERVER_DIR_SRCS)#
add_executable(http_client      http-client/main.cpp                           
                                http-client/http_request_module.cpp
                                http-client/rypthread.c       )

target_link_libraries (http_client curl)
                                
add_executable(send_uds_pack    http-client/send_uds_pack.cpp)



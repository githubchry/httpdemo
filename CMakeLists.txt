cmake_minimum_required(VERSION 3.4.1)

project(http-demo)

# 添加头文件目录和库目录
include_directories("/home/chry/code/github/https/new-root/include")
link_directories("/home/chry/code/github/https/new-root/lib")

set(CMAKE_EXE_LINKER_FLAGS "-pthread")
set(CMAKE_CXX_STANDARD 11)

# 指定生成目标
add_executable(simple_server simple/server.cpp)
add_executable(simple_client simple/client.cpp)

target_link_libraries (simple_server event cjson)
target_link_libraries (simple_client curl cjson)

# aux_source_directory(dir DIR_SRCS)查找目录下的所有源文件 并将名称保存到 DIR_SRCS 变量
aux_source_directory(http-client    HTTP_CLIENT_DIR_SRCS)
add_executable (http_client        ${HTTP_CLIENT_DIR_SRCS})
target_link_libraries (http_client curl event cjson)
           
include_directories(http-client)

add_executable(send_uds_pack    send_uds_pack.cpp)



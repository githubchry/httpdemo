cmake_minimum_required(VERSION 3.4.1)

project(http-demo)

# 添加头文件目录和库目录
include_directories("/home/chry/codes/new-root/include")
link_directories("/home/chry/codes/new-root/lib")

set(CMAKE_CXX_STANDARD 11)

# aux_source_directory(dir DIR_SRCS)查找目录下的所有源文件 并将名称保存到 DIR_SRCS 变量
aux_source_directory(http-server HTTP_SERVER_DIR_SRCS)
aux_source_directory(http-client HTTP_CLIENT_DIR_SRCS)
# 指定生成目标
add_executable(http_server ${HTTP_SERVER_DIR_SRCS})
add_executable(http_client ${HTTP_CLIENT_DIR_SRCS})

target_link_libraries (http_server event cjson)
target_link_libraries (http_client curl cjson)




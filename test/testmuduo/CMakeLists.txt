# cmake_minimum_required(VERSION 3.0)#需要的版本
# project(main)#工程的名字

# #配置编译选项
# set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)#表示可执行文件可以调试

#配置头文件搜索路径
#include_directories()

#配置库文件搜索路径
#link_directories()

#生成可执行文件
#add_executable(server muduo_server.cpp)

#设置需要的源文件列表
#生成可执行文件server，有SRC_LIST变量定义的源文件编译而来
set(SRC_LIST muduo_server.cpp)
add_executable(server ${SRC_LIST})

#把指定路径下的所有源文件名字，放入变量名SRC_LIST中
#aux_source_directory(. SRC_LIST)#如果很多源文件，也不必一个个添加aux命令

#需要链接的库
target_link_libraries(server muduo_net muduo_base pthread)#server这个目标程序，需要链接的库文件

#设置可执行文件最终生成的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
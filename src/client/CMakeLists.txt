# 定义了一个SRC_LIST变量，包含了该目录下所有的源文件
aux_source_directory(. SRC_LIST)#将当前路径下所有的名字放进SRC_LIST变量

# 指定生成可执行文件
add_executable(ChatClient ${SRC_LIST})
# 指定可执行文件链接时需要依赖的库文件
target_link_libraries(ChatClient pthread)
#客户端有读写两个线程，读线程接收服务器的消息响应，写线程接收用户的输入
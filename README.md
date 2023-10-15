# chatserver2
可以工作在nginx tcp负载均衡环境中的集群聊天服务器和客户端源码 基于moduo库实现 redis mysql

编译方式
cd cuild
rm -rf *
cmake ..
make

需要nginx的tcp负载均衡

#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

//聊天服务器的主类
class ChatServer
{
public:
    //初始化聊天服务器对象
    ChatServer(EventLoop *loop,
               const InetAddress &listenAddr,
               const string &nameArg);

    //启动服务
    void start();

private:
    //上报链接相关信息（连接创建/断开）的回调函数
    void onConnection(const TcpConnectionPtr &conn);//这里的形参类型是查看tcpserver库中回调函数的形参的参数

    //上报读写事件相关信息的回调函数
    void onMessage(const TcpConnectionPtr &,
                   Buffer *,
                   Timestamp);


    TcpServer _server;//组合的muduo库，实现的服务器功能的类对象
    EventLoop *_loop; // 事件循环的指针，在合适的时候退出循环
};

#endif
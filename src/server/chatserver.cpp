#include "chatserver.hpp"
#include"chatservice.hpp"
#include "json.hpp"
#include <functional>
#include<string>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg),
      _loop(loop)
{
    // 注册连接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 上报链接相关信息（连接创建/断开）的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    //用户断开连接
    if(!conn->connected()){
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown(); // muduo库自身的日志会打印
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr & conn,
                           Buffer *buffer,
                           Timestamp time)
{
    string buf = buffer->retrieveAllAsString();//将缓冲区中的数据，放入字符串
    json js = json::parse(buf);//数据的反序列化，类似解码
    //这个消息里一定包含一个msgid，因为消息都有属于什么业务的标识
    //如果在这里判断是属于什么业务，然后调用对应方法，就会将网络模块代码和业务模块代码强耦合

    //为了完全解耦网络模块代码和业务模块代码
    //通过js["msgid"]，获取一个业务处理器（事先绑定的方法，在网络模块看不到），将网络模块的数据传给业务模块处理

    //instance获取服务的唯一实例
    //getHandler获取对应ID的事件处理器
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>()); // 需要int，不是json类型，需要转化一下
    //回调消息绑定好的事件处理器，执行相应的业务处理
    msgHandler(conn,js,time);//msgHandler 变量被赋值为消息处理函数
}
/*
muduo网络库给用户提供了两个主要的类
TcpServer：用于编写服务器程序
TcpClient：用于编写客户端程序

网络库是封装了epoll+线程池
好处：将网络I/O的代码和业务代码区分开（主要精力在业务模块开发）

业务代码：1、用户的连接和断开   2、用户的可读写事件
*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include<string>
using namespace std;
using namespace muduo;
using namespace muduo::net; /// TcpServer对象有很多，这里用到这个库
using namespace placeholders;

// 基于muduo网络库开发服务器程序
/*
    1、组合TcpServer对象
    2、创建EventLoop事件循环对象的指针
    3、明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
    4、在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
    5、设置合适的服务端线程数量，muduo库会自己划分i/o线程和worker线程
*/

class ChatServer
{

public:
    ChatServer(EventLoop *loop,               // 事件循环（reactor）
               const InetAddress &listenAddr, // IP+端口
               const string &nameArg)         // 服务器的名称，等同于给线程绑定一个名字
        : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        // 给服务器注册用户连接的创建和断开回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1)); // 参数占位符要加using namespace placeholders

        // 给服务器注册用户读写事件回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        //设置服务器端的线程数量
        _server.setThreadNum(4);//一般四核设置4个线程，1个i/o线程，3个worker线程
    }

    //开启事件循环
    void start(){
        _server.start();
    }

private:
    // 专门处理用户的连接、创建和断开  等于muduo库封装了 epoll listenfd accept等操作
    // 这里写成成员方法，是想访问对象的成员变量，但多了this指针，就和setConnectionCallback类型不同了，需要使用那个绑定器
    void onConnection(const TcpConnectionPtr &conn)
    {
        // 只需要关注业务即可，不需要关注什么时候调用。当有连接创建和断开，会自动调用

        
        if(conn->connected()){
            cout << conn->peerAddress().toIpPort() <<"->"<<conn->localAddress().toIpPort()<<"state:online"<<endl;
        }
        else{
            cout << conn->peerAddress().toIpPort() <<"->"<<conn->localAddress().toIpPort()<<"state:offline"<<endl;
            conn->shutdown();//连接释放，回收fd的 资源
            // _loop->quit();//退出epoll
        }
    }

    //专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr &conn,//连接，可以读写事件
                   Buffer *buffer,//缓冲区，用于提高数据收发的性能
                   Timestamp time)//接收到数据的时间信息
    {
        string buf = buffer->retrieveAllAsString();
        cout << "recv data:" << buf << "time:" << time.toString() << endl;
        conn->send(buf);
    }
    TcpServer _server; // #1
    EventLoop *_loop;  // #2 可以看做是epoll，向loop上注册感兴趣的事件
};


int main(){
    EventLoop loop;//epoll
    InetAddress addr("127.0.0.1", 6000);//addr通过引用接收，直接传对象就行
    ChatServer server(&loop, addr, "ChatServer");
    server.start();//启动服务，将epollfd 通过epoll_ctl添加到epoll
    loop.loop();//等于调用epoll_wait，以阻塞方式，等待新用户的连接、已连接用户的读写事件等操作

    return 0;
}
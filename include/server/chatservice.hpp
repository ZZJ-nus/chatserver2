#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include<unordered_map>
#include<functional>//用于回调函数
#include<muduo/net/TcpConnection.h>//对应下面MsgHandler
#include<mutex>//互斥锁
#include"offlinemessagemodel.hpp"
#include"friendmodel.hpp"
#include"groupmodel.hpp"
#include"redis.hpp"
using namespace std;
using namespace muduo;
using namespace muduo::net;
#include "json.hpp"
using namespace placeholders;
using json = nlohmann::json;

#include"usermodel.hpp"

// using定义一个时间处理器，类似typedef功能，给已经存在的类型定义一个新的类型名称
//function中void表示返回值为0，参数就是tcpserver.cpp中onmessage的参数（json之前已经反序列化了）
//表示处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn,json &js,Timestamp time )>;

//业务模块采用单例模式，对象有一个实例就够了
//聊天服务器业务类
class ChatService{
public:
    //获取单例对象的接口函数
    static ChatService* instance();
    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);//网络层派发的处理器回调，所以方法都是这样，需要传递3个参数
    //添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    //服务器异常后，业务重置方法
    void reset();
     // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, string);

private:
    ChatService();//单例，构造函数私有化

    unordered_map<int, MsgHandler> _msgHandlerMap;//存储消息ID和其对应的业务处理方法
    //在多线程环境中，不存在运行过程中增加业务

    //存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;
    //在多线程环境中，会随着用户上线下线改变，所以访问要注意线程安全

    //定义互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;

    //数据操作类对象
    UserModel _userModel;

    //
    OfflineMsgModel _offlineMsgModel;

    FriendModel _friendModel;

    GroupModel _groupModel;

    //redis操作对象
    Redis _redis;
};

#endif
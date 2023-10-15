#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
using namespace std;


class Redis
{
public:
    Redis();//资源的初始化和释放操作
    ~Redis();

    // 连接redis服务器 
    bool connect();

    // 向redis指定的通道channel发布消息
    bool publish(int channel, string message);

    // 向redis指定的通道subscribe订阅消息
    bool subscribe(int channel);

    // 向redis指定的通道unsubscribe取消订阅消息
    bool unsubscribe(int channel);

    // 在独立线程中接收订阅通道中的消息
    void observer_channel_message();//响应通道上发生的消息

    // 初始化向业务层上报通道消息的回调对象
    void init_notify_handler(function<void(int, string)> fn);

private:
    // hiredis同步上下文对象，负责publish消息
    redisContext *_publish_context;////////////////////publish和subscribe这两个命令必须在两个上下文进行
    //就像开了一个窗口publish，当前上下文不会显示subscribe的消息

    // hiredis同步上下文对象，负责subscribe消息
    redisContext *_subcribe_context;

    // 回调操作，收到订阅的消息，给service层上报
    function<void(int, string)> _notify_message_handler;////不知道什么时候上报，所以要先注册一个回调
    //int对应通道号，string对应消息
    //redis检测到相应的通道有消息发生，就会给chatserver进行事件的通知
};

#endif
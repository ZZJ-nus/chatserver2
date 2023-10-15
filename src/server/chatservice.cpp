#include"chatservice.hpp"
#include"public.hpp"
#include<muduo/base/Logging.h>//用于muduo库日志打印
#include<string>
#include<vector>
using namespace std;
using namespace muduo;

//获取单例对象的接口函数
ChatService* ChatService::instance(){//静态方法在类外定义时，不用再写static
    static ChatService service;///构建单例对象，并且是线程安全的

    return &service;
}

//注册消息以及对应的回调操作
ChatService::ChatService()//构造函数，成员变量和方法对应类中有
{
    //业务模块核心（解耦网络模块）
    //键是int型，定义在public.hpp中，然后值是对应函数，这里需要使用绑定器
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});//3个参数，参数占位符
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 连接redis服务器
    if (_redis.connect())///通道上有消息发生，就会向相应服务器上报
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));//init_notify_handler需要接受两个参数
        //一个参数是通道号，另一个是在通道中发生的消息
    }
}


//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid){
    //记录错误日志，msgid没有对应的事件回调
    //这里使用muduo库的日志打印
    auto it = _msgHandlerMap.find(msgid);
    if(it==_msgHandlerMap.end()){
        //返回一个默认处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time) { //=是按值获取
            LOG_ERROR << "msgid:" << msgid << "can not find handler!";//muduo库日志打印不需要endl
        };
    }
    else{
        return _msgHandlerMap[msgid];
    }
    
}

// 处理登录业务 需要   ID和密码
//这方法具体什么时候做不知道，等待网络模块调用
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time){
    // LOG_INFO << "do login service!!";//登录key是1
    int id = js["id"].get<int>();///get一个实例化的类型，将其转成整型
    string pwd = js["password"];
    User user=_userModel.query(id);
    if(user.getID()==id&&user.getPwd()==pwd){//user.getID()!=-1是query没找到默认返回构造中id=-1
        if(user.getState()=="online"){
            //该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;//注意msgid的都是写在public.hpp中
            response["errno"] = 2;//错误消息，如果是0说明响应成功
            response["errmsg"] = "该账号已经登录，请重新输入新账号";
            conn->send(response.dump()); // 发送json消息
        }
        else{

            //登录成功，记录用户连接信息
            {//括号保证作用域
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id);
            
            //会被多个线程调用，因为onmessage，也就是处理读写事件的函数，会被多个线程调用，
            //不同用户可能在不同工作线程响应，所以是多线程环境，所以要考虑线程安全问题
            //STL容器没有考虑线程安全问题


            //登陆成功，更新用户状态信息 offline-->online
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;//注意msgid的都是写在public.hpp中
            response["errno"] = 0;//错误消息，如果是0说明响应成功
            response["id"] = user.getID();
            response["name"] = user.getName();//实际昵称等信息都是记录在客户端


            //检查该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if(!vec.empty()){
                response["offlinemsg"] = vec;//json可以直接对容器序列化反序列化
                //读取该用户的离线消息后，把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }

            //查询该用户好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if(!userVec.empty()){//有好友
                // response["friends"] = userVec; //不是内置类型，不能直接放到json
                vector<string> vec2;
                for(User &it:userVec){//将userVec中的信息，转成合适的json字符串
                    json js;
                    js["id"] = it.getID();
                    js["name"] = it.getName();
                    js["state"] = it.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            conn->send(response.dump()); // 发送json消息
        }
        
    }
    else{
        //该用户不存在/用户存在，密码错误，登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;//注意msgid的都是写在public.hpp中
        response["errno"] = 1;//错误消息，如果是0说明响应成功
        response["errmsg"] = "用户名或者密码错误了";
        conn->send(response.dump()); // 发送json消息
    }
}

//处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time){
    // LOG_INFO << "do reg service!!";//注册key是2

    //注册需要：name+password
    string name = js["name"];
    string pwd = js["password"];
    //创建user对象
    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state=_userModel.insert(user);//判断是否注册成功
    if(state){//注册成功,返回用户id
        json response;
        response["msgid"] = REG_MSG_ACK;//注意msgid的都是写在public.hpp中
        response["errno"] = 0;//错误消息，如果是0说明响应成功
        response["id"] = user.getID();
        conn->send(response.dump());//发送json消息
    }
    else{//注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;//注意msgid的都是写在public.hpp中
        response["errno"] = 1;//错误消息，如果是1说明失败
        // response["id"] = user.getID();//有错就不需要读ID字段了
        conn->send(response.dump());//发送json消息
    }
}


// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();//可以获取id

    {//要操作connectionmap，需要加锁操作
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid); 

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

//处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn){
    //需要查unordered_map<int, TcpConnectionPtr> _userConnMap;，找到对应的id，将用户的数据库中的状态信息更新
    
    //可能用户在登录，然后又有异常退出，所以要保证线程安全问题
    User user;
    {//下面数据库操作部分不需要做线程安全
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)//迭代器，上过线就说明在map表中记录
        { // 迭代器
            if(it->second==conn){
                //从map表删除用户的连接信息
                user.setID(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getID()); 
    

    //更新用户的状态信息
    if(user.getID()!=-1){//如果没找到，就不用请求数据库了。但是其实一旦连接成功，一定在map表中
        user.setState("offline");
        _userModel.updateState(user);
    }
    
}


//一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int toid = js["to"].get<int>();
    
    { // 要访问连接信息表，保证其线程安全
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it!=_userConnMap.end()){
            //toid在线，转发消息。如果在线还要查找conn，所以这部分也得保证线程安全（可能存在在正发送的时候，conn被移除了）
            //服务器主动推送消息给toid用户
            it->second->send(js.dump());//相当于服务器做了消息中转

            return;//return后，这个锁就析构了
        }
    }

    //查询toid是否在线 （当前主机没找着，可能是不在线，或者在其他主机登录）
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());//直接将消息发送到以该用户命名的通道上
        return;
    }
    
    //toid不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

//服务器异常后，业务重置方法
void ChatService::reset(){
    //把online状态的用户设置成offline
    _userModel.resetState();
}

//添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["id"].get<int>();
    int friendid=js["friendid"].get<int>();
    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)//创建群到allgroup表，同时创建者也要记录在groupuser中
{
    int userid = js["id"].get<int>();//获取要创建群所需的消息
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);//一开始并不知道群组id，通过createGroup获取群组id（这里是allgroup表）
    //自动生成主键ID，然后createGroup将主键id写到group表中
    if (_groupModel.createGroup(group))//不能重复名称
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");//groupuser表
        //这里也可以向客户端作出响应，参考登录/注册成功的响应
    }
}

// 加入群组业务   groupuser表
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");//加入群身份就是normal
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);//该用户所在群组的其他用户的id

    lock_guard<mutex> lock(_connMutex);//要包括整个循环，否则会不断进行加锁解锁操作，保证map表操作的线程安全问题
    //因为要操作_userConnMap，操作时不允许其他人其他线程进行操作，所以要加锁
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())//是否在线    在线就转发
        {
            // 转发群消息
            it->second->send(js.dump());///键是id，值就是connection
        }
        else
        {
            // 查询toid是否在线 （当前主机不在线，还有可能在其他主机上）
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());//用id作为通道号，发布消息
            }
            else
            {
                // 存储离线群消息
                _offlineMsgModel.insert(id, js.dump());//不在线
            }
        }
    }
 
}

// 从redis消息队列中获取订阅的消息  实现预置的回调 通道号+消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)//由redis调用，用户在哪台服务器登录，服务器就会收到消息
{
    lock_guard<mutex> lock(_connMutex);
    //这里还要判断是否在线的原因是，可能发的时候对方在线，现在刚好下线了
    auto it = _userConnMap.find(userid);//能接收到消息，肯定使能找到connection的
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);//向通道发布消息，到从通道取消息的过程中，该用户下线
}
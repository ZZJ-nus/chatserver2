#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

// 记录当前（客户端）系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;//登录成功后，服务器会返回好友列表，群组列表等信息
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;
// 显示当前登录成功用户的基本信息
void showCurrentUserData();

// 控制主菜单页面程序
bool isMainMenuRunning = false;//登录成功时，设置为true，登出时false

// 用于读写线程之间的通信
sem_t rwsem;
// 记录登录状态
atomic_bool g_isLoginSuccess{false};


// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int);//登录成功才会进入


// 聊天客户端程序实现，main线程（主线程）用作发送线程，子线程用作接收线程
//发送和接收是并行的，不可能不发送，接收就阻塞
int main(int argc, char **argv)//第一个参数：表示命令行参数的数量，第二个参数：表示命令行参数的值
{
    if (argc < 3)//判断命令行参数个数
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;//cerr用来输出错误信心
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);//AF_INET 和 SOCK_STREAM 是预定义的常量，分别表示 IPv4 地址族和流式套接字类型
    //文件描述符是一个整数，用于唯一标识打开的文件或套接字
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 填写client需要连接的server信息ip+port（绑定IP地址和端口号）
    sockaddr_in server;//结构体，存储服务器IP和端口信息
    memset(&server, 0, sizeof(sockaddr_in));//将结构体所有字节函数设置为0，确保没有旧的数据残留

    server.sin_family = AF_INET;//使用IPV4地址族
    server.sin_port = htons(port);//htons 函数用于将主机字节序的端口号转换为网络字节序
    server.sin_addr.s_addr = inet_addr(ip);//inet_addr 函数将一个字符串形式的 IP 地址转换为网络字节序的 32 位整数表示

    // client和server进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {//如果连接失败，释放资源，然后退出
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // 初始化读写线程通信用的信号量
    sem_init(&rwsem, 0, 0);

    // 连接服务器成功，启动接收子线程   对于不同的操作，登录的响应逻辑就在这个子线程中
    std::thread readTask(readTaskHandler, clientfd); // 相当于pthread_create
    readTask.detach();                               // pthread_detach 设置成分离线程，线程使用完自动回收线程调用的tcp资源

    // main线程用于接收用户输入，负责发送数据
    for (;;)//死循环，如果用户没有退出，一直停留在首页面
    {
        // 显示首页面菜单 登录、注册、退出
        cout << "========================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "========================" << endl;
        cout << "choice:";//提示选择
        int choice = 0;
        cin >> choice;//实际上是敲了一个整数+回车
        cin.get(); // 读掉缓冲区残留的回车（防止回车被别的类似字符串接收）

        switch (choice)//登陆、注册、退出
        {
        case 1: // login业务  输入id+密码
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "userid:";
            cin >> id;
            cin.get(); // 读掉缓冲区残留的回车
            cout << "userpassword:";
            cin.getline(pwd, 50);

            json js;//发送时，序列化成字符串发送出去
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            g_isLoginSuccess = false;

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send login msg error:" << request << endl;
            }

            sem_wait(&rwsem); // 等待信号量，由子线程处理完登录的响应消息后，通知这里
                
            if (g_isLoginSuccess) 
            {
                // 进入聊天主菜单页面
                isMainMenuRunning = true;
                mainMenu(clientfd);/////登录成功就会进入主菜单页面
            }
        }
        break;
        case 2: // register业务，输入用户名或密码
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);//如果用cin，是不能读空格的
            cout << "userpassword:";
            cin.getline(pwd, 50);

            json js;//对应服务端注册所需的json  组装json
            js["msgid"] = REG_MSG;//注册消息绑定注册业务处理器
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }
            
            sem_wait(&rwsem); // 等待信号量，子线程处理完注册消息会通知
        }
        break;
        case 3: // quit业务
            close(clientfd);//资源释放
            sem_destroy(&rwsem);
            exit(0);
        default:
            cerr << "invalid input!" << endl;//输错
            break;
        }
    }

    return 0;
}

// 处理注册的响应逻辑
void doRegResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) // 注册失败
    {
        cerr << "name is already exist, register error!" << endl;
    }
    else // 注册成功
    {
        cout << "name register success, userid is " << responsejs["id"]
                << ", do not forget it!" << endl;
    }
}

// 处理登录的响应逻辑
void doLoginResponse(json &responsejs)//responsejs是接收的客户端响应的数据
{
    if (0 != responsejs["errno"].get<int>()) // 登录失败  errno0才是成功
    {
        cerr << responsejs["errmsg"] << endl;//服务器已经有登录的errormsg
        g_isLoginSuccess = false;
    }
    else // 登录成功
    {
        // 记录当前用户的id和name  根据响应的数据，填到g_currentUser类中
        g_currentUser.setID(responsejs["id"].get<int>());//id其实就是自己发过去的
        g_currentUser.setName(responsejs["name"]);

        // 记录当前用户的好友列表信息
        if (responsejs.contains("friends"))
        {
            // 初始化
            g_currentUserFriendList.clear();//之前出现过的信息清除

            vector<string> vec = responsejs["friends"];///当时存放也是json字符串的形式
            for (string &str : vec)
            {
                json js = json::parse(str);
                User user;
                user.setID(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);//将好友信息反序列化，然后填到全局vector
            }
        }
        

        // 记录当前用户的群组列表信息
        if (responsejs.contains("groups"))
        {
            // 初始化
            g_currentUserGroupList.clear();//不初始化登录几次就会有几次消息

            vector<string> vec1 = responsejs["groups"];//与服务器打包时，同样的方式进行解析
            for (string &groupstr : vec1)
            {
                json grpjs = json::parse(groupstr);
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                vector<string> vec2 = grpjs["users"];
                for (string &userstr : vec2)
                {
                    GroupUser user;
                    json js = json::parse(userstr);
                    user.setID(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }

                g_currentUserGroupList.push_back(group);
            }
        }

        // 显示登录用户的基本信息
        showCurrentUserData();

        // 显示当前用户的离线消息  个人聊天信息或者群组消息
        if (responsejs.contains("offlinemsg"))
        {
            vector<string> vec = responsejs["offlinemsg"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                // time + [id] + name + " said: " + xxx
                if (ONE_CHAT_MSG == js["msgid"].get<int>())
                {
                    cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                            << " said: " << js["msg"].get<string>() << endl;
                }
                else
                {
                    cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                            << " said: " << js["msg"].get<string>() << endl;
                }
            }
        }

        g_isLoginSuccess = true;
    }
}

// 子线程 - 接收线程
void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};//接收数据
        int len = recv(clientfd, buffer, 1024, 0);  // 阻塞了（当用户登出，该线程响应不了）
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }

        // 接收ChatServer转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (ONE_CHAT_MSG == msgtype)//聊天消息
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }

        if (GROUP_CHAT_MSG == msgtype)
        {
            cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }

        if (LOGIN_MSG_ACK == msgtype)
        {
            doLoginResponse(js); // 处理登录响应的业务逻辑
            sem_post(&rwsem);    // 通知主线程，登录结果处理完成
            continue;
        }

        if (REG_MSG_ACK == msgtype)
        {
            doRegResponse(js);
            sem_post(&rwsem);    // 通知主线程，注册结果处理完成
            continue;
        }
    }
}

// 显示当前登录成功用户的基本信息
void showCurrentUserData()
{
    cout << "======================login user======================" << endl;
    cout << "current login user => id:" << g_currentUser.getID() << " name:" << g_currentUser.getName() << endl;
    cout << "----------------------friend list---------------------" << endl;
    if (!g_currentUserFriendList.empty())//好友列表
    {
        for (User &user : g_currentUserFriendList)//好友列表不为空，打印好友信息
        {
            cout << user.getID() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "----------------------group list----------------------" << endl;
    if (!g_currentUserGroupList.empty())//群组列表
    {
        for (Group &group : g_currentUserGroupList)//群组列表不为空，打印群组信息  群组id,name,describe+群成员信息
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getID() << " " << user.getName() << " " << user.getState()
                     << " " << user.getRole() << endl;
            }
        }
    }
    cout << "======================================================" << endl;
}

// "help" command handler
void help(int fd = 0, string str = "");//不需要参数，但是为了和commandHandlerMap其他函数参数统一，增加了参数
// "chat" command handler
void chat(int, string);
// "addfriend" command handler
void addfriend(int, string);
// "creategroup" command handler
void creategroup(int, string);
// "addgroup" command handler
void addgroup(int, string);
// "groupchat" command handler
void groupchat(int, string);
// "loginout" command handler
void loginout(int, string);

// 系统支持的客户端命令列表   命令的字符串说明
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"loginout", "注销，格式loginout"}};

// 注册系统支持的客户端命令处理（如果存在命令，执行相应方法；这里值类似函数对象）
//相比于上一个字符串说明，这里值的function对象
unordered_map<string, function<void(int, string)>> commandHandlerMap = {//void(int, string)中，int是clientfd，string是用户输入的数据
    //这里function中int对应clientfd，string对应用户输入的数据
    {"help", help},//help不需要参数，但是为了map表中数据的格式统一，也写上
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

// 主聊天页面程序
void mainMenu(int clientfd)//进入，会给用户生成系统支持的命令，然后根据用户命令进行处理
{
    help();//两个参数给了默认值   

    char buffer[1024] = {0};
    while (isMainMenuRunning)//登录成功后，该值为true
    {
        cin.getline(buffer, 1024);//从用户获取输入
        string commandbuf(buffer);
        string command; // 存储命令
        int idx = commandbuf.find(":");//help和loginout命令是没有：的
        if (-1 == idx)//只有help和logout是没有：
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);//通过substr抠出命令  这里将字符串第一个冒号前的命令挖出
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())//如归随便输的，在commandHandlerMap中找不着
        {
            cerr << "invalid input command!" << endl;
            continue;
        }

        // 调用相应命令的事件处理回调，mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx)); // 调用命令处理方法
        //it->second指向命令名字对应的处理函数
        //commandbuf.substr(idx + 1, commandbuf.size() - idx)是除去命令以外，剩下的字符串
    }
}

// "help" command handler
void help(int, string)
{
    cout << "show command list >>> " << endl;
    for (auto &p : commandMap)///遍历命令列表  命令+对应格式  将系统支持的命令全打印一遍
    {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}
// "addfriend" command handler
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getID();//当前用户的ID
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addfriend msg error -> " << buffer << endl;
    }
}
// "chat" command handler
void chat(int clientfd, string str)
{
    // cerr << str << endl;
    int idx = str.find(":"); // friendid:message
    if (-1 == idx)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);
    // cerr << friendid << endl;


    json js;
    js["msgid"] = ONE_CHAT_MSG;//注意这里json字符要和服务端代码一致
    js["id"] = g_currentUser.getID();
    js["name"] = g_currentUser.getName();
    js["to"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send chat msg error -> " << buffer << endl;
    }

    
}
// "creategroup" command handler  groupname:groupdesc
void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "creategroup command invalid!" << endl;
        return;
    }

    string groupname = str.substr(0, idx);//substr（起始下标，长度）
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getID();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send creategroup msg error -> " << buffer << endl;
    }
}
// "addgroup" command handler
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getID();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addgroup msg error -> " << buffer << endl;
    }
}
// "groupchat" command handler   groupid:message
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "groupchat command invalid!" << endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getID();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send groupchat msg error -> " << buffer << endl;
    }
}
// "loginout" command handler
void loginout(int clientfd, string)//直接组织json字符串，和上面几个函数类似
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getID();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send loginout msg error -> " << buffer << endl;
    }
    else
    {//登出，发送完消息，mainmenu就退出
        isMainMenuRunning = false;
    }   
}

// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()///是一个C++11的方法
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}
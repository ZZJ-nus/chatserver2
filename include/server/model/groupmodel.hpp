#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <string>
#include <vector>
using namespace std;

// 维护群组信息的操作接口方法
class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);
    // 加入群组
    void addGroup(int userid, int groupid, string role);//成员id，群组id，角色
    // 查询用户所在群组信息
    vector<Group> queryGroups(int userid);//用户登录后，返回加入的群组列表
    // 根据指定的groupid查询群组用户id列表，除userid自己，主要用户群聊业务给群组其它成员群发消息(之前chatservice有用户对应的通信conn)
    vector<int> queryGroupUsers(int userid, int groupid);
    //发送群消息，就要知道该群中其他成员的用户id，进而通过之前map表中存储的用户conn，通过服务器进行转发。不在线就存储离线消息
};

#endif
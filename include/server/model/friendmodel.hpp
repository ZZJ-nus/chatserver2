#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include<vector>
#include"user.hpp"//两个表的联合查询
using namespace std;

//维护好友信息的操作接口方法
class FriendModel{
public:
    //添加好友关系
    void insert(int userid, int friendid);

    //返回用户好友列表  friendid--->name+state
    //两个表联合查询
    vector<User> query(int userid);
};

#endif
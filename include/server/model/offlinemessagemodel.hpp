#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include<string>
#include<vector>
using namespace std;

//提供离线消息表的操作接口方法
//返回消息后，要删除原有消息
class OfflineMsgModel{
public:
    //存储用户的离线消息
    void insert(int userid, string msg);

    //删除用户的离线消息
    void remove(int userid);

    //查询用户的离线消息（可能有多个）,json可以直接序列化vector数据
    vector<string> query(int userid);

private:
};

#endif
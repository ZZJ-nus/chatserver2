#include"friendmodel.hpp"
#include"db.h"



//添加好友关系
void FriendModel::insert(int userid, int friendid){//类似offlinemsg表增加方法
    //1、组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values('%d','%d')",userid,friendid);


    //2、连接数据库，发送SQL语句
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

//返回用户好友列表  userid--->friendid--->name+state
//两个表联合查询
vector<User> FriendModel::query(int userid){
    //1、组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on b.friendid=a.id where b.userid=%d",userid);
    //表的联合查询

    //2、连接数据库，发送SQL语句
    vector<User> vec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES *res = mysql.query(sql);
        if(res!=nullptr){//查询成功
            
            //把userid的所有离线消息放入vector中返回
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr){
                User user;
                user.setID(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user); 
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}
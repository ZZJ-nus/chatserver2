#include"offlinemessagemodel.hpp"
#include"db.h"//需要数据库






//存储用户的离线消息
void OfflineMsgModel::insert(int userid, string msg){//类似usermodel中的insert方法
    //1、组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values('%d','%s')",userid,msg.c_str());


    //2、连接数据库，发送SQL语句
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

//删除用户的离线消息
void OfflineMsgModel::remove(int userid){//类似插入，也是一个update
    //1、组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid=%d",userid);


    //2、连接数据库，发送SQL语句
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

//查询用户的离线消息（可能有多个）,json可以直接序列化vector数据
vector<string> OfflineMsgModel::query(int userid){//类似usermodel中的查询
    //1、组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid=%d",userid);//只需要msg，就不要select*了
    //根据userid，从offlinemsg表中查msg

    //2、连接数据库，发送SQL语句
    vector<string> vec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES *res = mysql.query(sql);
        if(res!=nullptr){//查询成功
            
            //把userid的所有离线消息放入vector中返回
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr){
                vec.push_back(row[0]);//只有一个字段，所以是row[0]
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}
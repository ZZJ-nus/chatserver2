#include"usermodel.hpp"
#include"db.h"

#include<iostream>
using namespace std;
bool UserModel::insert(User &user)
{
    //1、组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name,password,state) values('%s','%s','%s')",
        user.getName().c_str(),user.getPwd().c_str(),user.getState().c_str());
    //刚注册还没登录肯定是offline，但是默认user的state就是offline
    //通过.c_str()将string转成char*类型

    //2、连接数据库，发送SQL语句
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            //获取插入成功的用户数据生成的主键ID
            user.setID(mysql_insert_id(mysql.getConnection()));//mysql_insert_id是MySQL自带的方法，获取连接的主键
            return true;
        }
    }

    return false;
}

//根据用户号码查询用户信息
User UserModel::query(int id){
    //1、组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id=%d",id);

    //2、连接数据库，发送SQL语句
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES *res = mysql.query(sql);
        if(res!=nullptr){//查询成功
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row!=nullptr){//有数据
                //row的0123对应user表的四个字段
                User user;
                user.setID(atoi(row[0])); // row[]取出是字符串atoi转成整数
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);

                mysql_free_result(res);//因为res是指针，所以需要释放资源，否则内存会不断泄露
                return user;
            }
        }
    }
    

    return User();//没有找到，默认的id返回值是-1，使用的时候通过id判断
}



//更新用户的状态信息
bool UserModel::updateState(User user){
    //1、组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id =%d",user.getState().c_str(),user.getID());

    //2、连接数据库，发送SQL语句
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            return true;
        }
    }

    return false;
}


//重置用户的状态信息
void UserModel::resetState(){
    //1、组装SQL语句
    // char sql[1024] = {0};
    // sprintf(sql, "update user set state = 'offline' where state ='online'");

    char sql[1024] = "update user set state = 'offline' where state ='online'";

    //2、连接数据库，发送SQL语句
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}





#include"db.h"
#include<muduo/base/Logging.h>//日志
#include<iostream>


// 数据库配置信息
static string server = "127.0.0.1";
static string user = "root";
static string password = "zzj159";
static string dbname = "chat";

MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}
// 释放数据库连接资源
MySQL::~MySQL()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}
// 连接数据库
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)//连接成功
    {
        //C和C++默认编码是ascll码，如果不设置，从mysql拉取数据会出错
        mysql_query(_conn, "set names gbk");//使代码支持中文
        LOG_INFO << "connect mysql success";
    }
    else{
        cout <<"aaa"<< mysql_errno(_conn) << endl;
        LOG_INFO << "connect mysql fail";
    }
    return p;
}
// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
            << sql << "更新失败!";
        cout <<"bbb"<< mysql_errno(_conn) << endl;
        return false;
    }
    return true;
}
// 查询操作
MYSQL_RES* MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
            << sql << "查询失败!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

//获取连接（在外部没法访问私有成员变量，这里需要获取mysql生成的主键ID作为用户ID）
MYSQL* MySQL::getConnection(){
    return _conn;
}
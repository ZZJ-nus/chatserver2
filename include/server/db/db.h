#ifndef DB_H
#define DB_H


#include<mysql/mysql.h>
#include<string>
using namespace std;


// 数据库操作类
class MySQL
{
    public:
    // 初始化数据库连接
        MySQL();

        // 释放数据库连接资源
        ~MySQL();

        // 连接数据库
        bool connect();

        // 更新操作
        bool update(string sql);

        // 查询操作
        MYSQL_RES *query(string sql);

        //获取连接（在外部没法访问私有成员变量，这里需要获取mysql生成的主键ID作为用户ID）
        MYSQL *getConnection();

    private:
        MYSQL *_conn;
};

#endif
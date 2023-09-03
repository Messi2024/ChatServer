#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>

#include "sql_connection_pool.h"

using namespace std;

// 数据库操作类(RAII)
class MySQL
{
public:
    // 从数据库连接池中取连接
    MySQL();
    // 归还数据库连接到连接池中
    ~MySQL();
    // 更新操作
    bool update(string sql);
    // 查询操作
    MYSQL_RES *query(string sql);
    //获取连接
    MYSQL* getConnection();
private:
    MYSQL* _conn;
    connection_pool* connPool;
};
#endif
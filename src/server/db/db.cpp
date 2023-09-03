#include "db.h"
#include "sql_connection_pool.h"
#include <muduo/base/Logging.h>
#include <iostream>
// 数据库配置信息
static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "chat";

// 从数据库连接池中获得数据库连接
MySQL::MySQL()
{
    connPool = connection_pool::GetInstance();
    _conn = connPool->GetConnection();
}

// 将数据库连接放回连接池
MySQL::~MySQL()
{
    connPool->ReleaseConnection(_conn);
}

// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "更新失败！";
        return false;
    }
    return true;
}

// 查询操作
MYSQL_RES *MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "查询失败！";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

// 获取连接
MYSQL* MySQL::getConnection()
{
    return _conn;
}
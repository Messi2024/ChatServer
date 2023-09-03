#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include "sql_connection_pool.h"
#include <muduo/base/Logging.h>

using namespace std;

// 构造函数
connection_pool::connection_pool()
{
    freeConn = 0;
}

// 单例模式,程序运行期间只会存在一个数据库对象
connection_pool *connection_pool::GetInstance()
{
    static connection_pool connPool;
    return &connPool;
}

// 初始化数据库连接池
void connection_pool::init(string url, string User, string PassWord, string DBName, int Port, int maxConn)
{
    // 初始化数据库连接池信息
    m_url = url;
    m_Port = Port;
    m_User = User;
    m_PassWord = PassWord;
    m_DatabaseName = DBName;

    // 创建maxConn条数据库连接
    for (int i = 0; i < maxConn; i++)
    {
        MYSQL *_conn = mysql_init(nullptr);

        // 使用mysql_real_connect建立到数据库的连接
        _conn = mysql_real_connect(_conn, url.c_str(), User.c_str(), PassWord.c_str(), DBName.c_str(), Port, nullptr, 0);

        if (_conn != nullptr)
        {
            // C和C++代码默认的编码字符是ASCII，如果不设置，从MySQL上拉下来的中文显示乱码
            // 插入的话也可能会出错
            mysql_query(_conn, "set names utf8");
            LOG_INFO << "connect mysql success!";
        }
        else
        {
            LOG_INFO << "connect mysql fail!";
        }
        // 更新连接池和空闲连接数量
        connList.push_back(_conn);
        ++freeConn;
    }

    // 将信号量初始化为最大连接数
    sem_init(&reserve, 0, maxConn);
    maxConn = freeConn;
}
// 当有需要时，返回一个数据库连接，同时更新空闲和使用的连接数以及连接池
MYSQL *connection_pool::GetConnection()
{
    MYSQL *_conn = nullptr;

    if (0 == connList.size())
    {
        return nullptr;
    }

    // 操作信号量，如果为0则需要等待变为非0值,wait操作给信号量减一
    sem_wait(&reserve);

    // 防止操作同一个连接， 加锁
    lock.lock();

    _conn = connList.front();
    connList.pop_front();

    --freeConn;

    lock.unlock();
    return _conn;
}

// 释放连接
bool connection_pool::ReleaseConnection(MYSQL *con)
{
    if (con == nullptr)
    {
        return false;
    }
    lock.lock();

    connList.push_back(con);
    ++freeConn;

    lock.unlock();

    // 释放连接信号量原子加一
    sem_post(&reserve);
    return true;
}


// 销毁所有连接
void connection_pool::DestroyPool()
{
    lock.lock();
    if (connList.size() > 0)
    {
        // 迭代器遍历，关闭数据库连接
        list<MYSQL *>::iterator it;
        for (it = connList.begin(); it != connList.end(); ++it)
        {
            MYSQL *con = *it;

            // 使用mysql_close关闭连接
            mysql_close(con);
        }
        freeConn = 0;
        connList.clear();
    }
    lock.unlock();
}

// 析构，摧毁连接池
connection_pool::~connection_pool()
{
    DestroyPool();
}

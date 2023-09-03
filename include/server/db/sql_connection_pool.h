#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include <mutex>
#include <semaphore.h>

using namespace std;

class connection_pool
{
public:
    MYSQL *GetConnection();              // 获取数据库连接
    bool ReleaseConnection(MYSQL *conn); // 释放连接
    void DestroyPool();                  // 销毁所有连接

    // 线程安全的懒汉单例模式,程序运行期间只会存在一个数据库对象
    static connection_pool *GetInstance();

    // 初始化数据库,数据库配置信息
    void init(string url = "127.0.0.1", string User = "root", string PassWord = "123456", string DataBaseName = "chat", int Port = 3306, int MaxConn = 50);

private:
    connection_pool(); // 构造、析构放在私有里，保证只有一个对象
    ~connection_pool();
    mutex lock;             // 互斥锁
    list<MYSQL *> connList; // 连接池
    int freeConn;           // 剩余连接数
    int maxConn;            // 总连接数
    sem_t reserve;         // 剩余连接
public:
    string m_url;          // 主机地址
    string m_Port;         // 端口号
    string m_User;         // 数据库登陆用户名
    string m_PassWord;     // 数据库密码
    string m_DatabaseName; // 数据库名
};
#endif
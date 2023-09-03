#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

#include "sql_connection_pool.h"

using namespace muduo;
using namespace muduo::net;

class ChatServer
{
public:
    // 初始化聊天服务器对象
    ChatServer(EventLoop *loop,
               const InetAddress &listenAddr,
               const string &nameArg);
    // 启动服务
    void start();

private:
    // 上报连接相关信息的回调函数(连接建立、断开、销毁都会调用这个函数)
    void onConnection(const TcpConnectionPtr &conn);

    // 上报读写事件相关信息的回调函数
    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *,
                   Timestamp);
    TcpServer _server; //组合的muduo库，实现服务器功能的类对象
    EventLoop *_loop;   //指向事件循环对象的指针
    connection_pool *m_connPool; // 数据库连接池
};
#endif
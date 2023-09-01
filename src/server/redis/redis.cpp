#include "redis.hpp"
#include <iostream>
using namespace std;

Redis::Redis() : _publish_context(nullptr), _subscribe_context(nullptr)
{
}

Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }
    if (_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}

// 连接redis服务器
bool Redis::connect()
{
    // 负责publish发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _publish_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 负责subscribe订阅消息的上下文连接
    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _subscribe_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 在单独的线程中，监听通道上的事件，有消息给业务层进行上报
    // 之所以在单独的线程中进行，是因为监听是阻塞的
    thread t([&]()
             { observer_channel_message(); });
    t.detach();
    cout << "connect redis-server success!" << endl;

    return true;
}

// 向redistribution指定的通道channel发布消息
bool Redis::publish(int channel, string message)
{
    // 相当于往客户端发了一条命令
    // 它实际上相当于调用了这三个函数：
    // 1.redisAppendCommand 把命令写入本地发送缓冲区
    // 2.redisBufferWrite 把本地缓冲区的命令通过网络发送出去
    // 3.redisGetReply 阻塞等待redis server响应消息
    redisReply *reply = (redisReply *)redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (nullptr == reply)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    // 释放掉返回值
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    //SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，因此这里只做订阅通道，不接收消息
    //通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    //这里只负责发送命令，不阻塞接收redis server相应消息，否则和notifyMsg线程抢占响应资源

    //把命令写入本地发送缓冲区
    if(REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    //redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }

    return true;
}

// 向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if(REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }

    //redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

// 在独立线程中接受订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while(REDIS_OK == redisGetReply(this->_subscribe_context, (void**)&reply))
    {
        //订阅收到的消息是一个带三元素的数组
        if(reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            //给业务层上报通道上发生的消息
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }

        freeReplyObject(reply);
    }

    cerr << ">>>>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<<<" << endl;
}

// 初始化向业务层上报通道消息的回调对象
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    _notify_message_handler = fn;
}
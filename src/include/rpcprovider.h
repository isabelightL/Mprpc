#pragma once 
#include "google/protobuf/service.h"
#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <string>
#include<functional>//-----绑定器头文件
#include<google/protobuf/descriptor.h>//------NotifyService类里调用
#include<unordered_map>
#include"logger.h"

//框架提供的专门服务发布rpc服务的网络对象类
class RpcProvider
{
public:
    //这里是框架提供给外部使用的，可以发布rpc方法的函数接口,抽象类接收子类，比如UserService类等
    void NotifyService(google::protobuf::Service *service);

    //启动rpc 服务节点，开始提供rpc远程调用服务
    void Run();
private:
    //组合了TcpServer--------只一次调用，所以写在run方法里
    //std::unique_ptr<muduo::net::TcpServer> m_tcpserverPtr;//------用到智能指针，独占指针
    
    //组合EventLoop--------有可能调用多次，所以作为成员变量
    muduo::net::EventLoop m_eventLoop;

    //新的socket连接回调
    void OnConnection(const muduo::net::TcpConnectionPtr&);

     //已建立连接用户的读写事件回调
    void OnMessage(const muduo::net::TcpConnectionPtr&,muduo::net::Buffer*,muduo::Timestamp); 

    

   //-------------------2、RpcProvider::NotifyService 相关---------------------

    //service 类型服务信息
    struct ServiceInfo
    {
        google::protobuf::Service *m_service;//保存具体的服务对象
        std::unordered_map<std::string,const google::protobuf::MethodDescriptor*> m_methodMap;//第一个key是methd name 方法的名字， 第二个value 是方法的描述类型
    };
    //存储注册成功的服务对象和其服务方法的所有信息
    std::unordered_map<std::string,ServiceInfo> m_serviceMap;//第一个key是服务的名字 第二个value 是服务的信息


    //----------------------3、RpcProvider::OnMessage相关-----------------------------
    //由于rpcprovider.cc的service->CallMethod（）需要一个:google::protobuf::Closure * done，
    //这个是Closure的回调操作，用于序列化rpc的响应和网络发送
    void SendRpcResponse(const muduo::net::TcpConnectionPtr&,google::protobuf::Message*);

};
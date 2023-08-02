#include<iostream>
#include<string>
#include"user.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
/*
 UserService 原来是一个本地服务，提供了两个进程内的本地方法，Login和GetFriendLists
 需求:是有没有这样的一个框架，能够让我把UserService这个类，
              生成的对象的Login方法，直接变成一个RPC远程方法，
              
              不仅仅可以在进程内部调用，在同一台机器上的其他进程调用这个方法，甚至在其他的机器上来调用这个方法
要进行远程请求一个方法调用，你不仅仅得带方法的名字，还得带方法的参数，在这儿把方法执行完了以后，还有方法的这个返回值，
所这里边涉及了这个方法名字，方法参数，以及方法的这个返回值的数据的序列化跟反序列化*/
class UserService :public fixbug::UserServiceRpc//把在协议生成的user.pb.h中的UserServiceRpc类，在服务端里继承
{
    public:
        bool Login(std::string name,std::string pwd)
        {
            std::cout<<"doing local service login"<<std::endl;
            std::cout<<"name: "<<name<<" pwd: "<<pwd<<std::endl;
            return false;
        }

        bool Register(uint32_t id,std::string name,std::string pwd)
        {
            std::cout<<"doing local service register"<<std::endl;
            std::cout<<"id: "<<id<<" name: "<<name<<" pwd: "<<pwd<<std::endl;
            return true;
        }
       
        /*重写基类UserServiceRpc 的虚函数 下面这些方法都是框架直接调用的
         1. caller   ===>   Login(LoginRequest)  => muduo =>   callee 
         2. callee   ===>    Login(LoginRequest)  => 交到下面重写的这个Login方法上了
        */
        void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done)
        {
            // 1、框架给业务上报了请求参数LoginRequest，业务获取相应数据做本地业务
            std::string name = request->name();
            std::string pwd = request->pwd();

            // 2、做本地业务
            bool login_result = Login(name, pwd); 

             // 3、把响应写入  包括错误码、错误消息、返回值
            fixbug::ResultCode *code = response->mutable_result();
            code->set_errcode(0);
            code->set_errmsg("");
            response->set_sucess(login_result);

            // 4、执行回调操作   执行响应对象数据的序列化和网络发送（都是由框架来完成的）
            done->Run();

        }

        void Register(::google::protobuf::RpcController* controller,
                       const ::fixbug::RegisterRequest* request,
                       ::fixbug::RegisterResponse* response,
                       ::google::protobuf::Closure* done)
        {
            uint32_t id=request->id();
            std::string name=request->name();
            std::string pwd=request->pwd();

             bool register_result = Register(id,name, pwd);

             fixbug::ResultCode *code = response->mutable_result();
            code->set_errcode(0);
            code->set_errmsg("");
            response->set_sucess(register_result);

            done->Run();


        }

};
int main(int argc, char **argv)
{
    // 调用框架的初始化操作,配置文件等  一开始希望使用provider -i config.conf命令到config.conf文件中读取
    MprpcApplication::Init(argc, argv);

    // provider是一个rpc网络服务对象。把UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new UserService());

    // 启动一个rpc服务发布节点   Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run();

    return 0;
}
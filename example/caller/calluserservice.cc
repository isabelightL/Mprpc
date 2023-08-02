#include<iostream>
#include "mprpcapplication.h"
#include "user.pb.h"
#include "mprpcchannel.h"

int main(int argc, char **argv)
{
    //整个程序启动后。想使用mprpc框架来享受rpc服务调用，一定需要先调用框架的初始化函数（只初始化一次）
    MprpcApplication::Init(argc,argv);

    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());

    //---------准备好stub.Login()需要的四个参数，并发送给服务端------------------------
    //演示调用远程发布的rpc方法Login
    //1、request参数，由客户端填入
    fixbug::LoginRequest  request;
    request.set_name("zhang san");
    request.set_pwd("123456");

    //2、response 参数，需要等服务端返回来序列化的消息，先定义好
    fixbug::LoginResponse response;

    //发起rpc方法调用 是个同步的调用过程，需要等远程服务端返回消息,RpcChannel->RpcChannel::callMethod .第一四个参数为空指针
    stub.Login(nullptr, &request, &response, nullptr);//RpcChannel->RpcChannel::callMethod 集中来做所有rpc方法调用的参数序列化和网络发送
    
     //一次rpc调用完成，读调用的结果
     if(0== response.result().errcode())
     {
        //返回来errocode，0代表返回消息成功
        std::cout <<"rpc login response success:"<<response.sucess()<<std::endl;
     }
     else
     {
        //响应失败
         std::cout <<"rpc login response error:"<<response.result().errmsg()<<std::endl;
     }

      //---------准备好stub.Register()需要的参数，并发送给服务端------------------------
      //演示调用远程发布的rpc方法Register
       //1、request参数，由客户端填入
    fixbug::RegisterRequest  req;
    req.set_id(2000);
    req.set_name("mprpc");
    req.set_pwd("666666");

    //2、response 参数，需要等服务端返回来序列化的消息，先定义好
    fixbug::RegisterResponse rsp;

    //发起rpc方法调用 是个同步的调用过程，需要等远程服务端返回消息,RpcChannel->RpcChannel::callMethod .第一四个参数为空指针
    stub.Register(nullptr, &req, &rsp, nullptr);//RpcChannel->RpcChannel::callMethod 集中来做所有rpc方法调用的参数序列化和网络发送
    
     //一次rpc调用完成，读调用的结果
     if(0== rsp.result().errcode())
     {
        //返回来errocode，0代表返回消息成功
        std::cout <<"rpc Register response success:"<<rsp.sucess()<<std::endl;
     }
     else
     {
        //响应失败
         std::cout <<"rpc Register response error:"<<rsp.result().errmsg()<<std::endl;
     }

     return 0;
}
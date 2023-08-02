#include<iostream>
#include "mprpcapplication.h"
#include "friend.pb.h"


int main(int argc, char **argv)
{
    //整个程序启动后。想使用mprpc框架来享受rpc服务调用，一定需要先调用框架的初始化函数（只初始化一次）
    MprpcApplication::Init(argc,argv);

    fixbug::FriendServiceRpc_Stub stub(new MprpcChannel());

    //---------准备好stub.GetFriendList()需要的四个参数，并发送给服务端------------------------
    //演示调用远程发布的rpc方法GetFriendList
    //1、request参数，由客户端填入
    fixbug::GetFriendListRequest  request;
    request.set_userid(10000);

    //2、response 参数，需要等服务端返回来序列化的消息，先定义好
    fixbug::GetFriendListResponse response;
    MprpcController controller;//定义一个控制对象，观测rpc在调用过程中出现的一些状态信息

    //发起rpc方法调用 是个同步的调用过程，需要等远程服务端返回消息,RpcChannel->RpcChannel::callMethod .第一四个参数为空指针
    stub.GetFriendList(&controller, &request, &response, nullptr);//RpcChannel->RpcChannel::callMethod 集中来做所有rpc方法调用的参数序列化和网络发送
    if(controller.Failed())
    {
        //代表controller调用失败
        std:: cout <<controller.ErrorText()<<std::endl;

    }
    else
    {
        //一次rpc调用完成，读调用的结果
     if(0== response.result().errcode())
     {
        //返回来errocode，0代表返回消息成功
        std::cout <<"rpc GetFriendList response success! "<<std::endl;
        //打印好友列表
        int size =response.friends_size();
        for(int i=0;i<size;i++)
        {
            std::cout<<"index: "<<(i+1)<<" name: "<<response.friends(i)<<std::endl;
        }
     }
     else
     {
        //响应失败
         std::cout <<"rpc GetFriendList response error: "<<response.result().errmsg()<<std::endl;
     }

    }
     


     return 0;
}

#include<iostream>
#include<string>
#include"friend.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "logger.h"
#include<vector>

class FriendSevice : public fixbug::FriendServiceRpc
{
    //服务端本地方法
    public:
    std::vector<std::string> GetFriendList(uint32_t userid)
    {
        std::cout<<"do GetFriendsList service! userid: "<<userid<< std::endl;
        std::vector<std::string> vec;
        vec.push_back("gao yang");
        vec.push_back("liu hong");
        vec.push_back("wang shuo");
        return vec;
    }

    //重写基类方法
    void GetFriendList(::google::protobuf::RpcController* controller,
                       const ::fixbug::GetFriendListRequest* request,
                       ::fixbug::GetFriendListResponse* response,
                       ::google::protobuf::Closure* done)
    {
        //从request获取参数，做本地业务调用
        uint32_t userid=request->userid();
        std::vector<std::string> friendsList = GetFriendList(userid);


         // 3、把响应写入  包括错误码、错误消息、返回值
            fixbug::ResultCode *code = response->mutable_result();
            code->set_errcode(0);
            code->set_errmsg("");
            
            //本地业务得到的friendsList 填到response里
            for(std::string &name : friendsList)
            {
                std:: string *p =response->add_friends();
                             *p=name;

            }

             // 执行回调操作   执行响应对象数据的序列化和网络发送（都是由框架来完成的）
            done->Run();

    }
};

int main(int argc, char **argv)
{
    LOG_INFO("first log message!");
    LOG_ERR("%s:%s:%d",__FILE__,__FUNCTION__,__LINE__);
    
    
    // 调用框架的初始化操作,配置文件等
    MprpcApplication::Init(argc, argv);

    // provider是一个rpc网络服务对象。把UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new FriendSevice());

    // 启动一个rpc服务发布节点   Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run();

    return 0;
}
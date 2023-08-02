#include"mprpcchannel.h"
#include<string>
#include "./include/rpcheader.pb.h"
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<unistd.h>
#include<errno.h>
#include"mprpcapplication.h"
#include"mprpccontroller.h"
#include "zookeeperutil.h"
/*
封装成约定好的数据格式：
       header_size+service_name method name args_size +args
*/
 //所有通过stub代理对象调用的rpc方法，都走到这里了，统一做rpc方法调用的数据数据序列化和网络发送
void MprpcChannel::CallMethod(const  google::protobuf::MethodDescriptor* method,
                               google::protobuf::RpcController* controller, 
                        const  google::protobuf:: Message* request,
                               google::protobuf::Message* response, 
                                google::protobuf::Closure* done)//要重写的CallMethod方法
{

//---------------------------1、将要发送的信息 序列化封装成约定好的数据格式 -------------------------------------
       
       
       const google::protobuf::ServiceDescriptor* sd=method->service();
       std::string service_name=sd->name();//获得服务名 service_name
       std::string method_name =method->name();//获得方法名

       //对第三部分参数进行序列化得到args_str，并获取序列化后的参数长度args_size
       uint32_t args_size=0;
       std::string args_str;
       if(request->SerializeToString(&args_str))
       {
              args_size = args_str.size();//得到args_size

       }
       else
       {
              controller->SetFailed("serialize request error!");
              return;
       }

       //定义rpc的请求头rpcHeader
       mprpc::RpcHeader  rpcHeader;
       rpcHeader.set_service_name(service_name);
       rpcHeader.set_method_name(method_name);
       rpcHeader.set_args_size(args_size);
      
       uint32_t header_size=0;
      //对rpcHeader进行序列化
       std::string rpc_header_str;
       if(rpcHeader.SerializeToString(&rpc_header_str))
       {
              header_size=rpc_header_str.size();
       }
       else
       {
               controller->SetFailed("serialize rpcheader error!");
              return;

       }

       //拼贴组装成要发送的数据
       
       /*1、对于header_size，要四字节按二进制来存储，header_size不能按字符来存储，如果按字符存储解析的时候都不知道前几个字符是属于我们的header_size
        send_rpc_str从零个位置开始写，写四个字节，写一个二进制的一个header_size把这个整数写进去，也就是说我们固定了这个字符串的前四个字节，放的就是一个整数
        */
       std::string send_rpc_str;
       send_rpc_str.insert(0,std::string((char*)&header_size,4));

       //2、对于请求头，直接相加
       send_rpc_str+=rpc_header_str;

       //3、对于参数，直接相加
        send_rpc_str+=args_str;



       
       //打印序列化数据
       std::cout <<"==============================="<<std::endl;
       std::cout <<"header_size:  "<<header_size<<std::endl;
       std::cout <<"rpc_header_str :"<<rpc_header_str<<std::endl;
        std::cout <<"service_name:  "<<service_name<<std::endl;
        std::cout <<"method_name:  "<<method_name<<std::endl;
        std::cout <<"args_size:  "<<args_size<<std::endl;
        std::cout <<"args_str:  "<<args_str<<std::endl;
        std::cout <<"================================"<<std::endl;

//------------------------2、通过网络发送信息给服务端-----------------------------
//使用tcp编程完成rpc方法的远程调用

int clientfd=socket(AF_INET,SOCK_STREAM,0);
if(-1 == clientfd)
{
       char errtxt[512]={0};
       sprintf(errtxt,"create socket error! errno: %d",errno);
       controller->SetFailed(errtxt);
       return ;
}
else
{
       //从配置文件读rpcserver的信息：ip地址和端口值
       //std:: string ip=MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
       //uint16_t port=atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());

        // rpc调用方想调用service_name的method_name服务，需要查询zk上该服务所在的host信息
       // rpc调用方想调用service_name的method_name服务，需要查询zk上该服务所在的host信息
       ZkClient zkCli;
       zkCli.Start();
       //  /UserServiceRpc/Login
       std::string method_path = "/" + service_name + "/" + method_name;
       // 192.168.85.135:8000
       std::string host_data = zkCli.GetData(method_path.c_str());
       if (host_data == "")
       {
              controller->SetFailed(method_path + " is not exist!");
              return;
       }
       int idx = host_data.find(":");
       if (idx == -1)
       {
              controller->SetFailed(method_path + " address is invalid!");
              return;
       }
       std::string ip = host_data.substr(0, idx);
       uint16_t port = atoi(host_data.substr(idx+1, host_data.size()-idx).c_str()); 


       
       
       struct sockaddr_in server_addr;
       server_addr.sin_family=AF_INET;
       server_addr.sin_port =htons(port);
       server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

       //连接rpc服务节点
       int ret=connect(clientfd,(struct sockaddr *)&server_addr,sizeof(server_addr));
       if(ret==-1)
       {
           close(clientfd);

           char errtxt[512]={0};
           sprintf(errtxt,"connect error! errno: %d",errno);
           controller->SetFailed(errtxt);
           return ;
           
       }

       //发送rpc请求

       if(-1 == send(clientfd,send_rpc_str.c_str(),send_rpc_str.size(),0))
       {
              
            close(clientfd);
            char errtxt[512]={0};
            sprintf(errtxt,"send  error! errno: %d",errno);
            controller->SetFailed(errtxt);
            return ;
            
       }

       //接收rpc请求的响应值
       char recv_buf[1024]={0};//1024是最大值，但实际接收的未必是最大值
       
       int recv_size=0;
       if(-1 == (recv_size=recv(clientfd,recv_buf,1024,0)))
       {
            close(clientfd); 
            char errtxt[512]={0};
            sprintf(errtxt,"recv error! errno: %d",errno);
            controller->SetFailed(errtxt);
            return ;
       }


       //反序列化RPC调用的响应数据
      // std::string response_str(recv_buf,0,recv_size);
       //if(!response->ParseFromString(response_str))
       if(!response->ParseFromArray(recv_buf,recv_size))
       {
              close(clientfd);
              char errtxt[512]={0};
              sprintf(errtxt,"parse error! response_str: %s",recv_buf);
              controller->SetFailed(errtxt);
              return ;
       }
       close(clientfd);


}
}





       

       



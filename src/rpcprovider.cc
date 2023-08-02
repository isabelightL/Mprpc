#include "rpcprovider.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h" //-----------protobuf生成的协议序列化
#include "zookeeperutil.h"



/*
service_name =>  service描述   
                        =》 service* 记录服务对象
                        method_name  =>  method方法对象
json   protobuf
*/

 //-----------这里是框架提供给外部使用的，可以发布rpc方法的函数接口,抽象类接收子类，比如UserService类等---------------
    void RpcProvider::NotifyService(google::protobuf::Service *service){
      

      ServiceInfo service_info;
      
      //获取服务对象的描述信息
      const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();


      //获取服务的名字
      std::string service_name = pserviceDesc->name();

      //获取服务对象service 的方法的数量
      int methodCnt = pserviceDesc->method_count();


       //std::cout << "service_name:" << service_name << std::endl;
       LOG_INFO("service_name: %s", service_name.c_str());

    for (int i=0; i < methodCnt; ++i)
    {
        // 获取了服务对象指定下标的服务方法的描述（抽象描述） UserService   Login
        const google::protobuf::MethodDescriptor* pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        //std::cout << "method_name:" << method_name << std::endl;
        LOG_INFO("method_name: %s", method_name.c_str());

        service_info.m_methodMap.insert({method_name, pmethodDesc});

    }
    service_info.m_service =service;

    //
    m_serviceMap.insert({service_name,service_info});

    }

    //-----------------------------启动rpc 服务节点，开始提供rpc远程调用服务（网络服务，调用muduo）------------------------------------
    void RpcProvider::Run(){

      //首先通过配置文件conf获得解析对应字符串，即获得ip地址和端口号
      std:: string ip=MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
      uint16_t port=atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
      muduo::net::InetAddress address(ip,port);


      
      //创建TcpServer对象
      muduo::net::TcpServer server(&m_eventLoop,address,"RpcProvider");

      //绑定连接回调和消息读写回调方法，分离了网络连接代码和业务代码
      server.setConnectionCallback(std::bind(&RpcProvider::OnConnection,this,std::placeholders::_1));

      server.setMessageCallback(std::bind(&RpcProvider::OnMessage,this,std::placeholders::_1,
                                                                        std::placeholders::_2,
                                                                        std::placeholders::_3));

      //设置muduo库的线程数量
      server.setThreadNum(4);//1个工作线程，3个业务线程

      

       // 把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
    // session timeout   30s     zkclient 网络I/O线程  1/3 * timeout 时间发送ping消息
    ZkClient zkCli;
    zkCli.Start();
    // service_name为永久性节点    method_name为临时性节点
    for (auto &sp : m_serviceMap) 
    {
        // /service_name   /UserServiceRpc
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(), nullptr, 0);
        for (auto &mp : sp.second.m_methodMap)
        {
            // /service_name/method_name   /UserServiceRpc/Login 存储当前这个rpc服务节点主机的ip和port
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            // ZOO_EPHEMERAL表示znode是一个临时性节点
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }



      //如果run成功，进行提示，rpc服务端准备启动，打印信息
      std::cout << "RpcProvider start service at ip: "<<ip<<" port: "<<port<<std::endl;

      //启动网络服务
      server.start();
      m_eventLoop.loop();

    }


    //建立新的socket连接回调
    void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn)
    {
      if(!conn->connected())
      {
        //和rpc client 的连接断开了
        conn->shutdown();
      }

    }




//-------------------对客户端发送来的消息进行反序列化，并在NotifyService 做的map中查找本地server端是否有发送来的服务方法、服务类 

/*
在框架内部，RpcProvider和RpcConsumer协商好之间通信用的protobuf数据类型
数据包含service_name method_name args   
 
                                 
数据头里有service_name method_name args_size   由于应用程序无法直接处理这样的信息，所以定义proto的message类型，进行消息头的序列化和反序列化
    如16UserServiceLoginzhang san123456       16代表从这儿开始数十六个字符，包含了服务名跟方法名，也包含了参数的长度，详见rpcheader.proto 
      对于参数，为了避免参数字符串的粘包问题，在数据头里除了服务名与方法名，也要定义记录参数的长度
      header_size(4个字节) + header_str + args_str   最终发送来的消息实质上的形式
10 "10"
10000 "1000000"----存的不是string，而是二进制
std::string   insert和copy方法 

序列化与反序列化都交给protobuf做
*/
    //已建立连接用户的读写事件回调,如果远程有一个rpc服务的调用请求，那么OnMessage 方法就会响应
   void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn,muduo::net::Buffer *buffer,muduo::Timestamp)
   {
    //网络上接收的远程rpc调用请求的字符流 Login args
       std:: string recv_buf=buffer->retrieveAllAsString();

       //从二进制的字符流文件中读取前4个字节的内容，放在header_size
       uint32_t header_size=0;
       recv_buf.copy((char*)&header_size,4,0);//把recv_buf的数据从0 pose 开始拷贝，储存前4个字节（即数据头大小）到header_size中

       //根据header_size 读取数据头的原始字符流，反序列化数据，得到rpc请求的详细信息
       std:: string rpc_header_str =recv_buf.substr(4,header_size);//recv_buf的数据从第4个字节开始，读header_size个到rpc_header_str中
      //rpc_header_str 是真正的数据头
       mprpc::RpcHeader rpcHeader;
       std::string service_name;
       std::string method_name;
       uint32_t args_size;

       if(rpcHeader.ParseFromString(rpc_header_str))//protobuf 解析数据头，把数据头的字符串给反序列化
       {
        //数据头反序列化成功
        service_name=rpcHeader.service_name();
        method_name =rpcHeader.method_name();
        args_size =rpcHeader.args_size();

       }
       else
       {
        //数据头反序列化失败
        std::cout<< "rpc_header_str: "<< rpc_header_str<<" parse error!"<<std::endl;
       }

       //------------------------打印调试信息--------------------------------------

       //打印原始字符流
       //获取rpc方法参数的字符流数据
       std::string args_str = recv_buf.substr(4+header_size,args_size);

        //打印反序列化数据解析得到的
       std::cout <<"==============================="<<std::endl;
       std::cout <<"header_size:  "<<header_size<<std::endl;
       std::cout <<"rpc_header_str :"<<rpc_header_str<<std::endl;
        std::cout <<"service_name:  "<<service_name<<std::endl;
        std::cout <<"method_name:  "<<method_name<<std::endl;
        std::cout <<"args_size:  "<<args_size<<std::endl;
        std::cout <<"args_str:  "<<args_str<<std::endl;
        std::cout <<"================================"<<std::endl;


        //获取客户端发来的service 对象 和 method对象,查看server端下NotifyService里的map表中是否有
        auto it=m_serviceMap.find(service_name);
        if(it==m_serviceMap.end())
        {
          std::cout << service_name << "is not exist!"<<std::endl;
          return ;
        }

        auto mit=it->second.m_methodMap.find(method_name);//it->second是ServiceInfo
        if(mit == it->second.m_methodMap.end())
        {
           std::cout << service_name << ": "<< method_name << "is notexist!"<<std::endl;
        }


        //获取服务对象
        google::protobuf::Service *service =it->second.m_service;//见rpcprovider.h 下:NotifyService 相关,获取service对象-----例如：new出来的userservice
        const google::protobuf::MethodDescriptor *method =mit->second;//获取method对象  如 Login

        //生成 rpc方法调用的请求request 和响应的response参数，从抽象层动态的生成method的请求和响应
        google::protobuf::Message *request=service->GetRequestPrototype(method).New();
        //参数还没有序列化，需要先序列化
        if(!request->ParseFromString(args_str))
        {
          std::cout << "request parse error,content: "<< args_str<<std::endl;
        }

        //生成响应的response
        google::protobuf::Message *response=service->GetResponsePrototype(method).New();

        //由于rpcprovider.cc的service->CallMethod（）需要一个:google::protobuf::Closure * done,绑定Closure的回调函数
        google::protobuf::Closure *done=
                                     google::protobuf::NewCallback<RpcProvider,
                                                                    const muduo::net::TcpConnectionPtr&,
                                                                    google::protobuf::Message*>
                                                                    (this,
                                                                    &RpcProvider::SendRpcResponse,
                                                                    conn,
                                                                    response);

                                    
       
 
        //在框架上根据远端rpc请求，调用当前rpc节点上发布的方法
        service->CallMethod(method,nullptr,request,response,done);//对于Login方法相当于new UserService().Login(controller,response,done)，业务层的Login方法被调用



      

   }

//-------这个是Closure的回调操作，用于序列化rpc的响应和网络发送（userservice.cc业务层done.run()后执行）--------
   void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response)
   {
    std::string response_str;
    if(response->SerializeToString(&response_str))//response进行序列化
    {
      //序列化成功，通过muduo网络把rpc方法执行的结果发送回rpc的调用方
      conn->send(response_str);

    }
    else
    {
      std::cout<<"serialize response_str error!"<<std::endl;
    }
    conn->shutdown();//模拟http的短连接服务，由rpc provider主动断开连接

   }
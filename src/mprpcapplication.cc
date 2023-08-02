#include "mprpcapplication.h"
#include <iostream>
#include <unistd.h>
#include <string>

MprpcConfig MprpcApplication::m_config;

void ShowArgsHelp()//对传入的参数进行提示
{
    std::cout<<"format: command -i <configfile>" << std::endl;
}


void MprpcApplication:: Init(int argc,char ** argv)
{
    if (argc < 2)
    {
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }

    int c = 0;
    std::string config_file;
    while((c = getopt(argc, argv, "i:")) != -1)//"i:"是因为必须要传入这样一个参数
    {
        switch (c)
        {
        case 'i'://说明有配置文件
            config_file = optarg;//config_file是string,等于参数，
            break;
        case '?':
            //std::cout<<"invalid args!!"<<std::endl;
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        case ':':
            //std::cout<<"need <configfile>!!"<<std::endl;
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        default:
            break;
        }
    }

    //开始加载配置文件------配置文件另外写一对config.h  config.cc 里面需要写rpcserver_ip=  rpcserver_port   zookeeper_ip=  zookepper_port=
    m_config.LoadConfigFile(config_file.c_str());
    std::cout << "rpcserverip:" << m_config.Load("rpcserverip") << std::endl;
    std::cout << "rpcserverport:" << m_config.Load("rpcserverport") << std::endl;
    std::cout << "zookeeperip:" << m_config.Load("zookeeperip") << std::endl;
    std::cout << "zookeeperport:" << m_config.Load("zookeeperport") << std::endl;
}

MprpcApplication& MprpcApplication::GetInstance()
    {
        static MprpcApplication app;
        return app;
    }

MprpcConfig& MprpcApplication::GetConfig()
{
    return m_config;
}
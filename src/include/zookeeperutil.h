
#pragma once

#include <semaphore.h>
#include <zookeeper/zookeeper.h>
#include<string>
//封装的zk客户端类
class ZkClient
{
    public:
    ZkClient();
    ~ZkClient();
    //zk client 启动 zkserver
    void Start();
    //在zkserver 上根据指定的path创建znode节点
    void Create(const char *path,//节点上保存的路径
                const char *data,//服务的数据
                int datalen,//服务数据的长度
                int state=0  //0代表永久性节点。1代表临时性节点
    );
    //根据参数指定的znode节点路径，获得znode节点的值
    std::string GetData(const char *path);
    private:
        //zk的客户端句柄，通过句柄操作zookeeper
        zhandle_t *m_zhandle;
};
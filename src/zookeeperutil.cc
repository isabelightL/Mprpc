#include "zookeeperutil.h"
#include "mprpcapplication.h"
#include <semaphore.h>
#include <iostream>

// 全局的watcher观察器   zkserver给zkclient的通知
void global_watcher(zhandle_t *zh, int type,
                   int state, const char *path, void *watcherCtx)
{
    if (type == ZOO_SESSION_EVENT)  // 回调的消息类型是和会话相关的消息类型
	{
		if (state == ZOO_CONNECTED_STATE)  // zkclient和zkserver连接成功
		{
			sem_t *sem = (sem_t*)zoo_get_context(zh);
            sem_post(sem);//给信号量资源+1
		}
	}
}

ZkClient::ZkClient() : m_zhandle(nullptr)//构造函数初始化句柄为空
{
}

ZkClient::~ZkClient()
{
    if (m_zhandle != nullptr)//指针不为空，相当于已经和zkserver连接了
    {
        zookeeper_close(m_zhandle); // 关闭句柄，释放资源  类似聊天服务器项目MySQL_Conn
    }
}

// 连接zkserver
void ZkClient::Start()
{
    std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port;
    
	/*
	zookeeper_init函数是发送对zk的连接，但是是异步的连接。zookeeper_init仅表示创建句柄资源成功，不代表连接zookeeper成功
	zookeeper_mt：多线程版本
	zookeeper的API客户端程序提供了三个线程
	API调用线程 
	网络I/O线程  pthread_create  poll
	watcher回调线程 pthread_create  ---------在watcher里zkclient和zkserver连接成功后信号量+1，sem_wait(&sem)不再阻塞继续执行
	*/
    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);//zookeeper库里zk客户端提供的api， global_watcher是回调函数
    if (nullptr == m_zhandle) 
    {
        std::cout << "zookeeper_init error!" << std::endl;
        exit(EXIT_FAILURE);
    }

    //init只代表句柄初始化、创建句柄资源成功了，不代表连接zookeeper资源成功了
    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(m_zhandle, &sem);//给设置的句柄添加一些额外的信息，用了信号量

    sem_wait(&sem);//信号量就是为了：让global_watcher回调函数调用成功，即zookeeper zkserver给zkclient通知连接成功后，信号量资源+1， sem_wait不再阻塞等待j
    std::cout << "zookeeper_init success!" << std::endl;
}

/*
 //在zkserver 上根据指定的path创建znode节点
    void Create(const char *path,//节点上保存的路径
                const char *data,//服务的数据
                int datalen,//服务数据的长度
                int state=0  //0代表永久性节点。1代表临时性节点
    );
*/
void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;
	// 先判断path表示的znode节点是否存在，如果存在，就不再重复创建了
	flag = zoo_exists(m_zhandle, path, 0, nullptr);
	if (ZNONODE == flag) // 表示path的znode节点不存在
	{
		// 创建指定path的znode节点了
		flag = zoo_create(m_zhandle, path, data, datalen,
			&ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);//需要注意state，代表临时性节点还是永久性节点，默认创建永久性节点，否则是临时性节点
		if (flag == ZOK)
		{
			std::cout << "znode create success... path:" << path << std::endl;
		}
		else
		{
			std::cout << "flag:" << flag << std::endl;
			std::cout << "znode create error... path:" << path << std::endl;
			exit(EXIT_FAILURE);
		}
	}
}

// 根据指定的path，获取znode节点的值
std::string ZkClient::GetData(const char *path)
{
    char buffer[64];
	int bufferlen = sizeof(buffer);
	int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);//以同步的方式获得znode节点的值，并写到buffer里
	if (flag != ZOK)
	{
		std::cout << "get znode error... path:" << path << std::endl;
		return "";
	}
	else
	{
		return buffer;
	}
}
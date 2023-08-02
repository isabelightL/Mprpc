#问题一
一开始没有用zookeeper：远程过程调用的ip 和port写在test.conf文件里，这样是非常低效的，后来借鉴网上的zookeeper中间件，将远程过程调用的ip 和port发布在zookeeper上。

#问题二
zookeeper 的全局 watcher 功能这一块，一开始也并没有意识到需要同步的操作，后来也是翻阅资料解决了这个问题。具体来说，zookeeper init中调用
global_watcher回调机制（即连接zookeeper时，如果回调函数调用成功，则代表zookeeper连接成功，信号量+1）
![](Mprpc/problem/problem2_1.png)
![](Mprpc/problem/problem2_2.png)

#问题三
在写日志系统时几次出错，
![](Mprpc/problem/problem3_1.png)
检查之前在整个框架中集成日志模块logger，一开始为了方便打印错误引入了muduo自带的日志头文件，后来没有检查，在自己写日志文件想引入时就报了LOG_INFO redifined.怀疑发生了冲突。修改之后又发现src文件夹下的CMakeLists.txt文件并没有引入logger.cc,修改后不再报错。
![](Mprpc/problem/problem3_2.png)

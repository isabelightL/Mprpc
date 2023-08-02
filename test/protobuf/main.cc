#include "test.pb.h"
#include<iostream>
#include <string>

using namespace fixbug;







//----------------int main1保存上一节protobuf是如何做到基本的序列化与反序列化，解析单个数据的

int main1()
{
    //先打包做一个LoginRequest 对象
    LoginRequest req;
    req.set_name("zhang san");
    req.set_pwd("123456");


   
    //对象序列化到send_str 里
    std:: string send_str;
    if(req.SerializeToString(& send_str))
    {
        std::cout<<send_str.c_str()<<std::endl;
    }


    //从 send_str 反序列化成一个对象
    LoginRequest reqB;
    if(reqB.ParseFromString(send_str))
    {
        std::cout << reqB.name()<<std::endl;
        std::cout << reqB.pwd()<<std::endl;

    }

    return 0;


}

//int main 看涉及到列表数据protobuf 怎么序列化和反序列化
int main()
{
    GetFriendListsResponse rsp;
    ResultCode *rc=rsp.mutable_result();//为了获取rsp里的错误码类封装并进行设置修改
    rc->set_errcode(0);
    rc->set_errmsg(" GetFriendListsResponse fail!");

    //创建好友列表里的第一个好友
    User *user1=rsp.add_friend_list();
    user1->set_name("zhang san");
    user1->set_age(20);
    user1->set_sex(User::MAN);

    //创建好友列表里的第二个好友
    User *user2=rsp.add_friend_list();
    user2->set_name("li si");
    user2->set_age(22);
    user2->set_sex(User::MAN);


    std::cout <<rsp.friend_list_size()<<std::endl;
    // std::cout <<rsp.friend_list[0].name<<std::endl;


    
    return 0;
}
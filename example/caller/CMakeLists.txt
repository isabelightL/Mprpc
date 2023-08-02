#set(SRC_LIST calluserservice.cc ../user.pb.cc)
set(SRC_LIST callfriendservice.cc ../friend.pb.cc)
add_executable(consumer ${SRC_LIST})

target_link_libraries(consumer mprpc protobuf)#consumer的编译需要用到我们框架的代码，那他就应该是动态链接我们框架的so库,provider链接 mprpc protobuf的动态库



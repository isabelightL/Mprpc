#set(SRC_LIST userservice.cc ../user.pb.cc)
set(SRC_LIST friendservice.cc ../friend.pb.cc)
add_executable(provider ${SRC_LIST})

target_link_libraries(provider mprpc protobuf)#provide的编译需要用到我们框架的代码，那他就应该是动态链接我们框架的so库,provider链接 mprpc protobuf的动态库



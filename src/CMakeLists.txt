#aux_source_directory(. SRC_LIST)
set(SRC_LIST 
    mprpcapplication.cc 
    mprpcconfig.cc 
    rpcheader.pb.cc 
    rpcprovider.cc 
    mprpcchannel.cc
    mprpccontroller.cc
    logger.cc
    zookeeperutil.cc
    )
add_library(mprpc ${SRC_LIST}) #mprpc 由SRC_LIST这些文件生成而来，生成一个SHARED动态库

target_link_libraries(mprpc muduo_net muduo_base pthread zookeeper_mt)
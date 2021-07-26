PROJECT(protobuf-rpc-demo)
cmake_minimum_required(VERSION 3.2)
add_compile_options(-std=c++11)

include_directories(
    ./src
    /root/src/boost_1_53_0/boost
    /root/src/protobuf-3.7.0/include
)
link_directories(
    /root/src/protobuf-3.7.0/lib
    /root/src/boost_1_53_0/stage/lib
    ./build)

set(LIBMYRPC_SRC ./src/rpc_meta.pb.cc)
add_library(myrpc ${LIBMYRPC_SRC})
add_executable(server ./src/echo.pb.cc ./src/test_myrpc.cpp)
target_link_libraries(server libprotobuf.a libboost_signals.a libboost_system.a -lpthread myrpc)
add_executable(client ./src/echo.pb.cc ./src/test_myrpc_client.cpp)
target_link_libraries(client libprotobuf.a libboost_signals.a libboost_system.a -lpthread myrpc)

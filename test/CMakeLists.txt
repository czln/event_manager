cmake_minimum_required(VERSION 3.10)
project(unitest)

add_executable(unihandle unihandle.cpp)
add_test(NAME test_unihandle COMMAND unihandle)

find_package(Threads)
add_executable(unithreadpool unithrdpool.cpp)
target_link_libraries(unithreadpool ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME test_unithreadpool COMMAND unithreadpool)

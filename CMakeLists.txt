cmake_minimum_required(VERSION 3.16)
project(event_manager)

set(CMAKE_CXX_STANDARD 17)

include_directories(.)

find_package(Threads)

# add_executable(${PROJECT_NAME} test.cpp)

# target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

# unitests
# add_test(unithrdpool unitests/unithrdpool.cpp)
# target_link_libraries(unithrdpool ${CMAKE_THREAD_LIBS_INIT})
enable_testing()
add_subdirectory(test)

# add_executable(unihandle unitests/unihandle.cpp)


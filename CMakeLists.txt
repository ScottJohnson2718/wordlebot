cmake_minimum_required(VERSION 3.23)
project(wordlebot)

set(CMAKE_CXX_STANDARD 17)

#if you want to see std::string in the debugger then you need this line
#https://stackoverflow.com/questions/58578615/cannot-inspect-a-stdstring-variable-in-lldb
add_compile_options(-fstandalone-debug -fno-limit-debug-info -stdlib=libc++)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

add_executable(wordlebot main.cpp)

cmake_minimum_required(VERSION 3.23)
project(wordlebot)

set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} main.cpp WordTree.h WordQuery.cpp WordQuery.h Entropy.h)

#https://stackoverflow.com/questions/58578615/cannot-inspect-a-stdstring-variable-in-lldb
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    #if you want to see std::string in the debugger then you need this line
    # It still didn't work
    target_compile_options(${PROJECT_NAME} PUBLIC -fstandalone-debug -fno-limit-debug-info -stdlib=libc++)

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} PUBLIC -D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4267) # conversion from size_t
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4244) # conversion from double to float
    target_compile_options(${PROJECT_NAME} PRIVATE /wd4305) # truncation from double to float

    target_compile_options(${PROJECT_NAME} PRIVATE "/MP")   # multiprocessor build
endif()



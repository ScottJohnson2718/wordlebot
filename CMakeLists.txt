cmake_minimum_required(VERSION 3.23)
project(wordle)

set(CMAKE_CXX_STANDARD 17)

# Fetch Google Test according to https://google.github.io/googletest/quickstart-cmake.html
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Create the library across the applications
add_library(WordleLib STATIC
        WordQuery.cpp WordQuery.h
        ScoredWord.cpp ScoredWord.h
        Wordle.h Wordle.cpp
        Board.cpp Board.h
        Entropy.h
        Strategy.cpp Strategy.h
        WordleBot.cpp WordleBot.h
        Entropy.cpp)

add_executable(wordlebot main.cpp)
target_link_libraries(wordlebot PUBLIC WordleLib )

# Add the test executable
add_executable(wordleTest wordleTests.cpp)
target_link_libraries(wordleTest PUBLIC WordleLib GTest::gtest_main )

#https://stackoverflow.com/questions/58578615/cannot-inspect-a-stdstring-variable-in-lldb
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    #if you want to see std::string in the debugger then you need this line
    # It still didn't work
    add_compile_options(-fstandalone-debug -fno-limit-debug-info -stdlib=libc++)

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(-D_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/wd4267) # conversion from size_t
    add_compile_options(/wd4244) # conversion from double to float
    add_compile_options(/wd4305) # truncation from double to float

    add_compile_options("/MP")   # multiprocessor build
endif()



cmake_minimum_required(VERSION 3.27)
project(68000_Microcode_Division)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
#set(FETCHCONTENT_FULLY_DISCONNECTED true)

FetchContent_Declare(googletest
    GIT_REPOSITORY     https://github.com/google/googletest.git
    GIT_TAG            main
    CMAKE_ARGS         -Dgtest_force_shared_crt=ON
    -DCMAKE_CXX_STANDARD=23
    )

FetchContent_MakeAvailable(googletest)

add_subdirectory(src)

enable_testing()
add_subdirectory(test)

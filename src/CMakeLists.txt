add_library(68000_Division
    68000_Common.cpp
    68000_Divu.cpp
    68000_Divs.cpp)

target_include_directories(68000_Division
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
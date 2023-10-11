add_executable(
    68000_Division_Test
    68000_Divs_Test.cpp
    68000_Divu_Test.cpp)

target_link_libraries(68000_Division_Test
    68000_Division
    gtest
    gtest_main
    gmock_main)

add_test(NAME 68000_Division_Test COMMAND 68000_Division_Test)
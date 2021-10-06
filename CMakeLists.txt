cmake_minimum_required(VERSION 3.1)
project(cblog C)

set(CMAKE_C_STANDARD 11)
#set(CMAKE_C_COMPILER musl-gcc)
add_executable(cblog main.c)
add_executable(test_layer_fileno tests/test_layer_fileno.c)
target_link_libraries(cblog fcgi pthread)

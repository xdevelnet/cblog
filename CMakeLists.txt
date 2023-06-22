cmake_minimum_required(VERSION 3.1)
project(cblog C)

set(CMAKE_C_STANDARD 99)
#set(CMAKE_C_COMPILER musl-gcc)
include_directories(../ssb/src)

#add_compile_options("$<$<CONFIG:Debug>:-Wall>")
#add_compile_options("$<$<CONFIG:Release>:-Wall>")


#fcgi
add_executable(cblog src/fcgi_version.c)
target_link_libraries(cblog fcgi pthread)
#mongoose
add_executable(cblog_mon src/mon_version.c)
target_sources(cblog_mon PRIVATE ../mongoose/mongoose.c)
target_include_directories(cblog_mon PUBLIC ../mongoose)
#demo app
add_executable(demo src/demo.c)
#tests
add_executable(test_layer_fileno tests/test_layer_fileno.c)

set(COMPILER_OPTIONS "-Wall;-pthread;-Wno-unused-result;-Wno-misleading-indentation;-Wno-unused-parameter")
set(COMPILER_DEBUG_OPTIONS "${COMPILER_OPTIONS};-g;-O0")
set(COMPILER_RELEASE_OPTIONS "${COMPILER_OPTIONS};-O3")
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:Debug>:${COMPILER_DEBUG_OPTIONS}>")
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:Release>:${COMPILER_RELEASE_OPTIONS}>")

# Hey cmake developers, I hate you SO MUCH, you would not believe me, this shit is simply ignored for NO REASON AT ALL

cmake_minimum_required(VERSION 3.20)
project(JinWoVecDBDemo C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(JW_VECDB_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../..)

add_executable(linux_demo linux_demo.c)

target_include_directories(linux_demo PRIVATE
    ${JW_VECDB_SOURCE_DIR}/include
)

target_link_libraries(linux_demo
    ${JW_VECDB_SOURCE_DIR}/build/libjw_vecdb.a
    pthread
    m
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(linux_demo PRIVATE DEBUG)
endif()

install(TARGETS linux_demo DESTINATION bin)

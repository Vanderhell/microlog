cmake_minimum_required(VERSION 3.16)

project(microlog_consumer_cpp LANGUAGES CXX)

find_package(microlog CONFIG REQUIRED)

add_executable(microlog_consumer_cpp main.cpp)
target_link_libraries(microlog_consumer_cpp PRIVATE microlog::microlog)
set_target_properties(microlog_consumer_cpp PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

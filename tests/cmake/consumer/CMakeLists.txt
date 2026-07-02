cmake_minimum_required(VERSION 3.16)

project(microlog_consumer_c LANGUAGES C)

find_package(microlog CONFIG REQUIRED)

add_executable(microlog_consumer_c main.c)
target_link_libraries(microlog_consumer_c PRIVATE microlog::microlog)
set_target_properties(microlog_consumer_c PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)

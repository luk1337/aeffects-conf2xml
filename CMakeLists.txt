cmake_minimum_required(VERSION 3.9)
project(untitled1)

set(CMAKE_CXX_STANDARD 17)

include_directories("${CMAKE_SOURCE_DIR}/include")

set(SOURCE_FILES
        ${CMAKE_SOURCE_DIR}/src/config_utils.cpp
        ${CMAKE_SOURCE_DIR}/src/load_file.c
        ${CMAKE_SOURCE_DIR}/src/main.cpp)
add_executable(aeffects-conf2xml ${SOURCE_FILES})
target_link_libraries(aeffects-conf2xml tinyxml2)
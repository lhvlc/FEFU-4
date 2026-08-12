cmake_minimum_required(VERSION 3.10)
project(OS_Lab2)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(process_launcher STATIC src/process_launcher.cpp)
target_include_directories(process_launcher PUBLIC include)

add_executable(test_launcher src/test_launcher.cpp)
target_link_libraries(test_launcher process_launcher)

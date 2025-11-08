
cmake_minimum_required(VERSION 3.20)

project(slayawaycamp)

find_package(Qt5 REQUIRED Core)

add_executable(slayawaycamp main.cpp)
target_link_libraries(slayawaycamp PRIVATE Qt5::Core)

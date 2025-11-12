
cmake_minimum_required(VERSION 3.20)

project(slayawaycamp)

set(CMAKE_CXX_STANDARD 20)

find_package(Qt5 REQUIRED Core)

add_executable(slayawaycamp
	main.cpp
	App.cpp
	Map.cpp
)

target_compile_definitions(slayawaycamp PRIVATE
	SLAYAWAYCAMP_LEVELS_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/levels\"
)

target_link_libraries(slayawaycamp PRIVATE Qt5::Core)

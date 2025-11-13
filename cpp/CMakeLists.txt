
cmake_minimum_required(VERSION 3.20)

project(slayawaycamp)

set(CMAKE_CXX_STANDARD 20)

get_filename_component(root_dir "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

find_package(Qt5 REQUIRED Core)

add_executable(slayawaycamp
	main.cpp
	App.cpp
	Map.cpp
	Moobaa.cpp
	Player.cpp
	Solver.cpp
	State.cpp
)

target_compile_definitions(slayawaycamp PRIVATE
	SLAYAWAYCAMP_LEVELS_DIR=\"${root_dir}/levels\"
	SLAYAWAYCAMP_REFERENCE_DIR=\"${root_dir}/reference\"
)

target_link_libraries(slayawaycamp PRIVATE Qt5::Core)

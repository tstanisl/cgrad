cmake_minimum_required (VERSION 2.8.11)
project (cgrad)
set(CMAKE_C_STANDARD 99)

add_library(cgrad STATIC
        src/matrix.c
	# add other cpp files if required
)

target_include_directories(cgrad PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(cgrad PUBLIC)

add_subdirectory(tests)


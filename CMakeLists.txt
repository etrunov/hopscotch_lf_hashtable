cmake_minimum_required(VERSION 3.10)
project(hopscotch_ht C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Set include directories (modern approach)
set(INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/tests
)

# Add the executable with proper source files
add_executable(hopscotch_ht_app
	src/hopscotch_ht.c
	tests/hopscotch_ht_test_misc.c
	tests/threads_test.c
	tests/basic_tests.c
	hopscotch_ht_main.c
)

# Modern way to handle includes (per-target)
target_include_directories(hopscotch_ht_app PRIVATE
	${INCLUDE_DIRS}
)

# Set compiler options
target_compile_options(hopscotch_ht_app PRIVATE
	-Wall
	-Wextra
	-Wno-unused-function
)

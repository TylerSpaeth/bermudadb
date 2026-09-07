cmake_minimum_required(VERSION 3.25)

project(BermudaDB
				VERSION  0.1.0
				LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(bermudadb 
	src/main.cpp
		src/common/logger.cpp
)

#Core library
add_library(bermudadb_core
		STATIC
		src/common/logger.cpp
)

target_include_directories(bermudadb_core
		PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(bermudadb PRIVATE bermudadb_core)

# Testing
include(CTest)
enable_testing()
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
add_executable(bermudadb_tests
	tests/main.cpp
)
target_link_libraries(bermudadb_tests PRIVATE GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(bermudadb_tests)

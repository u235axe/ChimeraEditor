cmake_minimum_required(VERSION 3.5)

project(ChimeraEditor LANGUAGES CXX)

add_executable(Chimera WIN32 main.cpp)

set_target_properties(Chimera PROPERTIES CXX_STANDARD 14
	                                     CXX_STANDARD_REQUIRED ON)

target_compile_definitions(Chimera PRIVATE _UNICODE UNICODE)

file(COPY clang_MSVC15.txt
          compilers.txt
          g++.txt
          msvc15.txt
          style.txt
		  test.cpp
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
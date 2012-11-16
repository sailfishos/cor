PROJECT(cor)
cmake_minimum_required(VERSION 2.6)

find_package(PkgConfig)

set(CMAKE_CXX_FLAGS 
  "${CMAKE_CXX_FLAGS} -Wall -O2 -feliminate-unused-debug-types -std=c++0x"
  )

# fix for gcc 4.6 specific compiling/linking issues, no dwarf-4
set(CMAKE_CXX_FLAGS 
  "${CMAKE_CXX_FLAGS} -gdwarf-3"
  )


INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_subdirectory(src)
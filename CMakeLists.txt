PROJECT(cor)
cmake_minimum_required(VERSION 2.8)

find_package(PkgConfig)
find_package (Threads)

IF(NOT DEFINED VERSION)
MESSAGE(FATAL_ERROR "Define VERSION")
ENDIF(NOT DEFINED VERSION)

option(ENABLE_MULTIARCH "Enable support for multi-arch distros (lib64)" OFF)
IF(ENABLE_MULTIARCH)
  IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
    SET(LIB_SUFFIX "")
  ELSE(CMAKE_SIZEOF_VOID_P EQUAL 4)
    SET(LIB_SUFFIX 64)
  ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
ELSE()
  SET(LIB_SUFFIX "")
ENDIF()

option(ENABLE_UDEV "Enable libudev wrapper" ON)

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

set(prefix ${CMAKE_INSTALL_PREFIX})
set(DST_LIB lib${LIB_SUFFIX})

configure_file(cor.pc.in ${CMAKE_BINARY_DIR}/cor.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/cor.pc DESTINATION lib${LIB_SUFFIX}/pkgconfig)

if(ENABLE_UDEV)
pkg_check_modules(UDEV libudev REQUIRED)
configure_file(cor-udev.pc.in ${CMAKE_BINARY_DIR}/cor-udev.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/cor-udev.pc DESTINATION lib${LIB_SUFFIX}/pkgconfig)
endif()


if(ENABLE_UDEV)
install(
  DIRECTORY include/cor
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.hpp"
  PATTERN "*.h"
)
else()
message(STATUS "NO UDEV")
install(
  DIRECTORY include/cor
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.hpp"
  PATTERN "*.h"
  PATTERN "udev*" EXCLUDE
  PATTERN "udev/*" EXCLUDE
)
endif()

add_subdirectory(src)
add_subdirectory(tests)
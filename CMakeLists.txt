PROJECT(cor)
cmake_minimum_required(VERSION 2.8)

find_package(PkgConfig)
find_package (Threads)

IF(NOT DEFINED VERSION)
MESSAGE(FATAL_ERROR "Define VERSION")
ENDIF(NOT DEFINED VERSION)

set(prefix ${CMAKE_INSTALL_PREFIX})

include(${CMAKE_SOURCE_DIR}/cmake/Cor.cmake)
add_subdirectory(cmake)

option(ENABLE_UDEV "Enable libudev wrapper" ON)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

configure_install_pkgconfig(cor)

if(ENABLE_UDEV)
  pkg_check_modules(UDEV libudev REQUIRED)
  configure_install_pkgconfig(cor-udev)
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
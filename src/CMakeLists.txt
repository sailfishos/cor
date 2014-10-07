add_library(cor SHARED
  notlisp.cpp mt.cpp sexp.cpp util.cpp error.cpp
  )

set_target_properties(cor PROPERTIES
  SOVERSION 0
  VERSION ${VERSION}
  )

target_link_libraries(cor
  ${CMAKE_THREAD_LIBS_INIT}
  -ldl
)

install(TARGETS cor DESTINATION lib${LIB_SUFFIX})

if(ENABLE_UDEV)
include_directories(
  ${UDEV_INCLUDE_DIRS}
)

link_directories(
  ${UDEV_LIBRARY_DIRS}
)

add_library(cor-udev SHARED
  udev.cpp
  udev-util.cpp
  )

set_target_properties(cor-udev PROPERTIES
  SOVERSION 0
  VERSION ${VERSION}
  )

target_link_libraries(cor-udev
  ${UDEV_LIBRARIES}
  ${COR_LIBRARIES}
  )

install(TARGETS cor-udev DESTINATION ${DST_LIB})
endif()

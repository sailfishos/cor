add_library(cor SHARED notlisp.cpp mt.cpp)
target_link_libraries(cor
  ${CMAKE_THREAD_LIBS_INIT}
  -ldl
)
install(TARGETS cor DESTINATION lib${LIB_SUFFIX})
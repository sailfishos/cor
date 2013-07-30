add_library(cor SHARED notlisp.cpp mt.cpp)
target_link_libraries(cor
  ${CMAKE_THREAD_LIBS_INIT}
)
install(TARGETS cor DESTINATION lib${LIB_SUFFIX})
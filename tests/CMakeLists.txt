find_package(Threads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")

add_executable(test_cor
  main.cpp options.cpp util.cpp sexp.cpp notlisp.cpp mt.cpp error.cpp
)

target_link_libraries(test_cor
  cor
  ${CMAKE_THREAD_LIBS_INIT}
)

add_test(test "./test_cor")

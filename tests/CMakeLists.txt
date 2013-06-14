find_package(Threads)

add_executable(test_cor
  main.cpp options.cpp util.cpp sexp.cpp notlisp.cpp mt.cpp
)

target_link_libraries(test_cor
  cor
  ${CMAKE_THREAD_LIBS_INIT}
)

add_test(test "./test_cor")

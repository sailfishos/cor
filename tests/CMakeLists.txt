add_executable(test_cor main.cpp options.cpp util.cpp sexp.cpp notlisp.cpp)
target_link_libraries(test_cor
  cor
)
add_test(test "./test_cor")

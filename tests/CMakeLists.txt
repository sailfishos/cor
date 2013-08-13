find_package(Threads)
pkg_check_modules(TUT tut REQUIRED)

INCLUDE_DIRECTORIES(${TUT_INCLUDES})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")

set(COR_TESTS options util sexp notlisp mt error)

MACRO(COR_TEST _name)
  set(_exe_name test_${_name})
  add_executable(${_exe_name} EXCLUDE_FROM_ALL main.cpp ${_name}.cpp)
  target_link_libraries(${_exe_name} cor ${CMAKE_THREAD_LIBS_INIT})
  add_test(${_exe_name} ${_exe_name})
  add_dependencies(check ${_exe_name})
ENDMACRO(COR_TEST)

foreach(t ${COR_TESTS})
  COR_TEST(${t})
endforeach(t)

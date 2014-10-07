find_package(Threads)
pkg_check_modules(TUT tut REQUIRED)

INCLUDE_DIRECTORIES(${TUT_INCLUDES})

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})
enable_testing()

set(COR_TESTS error options util sexp notlisp mt os)

# It counts stack trace depth, so optimization should not be done
set_source_files_properties(error.cpp PROPERTIES COMPILE_FLAGS -O0)

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

if(ENABLE_UDEV)
  COR_TEST(udev)
  target_link_libraries(test_udev cor-udev)
endif()

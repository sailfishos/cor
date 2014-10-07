find_package(Threads)
pkg_check_modules(TUT tut REQUIRED)

INCLUDE_DIRECTORIES(${TUT_INCLUDES})

set(COR_TESTS error options util sexp notlisp mt)
set(TESTS_DIR /opt/tests/cor)

# It counts stack trace depth, so optimization should not be done
set_source_files_properties(error.cpp PROPERTIES COMPILE_FLAGS -O0)

MACRO(COR_TEST _name)
  set(_exe_name test_${_name})
  add_executable(${_exe_name} main.cpp ${_name}.cpp)
  target_link_libraries(${_exe_name} cor ${CMAKE_THREAD_LIBS_INIT})
  install(TARGETS ${_exe_name} DESTINATION ${TESTS_DIR})
ENDMACRO(COR_TEST)

foreach(t ${COR_TESTS})
  COR_TEST(${t})
endforeach(t)

if(ENABLE_UDEV)
  COR_TEST(udev)
  target_link_libraries(test_udev cor-udev)
endif()

configure_file(tests.xml.in tests.xml @ONLY)
install(FILES tests.xml DESTINATION ${TESTS_DIR})

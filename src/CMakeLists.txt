add_library(cor SHARED notlisp.cpp mt.cpp)
install(TARGETS cor DESTINATION lib${LIB_SUFFIX})
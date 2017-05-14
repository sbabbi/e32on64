
add_library(e32loader OBJECT loader.cpp parser.cpp)
set_target_properties(e32loader PROPERTIES POSITION_INDEPENDENT_CODE ON)

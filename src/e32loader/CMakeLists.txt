
add_library(e32loader STATIC loader.cpp parser.cpp)
set_target_properties(e32loader PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(e32loader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


add_library(e32libc STATIC e32_enter.c e32_libc.c)
target_include_directories(e32libc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

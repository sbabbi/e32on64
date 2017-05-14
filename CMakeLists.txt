
cmake_minimum_required(VERSION 3.6)

add_subdirectory(e32libc)
add_subdirectory(e32loader)

add_library(e32on64 SHARED $<TARGET_OBJECTS:e32loader> )
target_include_directories(e32on64 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/e32loader)

enable_testing()
add_subdirectory(test)
# add_library(e32on64 elf_loader.cpp)

# add_executable(dump dump.cpp e32_libc.c e32_enter.c)
# target_link_libraries(dump e32on64)

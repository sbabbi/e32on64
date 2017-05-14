find_package(Boost REQUIRED COMPONENTS unit_test_framework)

add_subdirectory(32bit)

add_executable(libc_stdlib libc_stdlib.cpp)
target_link_libraries(libc_stdlib PRIVATE e32libc Boost::unit_test_framework)
add_test( NAME libc_stdlib COMMAND libc_stdlib )

add_executable(loader loader.cpp)
target_link_libraries(loader PRIVATE e32on64 e32libc Boost::unit_test_framework)
add_test( NAME loader COMMAND loader )

find_package(Boost REQUIRED COMPONENTS unit_test_framework)

add_subdirectory(32bit)

add_executable(e32libc_stdlib_test e32libc_stdlib.cpp)
target_link_libraries(e32libc_stdlib_test PRIVATE e32libc Boost::unit_test_framework)
add_test( NAME e32libc_stdlib COMMAND e32libc_stdlib_test )

add_executable(e32loader_test e32loader_test.cpp)
target_link_libraries(e32loader_test PRIVATE e32loader e32libc Boost::unit_test_framework)
add_test( NAME e32loader COMMAND e32loader_test )

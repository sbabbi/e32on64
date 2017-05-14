
add_library( base1 MODULE base1.c )
target_compile_options( base1 PRIVATE "-m32" )
set_target_properties( base1 PROPERTIES LINK_FLAGS "-m32" POSITION_INDEPENDENT_CODE OFF)

add_library( base1_pic MODULE base1.c )
target_compile_options( base1_pic PRIVATE "-m32" )
set_target_properties( base1_pic PROPERTIES LINK_FLAGS "-m32")

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE elf_loader
#include <boost/test/unit_test.hpp>

#include <e32_libc.h>
#include <loader.h>

int call( e32_function_ptr method, int arg )
{
    struct result_t
    {
        e32_function_ptr method;
        int res;
        int arg;
    } result = { method, 0, arg };
    
    e32_stack_jump( 1024 * 1024,
                    +[]( void * data )
                    {
                        result_t * ans = reinterpret_cast<result_t*>(data);
                        ans->res = e32_enter32_i( ans->method, ans->arg );
                    },
                    &result );

    return result.res;
}

uint32_t get_symlibc( std::experimental::string_view name )
{
    if ( name == "abort" )
    {
        return e32_abort;
    }

    if ( name == "abs" )
    {
        return e32_abs;
    }
    if ( name == "atoi" )
    {
        return e32_atoi;
    }
    
    return 0;
}

BOOST_AUTO_TEST_CASE(test_base1)
{
    elf::loader loader("32bit/libbase1.so", get_symlibc);

    BOOST_TEST( call( loader.get_sym("foo"), 10 ) == 45 );
    BOOST_TEST( call( loader.get_sym("foo"), -10 ) == 0 );
    
    BOOST_TEST( call( loader.get_sym("foo_abs"), 10 ) == 45 );
    BOOST_TEST( call( loader.get_sym("foo_abs"), -10 ) == 45 );
    
    BOOST_TEST( call( loader.get_sym("foo_atoi"), 10 ) == 120 );
    BOOST_TEST( call( loader.get_sym("foo_atoi"), -10 ) == -120 );
}

BOOST_AUTO_TEST_CASE(test_base1_pic)
{
    elf::loader loader("32bit/libbase1_pic.so", get_symlibc);

    BOOST_TEST( call( loader.get_sym("foo"), 10 ) == 45 );
    BOOST_TEST( call( loader.get_sym("foo"), -10 ) == 0 );
    
    BOOST_TEST( call( loader.get_sym("foo_abs"), 10 ) == 45 );
    BOOST_TEST( call( loader.get_sym("foo_abs"), -10 ) == 45 );
    
    BOOST_TEST( call( loader.get_sym("foo_atoi"), 10 ) == 120 );
    BOOST_TEST( call( loader.get_sym("foo_atoi"), -10 ) == -120 );
}

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE libc_stdlib
#include <boost/test/unit_test.hpp>

#include <e32_libc.h>

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

BOOST_AUTO_TEST_CASE(test_abs)
{
    BOOST_TEST( call( e32_abs, -20 ) == 20 );
    BOOST_TEST( call( e32_abs, -2147483647 ) == 2147483647 );
}

BOOST_AUTO_TEST_CASE(test_atoi)
{
    int res;
    e32_stack_jump( 1024 * 1024,
                    +[]( void * data )
                    {
                        const volatile char q[] = "4125";
                        *reinterpret_cast<int*>(data) = 
                            e32_enter32_i( e32_atoi, (int)(int64_t)(char*)q );
                    },
                    &res );
    BOOST_TEST( res == 4125 );

    e32_stack_jump( 1024 * 1024,
                    +[]( void * data )
                    {
                        const volatile char q[] = "-5121";
                        *reinterpret_cast<int*>(data) = 
                            e32_enter32_i( e32_atoi, (int)(int64_t)(char*)q );
                    },
                    &res );
    BOOST_TEST( res == -5121 );

}


#include <stdlib.h>

int foo( int c )
{
    int ans = 0;
    for ( int i = 0; i < c; ++i )
        ans += i;
    return ans;
}

int foo_abs( int c )
{
    return foo(abs(c));
}

int foo_atoi( int c )
{
    const char q[] = "12";

    return c * atoi(q);
}

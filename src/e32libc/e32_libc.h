
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

typedef uint32_t e32_function_ptr;

/**
 * @brief Jump on a stack that lives on a 32-bit segment.
 * @param stack_size Size of the new stack
 * @param f Function that will be called with the new stack
 * @param param Argument for \ref f.
 * @return Zero on success, negative value on failure.
 */
int e32_stack_jump( size_t stack_size, void (*f)(void*), void * param );

int e32_enter32_i( e32_function_ptr method, int arg0);

extern e32_function_ptr e32_abort;
extern e32_function_ptr e32_abs;
extern e32_function_ptr e32_atoi;

#ifdef __cplusplus
}
#endif //__cplusplus

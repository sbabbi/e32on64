
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>

#include "e32_libc.h"

// Entry points
e32_function_ptr e32_abort;
e32_function_ptr e32_abs;
e32_function_ptr e32_atoi;

// Base address of the library
static void * e32_libc_text_base;

#define S_E32_LIBC_FUNC_ALIGN   0x20
#define S_E32_LIBC_SECTION_SIZE 8096

/**
 * @brief Pad addr with nops, until the next S_E32_LIBC_FUNC_ALIGN bytes boundary.
 */
static void * s_e32_nop_pad( void * addr )
{
    char * p = addr;
    while ( (uint64_t)(p) % S_E32_LIBC_FUNC_ALIGN !=  0 )
    {
        *p++ = 0x90;
    }
    
    return p;
}

/**
 * @brief Align to the next S_E32_LIBC_FUNC_ALIGN bytes.
 */
size_t s_e32_align_size( size_t value )
{
    return value + ( S_E32_LIBC_FUNC_ALIGN - value % S_E32_LIBC_FUNC_ALIGN ) % S_E32_LIBC_FUNC_ALIGN;
}

/**
 * @brief Generates a wrapper for a libc call
 * @param addr Destination for the bytecode
 * @param size Size of the buffer at \ref addr.
 * @param target Target function
 * @param prologue 64-bit call prologue bytecode
 * @param prologue_size Size of prologue
 * @param epilogue 64-bit call epilogue bytecode
 * @param epilogue_size Size of epilogue
 * @return Next available address after \ref addr, or NULL on failure.
 */
static e32_function_ptr s_e32_make_libc_wrapper( e32_function_ptr addr,
                                                 size_t size,
                                                 void * target,
                                                 const void * prologue, size_t prologue_size,
                                                 const void * epilogue, size_t epilogue_size )
{
    static const char enter_64[] =
    {
        0x9a, 0x0, 0x0, 0x0, 0x0, 0x33, 0x0, // lcall $0x33,$trampoline
        0xc3 // ret
    };
    
    static const char call_target[] = 
    {
        0xe8, 0x0, 0x0, 0x0, 0x0, // callq 0x0
    };
    
    static const char exit_64[] =
    {   
        0xcb, // long ret
    };
    
    const size_t total_size = sizeof(enter_64) + prologue_size + sizeof(call_target) + epilogue_size + sizeof(exit_64);
    
    if ( size < s_e32_align_size(total_size) )
    {
        return (e32_function_ptr)0;
    }
    
    char * ptr = (char*)(uint64_t)addr;

    memcpy( ptr, enter_64, sizeof(enter_64) );
    ptr += sizeof(enter_64);
    
    if ( prologue_size > 0 )
    {
        memcpy( ptr, prologue, prologue_size );   
        ptr += prologue_size;
    }
    
    memcpy( ptr, call_target, sizeof(call_target) );
    ptr += sizeof(call_target);
    
    if ( epilogue_size > 0 )
    {
        memcpy( ptr, epilogue, epilogue_size );   
        ptr += epilogue_size;
    }
    
    memcpy( ptr, exit_64, sizeof(exit_64) );
    ptr += sizeof(exit_64);
    
    // Relocation base address
    const uint32_t base_addr_i = (uint32_t)(uint64_t)(addr);
    
    // Relocate (lcall trampoline)
    const uint32_t trampoline_addr = base_addr_i + 8;
    memcpy( (char*)(uint64_t)addr + 1,
            &trampoline_addr,
            sizeof(trampoline_addr) );
    
    const uint32_t call_target_offset = sizeof(enter_64) + prologue_size;
    
    // Relocate (callq target)
    const uint32_t target_rel_offset = 
        (uint32_t)(uint64_t)(target) - 
        (base_addr_i + call_target_offset + sizeof(call_target) );

    memcpy( (char*)(uint64_t)addr + call_target_offset + 1,
            &target_rel_offset,
            sizeof(target_rel_offset) );
    
    return (uint64_t)s_e32_nop_pad(ptr);
}

/**
 * @brief Copy opcodes for the abort() wrapper
 */
static e32_function_ptr s_e32_abort( e32_function_ptr addr, size_t size )
{
    return s_e32_make_libc_wrapper( addr, size,
                                    &abort, 
                                    NULL, 0, 
                                    NULL, 0 );
}
/**
 * @brief Copy opcodes for the abs() wrapper
 */
static e32_function_ptr s_e32_abs( e32_function_ptr addr, size_t size )
{
    static const char prologue[] =
    {
        0x8b, 0x7c, 0x24, 0x0C, // mov 12(%esp), %edi
    };
    
    return s_e32_make_libc_wrapper( addr, size,
                                    &abs, 
                                    prologue, sizeof(prologue),
                                    NULL, 0 );
}
/**
 * @brief Copy opcodes for the atoi() wrapper
 */
static e32_function_ptr s_e32_atoi( e32_function_ptr addr, size_t size )
{
    static const char prologue[] =
    {
        0x8b, 0x7c, 0x24, 0x0C, // mov 12(%esp), %edi
    };
    
    return s_e32_make_libc_wrapper( addr, size,
                                    &atoi, 
                                    prologue, sizeof(prologue),
                                    NULL, 0 );
}

/**
 * @brief Initialize all the entry points
 */
__attribute__((constructor)) static void e32_libc_init() 
{
    e32_libc_text_base = mmap( NULL, S_E32_LIBC_SECTION_SIZE ,
                               PROT_EXEC | PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_32BIT | MAP_ANONYMOUS,
                               -1, 0 );
    
    if ( ! e32_libc_text_base || (uint64_t)(e32_libc_text_base) % S_E32_LIBC_FUNC_ALIGN != 0) 
    {
        return;
    }
    
    if ( (uint64_t)e32_libc_text_base + S_E32_LIBC_SECTION_SIZE > 0xFFFFFFFF )
    {
        munmap( e32_libc_text_base, S_E32_LIBC_SECTION_SIZE  );
        e32_libc_text_base = NULL;
        return;
    }
    
    e32_function_ptr ptr = (e32_function_ptr)(uint64_t)e32_libc_text_base;
    const e32_function_ptr end = ptr + S_E32_LIBC_SECTION_SIZE ;

    ptr = s_e32_abort( e32_abort = ptr, end - ptr );
    ptr = s_e32_abs( e32_abs = ptr, end - ptr );
    ptr = s_e32_atoi( e32_atoi = ptr, end - ptr );
    
    // Revoke PROT_WRITE
    mprotect(e32_libc_text_base, S_E32_LIBC_SECTION_SIZE, PROT_EXEC | PROT_READ );
}

/**
 * @brief Release allocated memory
 */
__attribute__((destructor)) static void e32_libc_deinit()
{
    munmap( e32_libc_text_base, S_E32_LIBC_SECTION_SIZE  );
}

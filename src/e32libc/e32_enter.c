#include <stddef.h>

#include <sys/mman.h>
#include <sys/personality.h>

#include "e32_libc.h"

int e32_stack_jump( size_t stack_size, void (*f)(void*), void * param )
{
    void * stack = mmap(NULL, 
                        stack_size,
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_32BIT | MAP_ANONYMOUS,
                        -1, 0 );

    if ( !stack )
    {
        return -1;
    }
    
    void * stack_base = (char*)stack + stack_size;

    asm ( 
        "mov %%rsp, %%rdx\n\t" //save old RSP
        "mov %2, %%rsp\n\t" // Replace stack
          
        "push %%rdx\n\t"
        "push %%rbp\n\t" // Save some stuff
        
        "mov %1, %%rdi\n\t" // Load param
        "call *%0\n\t" // Call f(param)

        "pop %%rbp\n\t"
        "pop %%rsp\n\t"
        :
        :
        "r"(f), "r"(param), "r"(stack_base) :
        "rdx", "rsp", "rdi"  );

    munmap(stack, stack_size);
    
    return 0;
}

int e32_enter32_i(e32_function_ptr method, int arg0)
{        
    struct __attribute__((packed, aligned(16))) {
        uint32_t address;
        int16_t segment;
    } target = {method, 0x23};

//     const unsigned long prev_pers = personality(PER_LINUX32);
    int ret;

    asm (       
          "movl (%1), %%edi\n\t"            //Save address in EDI  
          "movl $trampoline%=, (%1)\n\t"      // Replace address in "target" 
          "mov %2, %%esi\n\t"               // Mov arg0 into esi.
          "lcall *(%1)\n\t"                 // Call the trampoline.
          "jmp exit%=\n\t"                    // On return jump to the end
          
          "trampoline%=:\n\t"         // trampoline
          
          ".byte 0x56\n\t" // push esi
          ".byte 0x56\n\t" // push esi
          ".byte 0x56\n\t" // push esi
          ".byte 0x56\n\t" // push esi
          
          "callq *%%rdi\n\t"
          ".byte 0x83, 0xc4, 0x10\n\t" // add $0x10, %esp
          "lret\n\t"
          
          "exit%=:\n\t"
        : 
        "=a"(ret)
        :
        "r"(&target), "r"(arg0)
        :
        "rsi", "rdi", "memory" );
    
//     personality(prev_pers);
    return ret;
}

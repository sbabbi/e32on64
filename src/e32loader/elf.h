
#ifndef E32LOADER_ELF_H
#define E32LOADER_ELF_H

#include <cstdint>

namespace elf
{

using half_t    = uint16_t;
using offset_t  = uint32_t;
using address_t = uint32_t;
using word_t    = uint32_t;
using sword_t   = int32_t;

/**
 * @brief Elf @c e_type.
 */
enum class et : half_t
{
    none    = 0, ///< No file type
    rel     = 1, ///< Relocatable file
    exec    = 2, ///< Executable file
    dyn     = 3, ///< Shared object file
    core    = 4, ///< Core file
    loproc  = 0xff00, ///< Processor-specific
    hiproc  = 0xffff ///<  Processor-specific
};

/**
 * @brief Elf @c e_machine
 */
enum class em : half_t
{
    none    = 0, // No machine
    m32     = 1, // 1 AT&T WE 32100
    sparc   = 2, // SPARC
    _386    = 3, // EM_386 3 Intel 80386
    _68K    = 4, // EM_68K 4 Motorola 68000
    _88K    = 5, // EM_88K 5 Motorola 88000
    _860K   = 7, // EM_860 7 Intel 80860
    mips    = 8 // EM_MIPS 8 MIPS RS3000
};

/**
 * @brief Elf @c p_type
 */
enum class pt : word_t
{
    null        = 0x0,
    load        = 0x1,
    dynamic     = 0x2,
    interp      = 0x3,
    note        = 0x4,
    shlib       = 0x5,
    phdr        = 0x6,
    loproc      = 0x70000000,
    hiproc      = 0x7fffffff
};

/**
 * @brief Elf @c p_flags
 */
enum class pf : word_t
{
    x           = 0x1,
    w           = 0x2,
    r           = 0x4
};

constexpr bool exec( pf f )     { return word_t(f) & word_t(pf::x); }
constexpr bool write( pf f )    { return word_t(f) & word_t(pf::w); }
constexpr bool read( pf f )     { return word_t(f) & word_t(pf::r); }


/**
 * @brief Elf @c sh_type. Section type.
 */
enum class sht : word_t
{
    null = 0,
    progbits = 1,
    symtab = 2,
    strtab = 3,
    rela = 4,
    hash = 5,
    dynamic = 6,
    note = 7,
    nobits = 8,
    rel = 9,
    shlib = 10,
    dynsym = 11,
    loproc = 0x70000000,
    hiproc = 0x7fffffff,
    louser = 0x80000000,
    hiuser = 0xffffffff
};

/**
 * @brief Elf relocation types
 */
enum class r_386
{
    none        = 0,
    _32         = 1, //!< S + A
    pc32        = 2, //!< S + A - P
    got32       = 3, //!< G + A - P
    plt32       = 4, //!< L + A - P
    copy        = 5, //!< None
    glob_dat    = 6, //!< S
    jmp_slot    = 7, //!< S
    relative    = 8, //!< B + A
    gotoff      = 9, //!< S + A - GOT 
    gotpc       = 10 //!< GOT + A - P
};

/**
 * @brief Elf header
 */
struct header 
{
    enum { EI_NIDENT =  16 };

    uint8_t     e_ident[EI_NIDENT];
    et          e_type;
    em          e_machine;
    word_t      e_version;
    address_t   e_entry;
    offset_t    e_phoff;
    offset_t    e_shoff;
    word_t      e_flags;
    half_t      e_ehsize;
    half_t      e_phentsize;
    half_t      e_phnum;
    half_t      e_shentsize;
    half_t      e_shnum;
    half_t      e_shstrndx;
};

/**
 * @brief Elf program header (Elf32_phdr)
 */
struct program_header
{
    pt          p_type;
    offset_t    p_offset;
    address_t   p_vaddr;
    address_t   p_paddr;
    word_t      p_filesz;
    word_t      p_memsz;
    pf          p_flags;
    word_t      p_align;
};

/**
 * @brief Elf section header
 */
struct section_header
{
    word_t      sh_name;
    sht         sh_type;
    word_t      sh_flags;
    address_t   sh_addr;
    offset_t    sh_offset;
    word_t      sh_size;
    word_t      sh_link;
    word_t      sh_info;
    word_t      sh_addralign;
    word_t      sh_entsize;
};

/**
 * @brief A single entry in the symbol table
 */
struct symbol_table_entry
{
    word_t          st_name;
    address_t       st_value;
    word_t          st_size;
    unsigned char   st_info;
    unsigned char   st_other;
    half_t          st_shndx;
};

/**
 * @brief Elf32_rel
 */
struct relocation
{
    address_t       r_offset;
    word_t          r_info;
    
    r_386 type() const
    {
        return r_386(r_info & 0xFF);
    }
    
    half_t sym() const
    {
        return r_info >> 8;
    }
};

/**
 * @brief Elf32_rela
 */
struct relocation_a
{
    address_t       r_offset;
    word_t          r_info;
    sword_t         r_addend;
    
    r_386 type() const
    {
        return r_386(r_info & 0xFF);
    }
    
    half_t sym() const
    {
        return r_info >> 8;
    }
};

} //namespace elf

#endif //E32LOADER_ELF_H


#ifndef E32ON64_ELF_LOADER_H
#define E32ON64_ELF_LOADER_H

#include <cstdint>
#include <iosfwd>
#include <vector>
#include <experimental/string_view>
#include <boost/range/iterator_range.hpp>

namespace elf
{

using half_t    = uint16_t;
using offset_t  = uint32_t;
using address_t = uint32_t;
using word_t    = uint32_t;
using sword_t   = int32_t;

enum class et : half_t
{
    none    = 0, // No file type
    rel     = 1, // Relocatable file
    exec    = 2, // Executable file
    dyn     = 3, // Shared object file
    core    = 4, // Core file
    loproc  = 0xff00, //Processor-specific
    hiproc  = 0xffff // Processor-specif
};

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

struct symbol_table_entry
{
    word_t          st_name;
    address_t       st_value;
    word_t          st_size;
    unsigned char   st_info;
    unsigned char   st_other;
    half_t          st_shndx;
};

enum class r_386
{
    none        = 0,
    _32         = 1,
    pc32        = 2,
    got32       = 3,
    plt32       = 4,
    copy        = 5,
    glob_dat    = 6,
    jmp_slot    = 7,
    relative    = 8,
    gotoff      = 9,
    gotpc       = 10
};

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

class loader
{
public:
    explicit loader( std::vector< char > file );

    const auto & header() const
    {
        return *access<struct header>( 0 );
    }
    
    auto sections() const
    {
        return boost::make_iterator_range(
            access< section_header >( header().e_shoff ),
            access< section_header >( header().e_shoff ) + header().e_shnum );
    }
    
    const section_header & shstrtab() const
    {
        return *( access<section_header>( header().e_shoff ) + header().e_shstrndx );
    }
    
    const section_header & symtab() const
    {
        return * std::find_if( sections().begin(),
                               sections().end(),
                               [](const section_header & sh)
                               {
                                   return sh.sh_type == sht::symtab;
                               } );
    }

    const section_header & dynsym() const
    {
        return * std::find_if( sections().begin(),
                               sections().end(),
                               [](const section_header & sh)
                               {
                                   return sh.sh_type == sht::dynsym;
                               } );
    }

    
    auto symbols(const section_header & st) const
    {
        assert(st.sh_type == sht::symtab || st.sh_type == sht::dynsym );
        
        return boost::make_iterator_range(
            access< symbol_table_entry >( st.sh_offset ),
            access< symbol_table_entry >( st.sh_offset + st.sh_size ) );
    }

    
    auto symbols() const
    {
        return symbols( symtab() );
    }
       
    std::experimental::string_view section_name( const section_header & sec )
    {
        return access<char>( shstrtab().sh_offset + sec.sh_name ) ;
    }
    
    std::experimental::string_view symbol_name( const symbol_table_entry & sym, const section_header & sec )
    {
        return sym.st_name == 0 ? 
                std::experimental::string_view() :
                std::experimental::string_view( access<char>(sections()[ sec.sh_link ].sh_offset + sym.st_name ) );
    }
    
    auto relocations( const section_header & sh ) const
    {
        assert( sh.sh_type == sht::rel );
        return boost::make_iterator_range(
            access< relocation >( sh.sh_offset ),
            access< relocation >( sh.sh_offset + sh.sh_size ) );
    }
    
    auto relocations_a( const section_header & sh ) const
    {
        assert( sh.sh_type == sht::rela );
        return boost::make_iterator_range(
            access< relocation_a >( sh.sh_offset ),
            access< relocation_a >( sh.sh_offset + sh.sh_size ) );
    }
    
    std::experimental::string_view section_data( const section_header & sh ) const 
    {
        return std::experimental::string_view( access<char>(sh.sh_offset), sh.sh_size );
    }
    
private:
    template<class T>
    const T * access(offset_t off) const
    {
        return reinterpret_cast<const T*>( file_.data() + off );
    }
    
    std::vector< char > file_;
};

std::ostream& operator<<(std::ostream &, et);
std::ostream& operator<<(std::ostream &, em);
std::ostream& operator<<(std::ostream &, sht);
std::ostream& operator<<(std::ostream &, r_386);

} //namespace elf

#endif //E32ON64_ELF_LOADER_H

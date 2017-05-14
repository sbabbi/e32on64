
#include "loader.h"

#include <cstring>
#include <fstream>
#include <vector>
#include <iostream>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/max_element.hpp>

#include <sys/mman.h>

#include "parser.h"

namespace elf
{
    
namespace
{

/**
 * @brief Parses the program headers and load the image to memory.
 */
mmap_region load_elf32( const parser & p )
{
    using boost::max_element;
    using boost::adaptors::transformed;
    
    auto phs = p.program_headers();
    assert( !phs.empty() );
    
    auto get_program_header_size = [](const program_header & ph ) { return ph.p_vaddr + ph.p_memsz; };
    
    const std::size_t total_size = 
        *max_element( phs | transformed( std::cref(get_program_header_size) ) );
        
    const std::size_t pagesize = getpagesize();
    
    const std::size_t allocated_size = total_size + ( pagesize - total_size % pagesize ) % pagesize;
    
    mmap_region result ( mmap( NULL, allocated_size, 
                               PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_32BIT | MAP_ANONYMOUS,
                               -1, 0 ),
                         allocated_size );
    
    // Start loading stuff
    for ( const program_header & ph : phs )
    {
        std::memcpy( result.at( ph.p_vaddr ),
                     p.raw_block( ph.p_offset, ph.p_filesz ).data(),
                     ph.p_filesz );
    }
    
    return result;
}

/**
 * @brief Applies protection flags to all the loaded sections
 */
void apply_prot_flags( const parser & p, const mmap_region & vs )
{
    const std::size_t pagesize = getpagesize();
    assert( vs.size() % pagesize == 0 );
    assert( uint64_t(vs.data()) % pagesize == 0 );
    
    const std::size_t pages = vs.size() / pagesize;
    
    std::vector< int > pageflags(pages);

    for ( const program_header & ph : p.program_headers() )
    {
        const std::size_t end_vaddr = ph.p_vaddr + ph.p_memsz;

        const std::size_t page_start = ph.p_vaddr - (ph.p_vaddr % pagesize);
        const std::size_t page_end   = end_vaddr + ( pagesize - end_vaddr % pagesize ) % pagesize;
        
        const int p1 = page_start / pagesize;
        const int p2 = page_end / pagesize;
        assert( p2 <= pageflags.size() );
        
        for ( int i = p1; i < p2; ++i )
        {
            pageflags[i] |= ( read(ph.p_flags) ? PROT_READ : 0 ) |
                            ( write(ph.p_flags) ? PROT_WRITE : 0 ) | 
                            ( exec(ph.p_flags) ? PROT_EXEC : 0 );
        }
    }
    
    for ( std::size_t i = 0; i < pageflags.size(); ++i )
    {
        void * dest = vs.at( i * pagesize );
        
        if (0 != mprotect( dest, pagesize, pageflags[i] ) )
        {
            throw std::runtime_error("load_elf32::protect");
        }
    }
}

inline uint32_t read_uint32_t( char * base, uint32_t offset )
{
    uint32_t ans;
    std::memcpy(&ans, base + offset, sizeof(uint32_t) );
    return ans;
}

inline void write_uint32_t( char * base, uint32_t offset, uint32_t value)
{
    std::memcpy( base + offset, &value, sizeof(uint32_t) );
}

/**
 * @brief Applies the relocation in the given DSO
 */
void relocate_elf32( const parser & p, 
                     const section_header & reloc_sec,
                     mmap_region const & vs,
                     std::function<uint32_t(std::experimental::string_view)> const & get_sym )
{
    assert( reloc_sec.sh_type == sht::rel );
    
    const section_header & symtab = p.section_headers()[ reloc_sec.sh_link ];
    
    auto get_symbol_name = 
        [ symbol_names = p.string_table( symtab.sh_link )](const symbol_table_entry & sym)
        { 
            return sym.st_name == 0 ? string_view() : symbol_names.get_string( sym.st_name );
        };

    auto symbols = p.symbols(symtab);

    for ( const relocation & r : p.relocations( reloc_sec ) )
    {
        const symbol_table_entry & sym = symbols[r.sym()];
        const string_view symbol_name = get_symbol_name(sym);
        
        char * base         = reinterpret_cast<char*>( vs.data() );

        const uint32_t A    = read_uint32_t( base, r.r_offset );
        const uint32_t B    = reinterpret_cast<uint64_t>(base);
        const uint32_t P    = r.r_offset + B;
//         const uint32_t S    = symbol_name.empty() ? 0 : get_sym( symbol_name );
        
        switch ( r.type() )
        {
        case r_386::pc32:
            write_uint32_t( base, r.r_offset, get_sym( symbol_name ) + A - P );
            break;    
        case r_386::jmp_slot:
            write_uint32_t( base, r.r_offset, get_sym( symbol_name ) );
            break;
        case r_386::relative:
            write_uint32_t( base, r.r_offset, A + B );
            break;
        case r_386::glob_dat:
            write_uint32_t( base, r.r_offset, get_sym( "abort") );
            break;
        default:
            std::cerr << "Unsupported relocation" << std::endl;
            break;
        }
    }
}

/**
 * @brief Read the dynamic symbols
 */
std::unordered_map< std::string, uint32_t > read_elf32_dynsym( const parser & p, mmap_region const & vs )
{
    std::unordered_map< std::string, uint32_t > symbols;

    using boost::find;
    using boost::adaptors::transformed;

    auto dynsym_section_hdr = find( p.section_headers() | transformed( std::mem_fn(&section_header::sh_type) ),
                                    sht::dynsym ).base();
    
    assert( dynsym_section_hdr != p.section_headers().end() );

    auto get_symbol_name = 
        [ symbol_names = p.string_table( dynsym_section_hdr->sh_link )](const symbol_table_entry & sym)
        { 
            return sym.st_name == 0 ? string_view() : symbol_names.get_string( sym.st_name );
        };

    for ( const symbol_table_entry & ste : p.symbols(*dynsym_section_hdr) )
    {
        const section_header & section = p.section_headers()[ ste.st_shndx ];
        symbols.emplace( get_symbol_name(ste), reinterpret_cast<uint64_t>(vs.at(ste.st_value) ) );
    }
    
    return symbols;
}


} //namespace

void mmap_region::deallocate()
{
    munmap( addr_, size_ );
}
    
loader::loader(const std::string& filename, get_symbol_t const & get_sym)
{
    std::ifstream in(filename, std::ios_base::in | std::ios_base::binary );
    
    std::vector<char> file;
    std::copy( std::istreambuf_iterator<char>(in),
               std::istreambuf_iterator<char>(),
               std::back_inserter(file) );
    
    parser p( string_view(file.data(), file.size()) );
    
    // Load the entire DSO
    data_ = load_elf32(p);
    
    // Read symbols
    symbols_ = read_elf32_dynsym( p, data_ );

    // Relocate
    auto get_symbols = [&get_sym, this]( std::experimental::string_view sym ) -> uint32_t
    {
        uint32_t sym_glob = get_sym(sym);
        if ( sym_glob != 0 )
        {
            return sym_glob;
        }

        auto iter = symbols_.find( sym.to_string() );
        if ( iter != symbols_.end() )
        {
            return iter->second;
        }
        
        return 0;
    };
    
    for ( const section_header & sec : p.section_headers() )
    {
        if ( sec.sh_type == sht::rel )
        {
            relocate_elf32(p, sec, data_, std::ref(get_symbols) );
        }
    }
    
//     using boost::find;
//     using boost::adaptors::transformed;
// 
//     auto rel_plt_hdr = find( p.section_headers() | transformed( get_section_name ), ".rel.plt" ).base();
//     assert( rel_plt_hdr != p.section_headers().end() );
// 
//     // Relocate section
//     relocate_elf32(p, *rel_plt_hdr, data_, get_sym );
    
    // Apply proper permissions
    apply_prot_flags(p, data_);
    
    
}

} //namespace elf

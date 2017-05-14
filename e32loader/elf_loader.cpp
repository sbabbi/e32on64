
#include "elf_loader.h"

#include <stdexcept>

namespace elf
{
    
loader::loader(std::vector<char> file) :
    file_(std::move(file))
{
    if ( file_.size() < sizeof(struct header) )
    {
        throw std::runtime_error("invalid elf file");
    }
    
    const struct header & hdr = header();
    
    const uint8_t expected_ident[] = 
    {
        0x7F,
        'E',
        'L',
        'F',
        1, // Class
        1, // Byte order
        1  // Version
    };
    
    if ( ! std::equal(std::begin(expected_ident),
                      std::end(expected_ident),
                      std::begin(hdr.e_ident) ) ||
        
        hdr.e_type != et::dyn ||
        hdr.e_machine != em::_386 || 
        hdr.e_shentsize != sizeof(section_header) )
    {
        throw std::runtime_error("Invalid or unsuppoerted e_ident");
    }
}

std::ostream& operator<<(std::ostream & os, r_386 value )
{
    switch( value )
    {
    default:
    case r_386::none:      return os << "none";
    case r_386::_32:       return os << "32";
    case r_386::pc32:      return os << "pc32";
    case r_386::got32:     return os << "got32";
    case r_386::plt32:     return os << "plt32";
    case r_386::copy:      return os << "copy";
    case r_386::glob_dat:  return os << "glob_dat";
    case r_386::jmp_slot:  return os << "jmp_slot";
    case r_386::relative:  return os << "relative";
    case r_386::gotoff:    return os << "gotoff";
    case r_386::gotpc:     return os << "gotpc";
    }
}
    
} //namespace elf

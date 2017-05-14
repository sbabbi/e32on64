
#include "parser.h"

namespace elf
{
    
parser::parser( string_view data ) :
    data_(data)
{
    if ( data_.size() < sizeof(struct header) )
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
    
} //namespace elf

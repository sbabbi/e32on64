
#include <iostream>

#include "elf.h"

namespace elf
{
    
std::ostream& operator<<(std::ostream &, et);
std::ostream& operator<<(std::ostream &, em);
std::ostream& operator<<(std::ostream &, sht);

std::ostream& operator<<(std::ostream & os, r_386 r)
{
    switch( r )
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

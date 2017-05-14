
#ifndef E32LOADER_PARSER_H
#define E32LOADER_PARSER_H

#include <stdexcept>
#include <experimental/string_view>
#include <boost/range/iterator_range.hpp>

#include "elf.h"

namespace elf
{

using std::experimental::string_view;

/**
 * @brief An elf parser.
 */
class parser
{
public:
    
    /**
     * @brief An handle to the string table
     */
    struct string_table_view
    {
        explicit string_table_view( string_view table ) :
            table_(table)
        {}
        
        string_view get_string( word_t sh_name ) const
        {
            if ( sh_name >= table_.size() )
            {
                throw std::out_of_range("get_string");
            }
            
            return string_view( table_.data() + sh_name );
        }
        
    private:
        string_view table_;
    };    
    
    explicit parser( string_view data );
    
    string_view raw_block( offset_t start, word_t size ) const
    {
        if ( (start + size ) >= data_.size() )
        {
            throw std::out_of_range("raw_block");
        }
        
        return data_.substr( start, size );
    }
    
    /**
     * @brief Retrieve the elf header
     */
    const struct header & header() const
    {
        return * access< struct header >( 0 );
    }
        
    /**
     * @brief Retrieve the range of program headers
     */
    auto program_headers() const
    {
        return boost::make_iterator_range(
            access< program_header > ( header().e_phoff ),
            access< program_header > ( header().e_phoff ) + header().e_phnum );
    }

    /**
     * @brief Retrieve the range of section headers
     */
    auto section_headers() const
    {
        return boost::make_iterator_range(
            access< section_header > ( header().e_shoff ),
            access< section_header > ( header().e_shoff ) + header().e_shnum );
    }
    
    /**
     * @brief Retrieve a string table by section index 
     */
    string_table_view string_table( const section_header & hdr ) const
    {
        if ( hdr.sh_type != sht::strtab )
        {
            throw std::invalid_argument("string_table");
        }
        
        return string_table_view(
            string_view( data_.data() + hdr.sh_offset,
                         hdr.sh_size ) );
    }

    /**
     * @overload
     */
    string_table_view string_table( half_t sh_ndx ) const
    {
        auto headers = section_headers();
        
        if ( sh_ndx >= headers.size() )
        {
            throw std::out_of_range("string_table");
        }
        
        return string_table( headers[sh_ndx] );
    }
    
    /**
     * @brief Retrieve a list of symbols in the given section
     */
    auto symbols( const section_header & hdr ) const 
    { 
        if ( hdr.sh_type != sht::dynsym && hdr.sh_type != sht::symtab )
        {
            throw std::invalid_argument("symbols");
        }
        
        if ( hdr.sh_size % sizeof(symbol_table_entry) != 0 )
        {
            throw std::runtime_error("invalid_symbol_section");
        }
        
        return boost::make_iterator_range(
            access< symbol_table_entry > ( hdr.sh_offset ),
            access< symbol_table_entry > ( hdr.sh_offset + hdr.sh_size ) );
    }
        
    /**
     * @overload
     */
    auto symbols( half_t sh_ndx ) const 
    {
        auto headers = section_headers();
        
        if ( sh_ndx >= headers.size() )
        {
            throw std::out_of_range("symbols");
        }
        
        return symbols( headers[sh_ndx] );
    }
    
    /**
     * @brief Get the raw data in a section
     */
    string_view section( const section_header & hdr ) const
    {
        return string_view( access<char>( hdr.sh_offset ), hdr.sh_size );
    }
    
    /**
     * @overload
     */
    string_view section( half_t sh_ndx ) const
    {
        auto headers = section_headers();
        
        if ( sh_ndx >= headers.size() )
        {
            throw std::out_of_range("section");
        }
        
       return section( headers[sh_ndx] );
    }
    
    /**
     * @brief Get a list of relocations at the given table entry
     */
    auto relocations( const section_header & hdr ) const
    {
        if ( hdr.sh_type != sht::rel )
        {
            throw std::invalid_argument("relocations");
        }
        
        if ( hdr.sh_size % sizeof(relocation) != 0 )
        {
            throw std::runtime_error("invalid_relocations_section");
        }

        return boost::make_iterator_range(
            access< relocation >( hdr.sh_offset ),
            access< relocation >( hdr.sh_offset + hdr.sh_size ) );
    }
    
    /**
     * @overload
     */
    auto relocations( half_t sh_ndx ) const
    {
        auto headers = section_headers();
        
        if ( sh_ndx >= headers.size() )
        {
            throw std::out_of_range("relocations");
        }
        
       return relocations( headers[sh_ndx] );
    }

private:
    template<class T>
    const T * access( offset_t off ) const
    {
        return reinterpret_cast< const T * >( data_.data() + off );
    }
    
    string_view data_;
};
    
} // namespace elf

#endif //E32LOADER_PARSER_H

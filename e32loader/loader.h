
#ifndef E32LOADER_LOADER_H
#define E32LOADER_LOADER_H

#include <functional>
#include <string>
#include <unordered_map>
#include <experimental/string_view>

namespace elf
{

class mmap_region
{
public:
    mmap_region() : addr_(nullptr), size_(0) {}
    
    explicit mmap_region(void * addr, std::size_t size) :
        addr_(addr),
        size_(size)
    {}
    
    mmap_region( mmap_region && other ) noexcept :
        addr_(other.addr_),
        size_(other.size_)
    {
        other.addr_ = nullptr;
        other.size_ = 0;
    }
    
    mmap_region& operator=( mmap_region && other ) noexcept
    {
        mmap_region cpy(std::move(other));
        swap(cpy);
        return *this;
    }
    
    ~mmap_region()
    {
        if ( addr_ != nullptr )
        {
            deallocate();
        }
    }
    
    void * data() const { return addr_; }
    std::size_t size() const { return size_; }
    
    void * at( uint32_t off ) const
    {
        if ( off >= size_ )
        {
            throw std::out_of_range( "mmap_region::at" );
        }
        return reinterpret_cast< char * >(addr_) + off;
    }

    void swap( mmap_region & other ) noexcept
    {
        std::swap( addr_, other.addr_ );
        std::swap( size_, other.size_ );
    }
        
private:
    void deallocate();
    
    void * addr_;
    uint32_t size_;
};
    
class loader
{
public:
    using get_symbol_t = std::function<uint32_t(std::experimental::string_view)>;
    
    explicit loader( const std::string & filename, get_symbol_t const & );
    
    uint32_t get_sym( const char * name ) const { return symbols_.at(name); }
    
private:
    mmap_region data_;
    std::unordered_map< std::string, uint32_t > symbols_;
};

} //namespace elf

#endif //E32LOADER_LOADER_H

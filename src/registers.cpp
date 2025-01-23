#include <iostream>

#include <libpdb/register.hpp>
#include <libpdb/bit.hpp>
#include <libpdb/process.hpp>

pdb::registers::value pdb::registers::read(const register_info& info) const 
{

    // we retrieve a pointer to raw bytes of register data 
    auto bytes = as_bytes(data_);


    // The register_info objects have an offset member that will tell us where in this bunch of bytes we can find the data we need.
    if(info.format == register_format::uint)
    {
        switch (info.size)
        {
        case 1:
            return from_bytes<std::uint8_t> (bytes + info.offset);
        case 2: 
            return from_bytes<std::uint16_t> (bytes + info.offset);
        case 4:
            return from_bytes<std::uint32_t> (bytes + info.offset);
        case 8: 
            return from_bytes<std::uint64_t> (bytes + info.offset);
        default:
            pdb::error::send("Unexpected register size");
        }   
    }
    else if(info.format == register_format::double_float)
    {
        return from_bytes<double>(bytes + info.offset);
    }
    else if(info.format == register_format::long_double)
    {
        return from_bytes<long double>(bytes + info.offset);
    }
    else if(info.format == register_format::vector and info.size == 8)
    {
        return from_bytes<byte64>(bytes + info.offset);
    }
    else
    {
        return from_bytes<byte128> (bytes + info.offset);
    }
} 

void pdb::registers::write(const register_info& info, value val)
{
    // first get the pointer to the whole registers memory addresses
    auto bytes = as_bytes(data_);

    // std::visit takes in a function and a variant
    // it calls the given function with the value stored in std::variant
    // if use a direct approach of using if else statements then it might get hard to maintain when the size of variant increase so to avoid that we use visit function
    // std::visit preserves the type
    std::visit([&](auto& v) {
        // this size gives us the size of type
        if(sizeof(v) == info.size)
        {
            auto val_bytes = as_bytes(v);
            std::copy(val_bytes, val_bytes + sizeof(v), bytes + info.offset);
        }
        else
        {
            std::cerr << "pdb::register::write called with mismatchted register and value sizes";
            std::terminate();
        }
    }, val);
}
#include <iostream>
#include <type_traits>
#include <algorithm>

#include <libpdb/registers.hpp>
#include <libpdb/bit.hpp>
#include <libpdb/process.hpp>

namespace
{
    template <class T>
    pdb::byte128 widen(const pdb::register_info &info, T t)
    {
        using namespace pdb;
        // regular if statement but runs at compile time
        // if type is fp then we cast it up to widest type before casting to byte128
        // is_floating_point_V is a type trait used to check type 
        // as some static cast might be invalid in runtime we must use constepxr
        if constexpr (std::is_floating_point_v<T>)
        {
            if (info.format == register_format::double_float)
                return to_byte128(static_cast<double>(t));

            if (info.format == register_format::long_double)
                return to_byte128(static_cast<long double>(t));
        }
        // if signed int then we extend the sign ot size of register 
        else if constexpr (std::is_signed_v<T>)
        {
            if (info.format == register_format::uint)
            {
                switch (info.size)
                {
                case 2:
                    return to_byte128(static_cast<std::int16_t>(t));
                case 4:
                    return to_byte128(static_cast<std::int32_t>(t));
                case 8:
                    return to_byte128(static_cast<std::int64_t>(t));
                }
            }
        }

        return to_byte128(t);
    }
}

pdb::registers::value pdb::registers::read(const register_info &info) const
{

    // we retrieve a pointer to raw bytes of register data
    auto bytes = as_bytes(data_);

    // The register_info objects have an offset member that will tell us where in this bunch of bytes we can find the data we need.
    if (info.format == register_format::uint)
    {
        switch (info.size)
        {
        case 1:
            return from_bytes<std::uint8_t>(bytes + info.offset);
        case 2:
            return from_bytes<std::uint16_t>(bytes + info.offset);
        case 4:
            return from_bytes<std::uint32_t>(bytes + info.offset);
        case 8:
            return from_bytes<std::uint64_t>(bytes + info.offset);
        default:
            pdb::error::send("Unexpected register size");
        }
    }
    else if (info.format == register_format::double_float)
    {
        return from_bytes<double>(bytes + info.offset);
    }
    else if (info.format == register_format::long_double)
    {
        return from_bytes<long double>(bytes + info.offset);
    }
    else if (info.format == register_format::vector and info.size == 8)
    {
        return from_bytes<byte64>(bytes + info.offset);
    }
    else
    {
        return from_bytes<byte128>(bytes + info.offset);
    }
}

void pdb::registers::write(const register_info &info, value val)
{
    // first get the pointer to the whole registers memory addresses
    auto bytes = as_bytes(data_);

    // std::visit takes in a function and a variant
    // it calls the given function with the value stored in std::variant
    // if use a direct approach of using if else statements then it might get hard to maintain when the size of variant increase so to avoid that we use visit function
    // std::visit preserves the type
    std::visit([&](auto &v)
               {
        // this size gives us the size of type
        if(sizeof(v) <= info.size)
        {
            auto wide = widen(info, v);
            auto val_bytes = as_bytes(wide);
            std::copy(val_bytes, val_bytes + sizeof(v), bytes + info.offset);
        }
        else
        {
            std::cerr << "pdb::register::write called with mismatchted register and value sizes";
            std::terminate();
        } }, val);

    // here we either write the while fpr at once and if not then write the debug and gprs one by one
    if (info.type == register_type::fpr)
    {
        proc_->write_fprs(data_.i387);
    }
    else
    {

        // to align the registers to 8 bit we flip the last three bits to 0
        // this is done to support the subreg ah bh ch and dh reg
        auto alinged_offset = info.offset & ~0b111;

        // although we made a change in our memory the operating system does not know that the values have been changed
        // ptrace provides an area of memory same format as user struct called user area where we can update it;

        proc_->write_user_area(alinged_offset, from_bytes<std::uint64_t>(bytes + alinged_offset));
    }
}
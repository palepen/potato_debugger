#ifndef PDB_BIT_HPP
#define PDB_BIT_HPP

#include <cstring>
#include <libpdb/types.hpp>

namespace pdb
{
    // takes a template and parameter and some bytes
    // creates a new object of given type and then copies the memory from given bytes to the new object and returns it
    template <class To>
    To from_bytes(const std::byte* bytes)
    {
        To ret;
        std::memcpy(&ret, bytes, sizeof(To));
        return ret;
    }

    // casts the arguments std::bytes
    template <class From>
    const std::byte* as_bytes (const From& from)
    {
        return reinterpret_cast<const std::byte*> (&from);
    }

    template <class From>
    const std::byte* as_bytes(From& from)
    {
        return reinterpret_cast<std::byte*> (&from);
    }

    // {} used bcoz they initialize them to 0

    template <class From>
    byte128 to_byte128(From src)
    {
        byte128 ret{};
        std::memcpy(&ret, &src, sizeof(From));

        return ret;
    }    
    
    template <class From>
    byte64 to_byte64(From src)
    {
        byte64 ret{};
        std::memcpy(&ret, &src, sizeof(From));

        return ret;
    }
}

#endif
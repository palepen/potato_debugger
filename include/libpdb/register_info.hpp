#ifndef PDB_REGISTER_INFO_HPP
#define PDB_REGISTER_INFO_HPP

#include <cstdint>
#include <cstddef>
#include <string_view>
#include <sys/user.h>
#include <algorithm>
#include <libpdb/error.hpp>

namespace pdb
{
    // give a unique value to register
    enum class register_id
    {
        #define DEFINE_REGISTER(name, dwarf_id, size, offset, type, format) name
        #include <libpdb/detail/register.inc>
        #undef DEFINE_REGISTER
    };


    enum class register_type
    {
        gpr /*General Purpose Register*/, 
        sub_gpr,  /*Subregister of a GPR eg 32-bit (eg :eax ) for rax*/
        fpr /*Floating Point register*/, 
        dr /*Debugging Register*/ 
    };
    
    // shows diff ways of enumerating or parsiong a register 
    enum class register_format
    {
        uint,
        double_float, 
        long_double,
        vector
    };

    // stores information abt a register 
    struct register_info
    {
        register_id id;
        std::string_view name;
        std::int32_t dwarf_id;
        std::size_t size;
        std::size_t offset; // The relative position of the register within a larger memory block or structure.
        register_type type;
        register_format format;
    };

    // global array of info abt every register
    inline constexpr const register_info g_register_infos[] = {
        #define DEFINE_REGISTER(name, dwarf_id, size, offset, type, format) \
            {register_id::name, #name, dwarf_id, size, offset, type, format}
        #include <libpdb/detail/register.inc>
        #undef DEFINE_REGISTER
    };


    // returns the the required register by passing a default comparator
    template<class F>
    const register_info& register_info_by(F f)
    {
        auto it = std::find_if(std::begin(g_register_infos), std::end(g_register_infos), f);

        // throws error if the not found
        if(it == std::end(g_register_infos))
            error::send("Can't find register info");

        return *it;
    }


    // finds the register by id
    inline const register_info& register_info_by_id(register_id id)
    {
        return register_info_by([id](auto &i) { return i.id == id; }); 
    }

    // finds the register by name
    inline const register_info& register_info_by_name(std::string_view name)
    {
        return register_info_by([name] (auto &i) { return i.name == name; });   
    }

    // finds the register by dwarf_id
    inline const register_info& register_info_by_dwarf(std::int32_t dwarf_id)
    {
        return register_info_by([dwarf_id](auto &i) { return i.dwarf_id == dwarf_id;});
    }
}
#endif
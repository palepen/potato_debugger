#ifndef PDB_TYPES_HPP
#define PDB_TYPES_HPP

#include <array>
#include <cstddef>

namespace pdb
{
    using byte64 = std::array<std::byte, 8>;
    using byte128 = std::array<std::byte, 16>;
}

#endif
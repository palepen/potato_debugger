#ifndef PDB_REGISTER_HPP
#define PDB_REGISTER_HPP

#include <sys/user.h>
#include <variant>
#include <libpdb/register_info.hpp>
#include <libpdb/types.hpp>

namespace pdb
{
    class process;
    class registers
    {
        public:
            // instance should be unique, so we disallow default construction and copying.
            registers() = delete;
            registers(const registers&) = delete;
            registers& operator=(const registers&) = delete;

            // these are all the types of values a register may contain
            // as diff types are present then we can use variant 
            using value = std::variant<
                std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t,
                std::int8_t, std::int16_t, std::int32_t, std::int64_t,
                float, double, long double, byte64, byte128>;
            
            // read and write func that operate on value type
            value read(const register_info& info ) const;

            void write(const register_info& info, value val);

            template <class T>
            T read_by_id_As(register_id id) const 
            {
                // std::get<T> retrives the type of the data which is use to cast the info
              return std::get<T>(read(register_info_by_id(id)));  
            }

            void write_by_id(register_id id, value val)
            {
                write(register_info_by_id(id), val);
            }

        private:
            // only the pdb::process will construct an pdb::register
            friend process;
            registers(process& proc) : proc_(&proc) {}
            
            // data member user data from sys/user.h and we'll store the reg value here 
            // data_ stores the memomry blokc of the whole registers so if we add any offset then to its address then we can get the snapshot of a current register
            user data_;
            process *proc_;
    };
}

#endif
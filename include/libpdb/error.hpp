#ifndef PDB_ERROR_HPP
#define PDB_ERROR_HPP

#include <stdexcept>
#include <cstring>

namespace pdb
{
    class error : public std::runtime_error
    {
    public:
        // [[noreturn]] means the function will not return to the caller
        // It tells the compiler that control flow will not return to the caller
        
        // send is usually for the errors we want to generate after check
        [[noreturn]]
        static void send(const std::string &what)
        {
            throw error(what);
        }

        // define the errno 
        // this is when an inbuilt library functio fails 
        [[noreturn]]
        static void send_errno(const std::string &prefix)
        {
            // diff betweem std::perror and std::strerror is that it sends the msg as a string rather than printing it to stderr 
            throw error(prefix + ": " + std::strerror(errno));
        }

    private:
        error(const std::string &what) : std::runtime_error(what) {}
    };

}

#endif
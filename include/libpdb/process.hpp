#ifndef PDB_PROCESS_HPP
#define PDB_PROCESS_HPP

#include <filesystem>
#include <memory>
#include <sys/types.h>
#include <cstdint>

namespace pdb{
    
    // to keep track of the state of running process
    enum class process_state{
        stopped, 
        running,
        exited,
        terminated
    };
    
    struct stop_reason
    {
        stop_reason(int wait_status);

        // keeps why the process has stopped
        process_state reason;
    
        // contains info abt stop like return value or signal
        std::uint8_t info;
    };
    
    // we need to create a process type
    // we should not be able to copy this as this is unique and we do not want ot start a new process
    // hence we use smart pointers
    class process{
        public:
            // to delete andy the process
            ~process();

            stop_reason wait_on_signal();

            // to launch a process
            static std::unique_ptr<process> launch(std::filesystem::path path);
            // to attach to a process
            static std::unique_ptr<process> attach(pid_t pid);

            
            void resume();

            pid_t pid() const { return pid_;}
            
            // this is to force to use static members
            process() = delete;

            // delete the copy and move behaviour
            process(const process&) = delete;
            process& operator=(const process&) = delete;

            process_state state() const { return state_;}
        private:

            process(pid_t pid, bool terminate_on_end) : pid_(pid), terminate_on_end_(terminate_on_end){}
            pid_t pid_ = 0;

            // to track termination
            bool terminate_on_end_ = true;

            process_state state_ = process_state::stopped;
    };
}

#endif
#ifndef PDB_PROCESS_HPP
#define PDB_PROCESS_HPP

#include <filesystem>
#include <memory>
#include <sys/types.h>
#include <cstdint>
#include <libpdb/registers.hpp>

namespace pdb
{

    // to keep track of the state of running process
    enum class process_state
    {
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
    class process
    {
    public:
        // to delete andy the process
        ~process();

        stop_reason wait_on_signal();

        // to launch a process
        static std::unique_ptr<process> launch(std::filesystem::path path, bool debug = true);

        // to attach to a process
        static std::unique_ptr<process> attach(pid_t pid);

        void resume();

        pid_t pid() const { return pid_; }

        // this is to force to use static members
        process() = delete;

        // delete the copy and move behaviour
        process(const process &) = delete;
        process &operator=(const process &) = delete;

        process_state state() const { return state_; }

        // const and non-const getter function
        registers &get_registers() { return *registers_; }
        const registers &get_registers() const { return *registers_; }

        // writes in the user area of process
        void write_user_area(std::size_t offset, std::uint64_t data);

        void write_fprs(const user_fpregs_struct &fprs);
        void write_gprs(const user_regs_struct &gprs);

    private:
        process(pid_t pid, bool terminate_on_end, bool is_attached) : pid_(pid), terminate_on_end_(terminate_on_end), is_attached_(is_attached), registers_(new registers(*this)) {}

        void read_all_registers();

        pid_t pid_ = 0;

        // to track termination
        bool terminate_on_end_ = true;
        bool is_attached_ = true;
        process_state state_ = process_state::stopped;

        // pointer to the register data
        std::unique_ptr<registers> registers_;
    };
}

#endif
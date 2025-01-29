#include <libpdb/process.hpp>
#include <libpdb/error.hpp>
#include <libpdb/pipe.hpp>

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace
{
    // writes the error representation to the pipe
    void exit_with_perror(pdb::pipe &channel, std::string const &prefix)
    {
        // get the error msg based on errno
        auto message = prefix + ": " + std::strerror(errno);
        channel.write(
            reinterpret_cast<std::byte *>(message.data()),
            message.size());
        exit(-1);
    }
}

// this function executes the process and waits for it to halt
// optional indicatest that this arg can be empty thats why the null check
std::unique_ptr<pdb::process> pdb::process::launch(std::filesystem::path path, bool debug, std::optional<int> stdout_replacement)
{
    // we set close on exec as true bcoz we dont want to leave the fd hanging
    pipe channel(/*close_on_exec=*/true);

    pid_t pid;

    // when we call fork this program gets duplicated into new process
    // the two process differs in pid ie the child has 0 and parent has pid of child
    if ((pid = fork()) < 0)
    {
        // Error: fork failed
        error::send_errno("fork failed");
    }

    if (pid == 0)
    {
        // if we are inside the process we ensure that we will not perform read operations
        channel.close_read();
        
        // nullcheck for the replacement
        if(stdout_replacement)
        {   
            // dup2 is a syscall which closes the second File Descriptor arg and then duplicates teh first fd to the second
            // simple temrss anything that goes through stdout now goes through whaterver *stdout_replacement points to 
            if(dup2(*stdout_replacement, STDOUT_FILENO) < 0)
            {
                exit_with_perror(channel, "stdout replacement failed");
            }
        }

        // we attach this process(child) by PTRACE_TRACEME
        if (debug and ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0)
        {
            // Error: Trace Failed
            exit_with_perror(channel, "Tracing failed");
        }

        // execute the program passed in arguments
        if (execlp(path.c_str(), path.c_str(), nullptr) < 0)
        {
            // Error: exec failed
            exit_with_perror(channel, "exec failed");
        }
    }

    channel.close_write();
    auto data = channel.read();
    channel.close_read();

    if (data.size() > 0)
    {
        waitpid(pid, nullptr, 0);
        auto chars = reinterpret_cast<char *>(data.data());
        error::send(std::string(chars, chars + data.size()));
    }

    // create a new process and set terminate on end as true as we want to end it on termination of the parent program
    std::unique_ptr<process> proc(new process(pid, /*terminate_on_end=*/true, debug));

    // stop the process after attach to it
    // it will stop at the entry of the program
    if (debug)
        proc->wait_on_signal();

    return proc;
}

// here we attach the process via the pid to the running process or debugger(parent)
std::unique_ptr<pdb::process> pdb::process::attach(pid_t pid)
{
    if (pid == 0)
    {
        // Error: Invalid pid
        error::send("Invalid PID");
    }

    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) < 0)
    {
        // Error: could not attach
        error::send_errno("Could not attach");
    }

    std::unique_ptr<process> proc(new process(pid, /*terminate_on_end=*/false, /*attached=*/true));

    // wait for the process to stop at the entry of the program
    proc->wait_on_signal();

    return proc;
}

pdb::process::~process()
{
    // if pid_ is valid we detach the process
    if (pid_ != 0)
    {
        int status;

        if (is_attached_)
        {

            // inferior(child) must be stopped before we can detach
            if (state_ == process_state::running)
            {
                // SIGSTOP stops the process
                kill(pid_, SIGSTOP);
                // we wait for it to stop by using status flag
                waitpid(pid_, &status, 0);
            }

            // detch the inferior
            ptrace(PTRACE_DETACH, pid_, nullptr, nullptr);

            // let it continue
            kill(pid_, SIGCONT);
        }

        // depending upon whether to terminate on end or not we simply kill the process
        // and then we wait for it to terminate
        if (terminate_on_end_)
        {
            kill(pid_, SIGKILL);
            waitpid(pid_, &status, 0);
        }
    }
}

// we use PTRACE_CONT to continue the process and to keep track on the process we update the state variable
void pdb::process::resume()
{
    if (ptrace(PTRACE_CONT, pid_, nullptr, nullptr) < 0)
    {
        error::send_errno("Could not resume");
    }

    state_ = process_state::running;
}

// wait_status holds the exit signal or signal status
pdb::stop_reason::stop_reason(int wait_status)
{

    // WIFEXITED checks if the child process terminated normally (i.e., via an exit() call or returning from main()).
    if (WIFEXITED(wait_status))
    {
        // set the reason of stop ie exited normally
        reason = process_state::exited;

        // WEXITSTATUS() returns info abt exit status(exit or return) by the macro
        info = WEXITSTATUS(wait_status);
    }

    /*
    WIFSIGNALED() checks whether a child process terminated because it received an uncaught signal.
    This happens if the process is killed by a signal rather than exiting normally or being stopped.
    */
    else if (WIFSIGNALED(wait_status))
    {
        reason = process_state::terminated;
        // WTERMSIG() Extracts the signal number that caused the child process to terminate
        info = WTERMSIG(wait_status);
    }

    /*
    WIFSTOPPED() is used to check whether a child process is currently stopped.
    A stopped process is one that has been paused, usually by receiving a signal
    like SIGSTOP, SIGTSTP, SIGTTIN, or SIGTTOU. it can potentially resume later
    */
    else if (WIFSTOPPED(wait_status))
    {
        reason = process_state::stopped;
        info = WSTOPSIG(wait_status);
    }
}

// utilizes the waitpid() system call to wait for a child process to change state,
// such as terminating or stopping due to a signal.
pdb::stop_reason pdb::process::wait_on_signal()
{
    int wait_status;
    int options = 0;

    if (waitpid(pid_, &wait_status, options) < 0)
    {
        error::send_errno("waitpid failed");
    }

    stop_reason reason(wait_status);
    state_ = reason.reason;
    
    // every time the inferior is not terminated and stopped then we read all the register values
    if(is_attached_ and state_ == process_state::stopped)
    {
        read_all_registers();
    }

    return reason;
}

void pdb::process::read_all_registers()
{
    // read all the gpr and store them in the data_.regs  
    if(ptrace(PTRACE_GETREGS, pid_, nullptr, &get_registers().data_.regs) < 0)
    {
        error::send_errno("Could not read GPR registers");
    }
    
    // read all the fpr and store them in the data_.i387 
    if(ptrace(PTRACE_GETFPREGS, pid_, nullptr, &get_registers().data_.i387) < 0)
    {
        error::send_errno("Could not read FPR registers");    
    }

    // we cant simple loop over the enums of the debug registers then we use this approach
    for(int i = 0; i < 8; i++)
    {
        // retrieve the id of the dr0 register then start adding the index to it to get the correct id 
        // hence we cast it to int
        auto id = static_cast<int> (register_id::dr0) + i;

        // cast by to the register_id and call the func
        auto info = register_info_by_id(static_cast<register_id>(id));

        errno = 0;
        // now we read the data and store it in data
        std::int64_t data = ptrace(PTRACE_PEEKUSER, pid_, info.offset, nullptr);
        if(errno != 0) error::send_errno("Could not read FPR registers");    

        // store this in user data_
        get_registers().data_.u_debugreg[i] = data;
    }
}

void pdb::process::write_user_area(std::size_t offset, std::uint64_t data)
{
    // PTRACE_POKEUSER is used to write data in the user area by ptrace
    if(ptrace(PTRACE_POKEUSER, pid_, offset, data) < 0)
    {
        error::send_errno("Could not write to user area");
    }
}


// as reading and writing may cause an error here we simply write all the FPRs
void pdb::process::write_fprs(const user_fpregs_struct& fprs)
{
    if(ptrace(PTRACE_SETFPREGS, pid_, nullptr, &fprs) < 0)
    {
        error::send_errno("Could not write FP registers");
    }
}

void pdb::process::write_gprs(const user_regs_struct& fprs)
{
    if(ptrace(PTRACE_SETREGS, pid_, nullptr, &fprs) < 0)
    {
        error::send_errno("Could not write GP registers");
    }
}
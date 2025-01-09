#include <libpdb/process.hpp>
#include <libpdb/error.hpp>

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// this function executes the process and waits for it to halt
std::unique_ptr<pdb::process> pdb::process::launch(std::filesystem::path path)
{

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
        // we attach this process(child) by PTRACE_TRACEME
        if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0)
        {
            // Error: Trace Failed
            error::send_errno("Tracing failed");
        }

        // execute the program passed in arguments
        if (execlp(path.c_str(), path.c_str(), nullptr) < 0)
        {
            // Error: exec failed
            error::send_errno("exec failed");
        }
    }

    // create a new process and set terminate on end as true as we want to end it on termination of the parent program
    std::unique_ptr<process> proc(new process(pid, /*terminate_on_end=*/true));

    // stop the process after attach to it 
    // it will stop at the entry of the program
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

    std::unique_ptr<process> proc(new process(pid, /*terminate_on_end=*/false));
    
    // wait for the process to stop at the entry of the program 
    proc->wait_on_signal();

    return proc;
}

pdb::process::~process()
{
    // if pid_ is valid we detach the process
    if(pid_ != 0)
    {
        int status;

        // inferior(child) must be stopped before we can detach
        if(state_ == process_state::running)
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


        // depending upon whether to terminate on end or not we simply kill the process
        // and then we wait for it to terminate
        if(terminate_on_end_)
        {
            kill(pid_, SIGKILL);
            waitpid(pid_, &status, 0);
        }
    }
}



// we use PTRACE_CONT to continue the process and to keep track on the process we update the state variable
void pdb::process::resume()
{
    if(ptrace(PTRACE_CONT, pid_, nullptr, nullptr) < 0)
    {
        error::send_errno("Could not resume");
    }

    state_ = process_state::running;
}

// wait_status holds the exit signal or signal status
pdb::stop_reason::stop_reason(int wait_status)
{

  // WIFEXITED checks if the child process terminated normally (i.e., via an exit() call or returning from main()).
    if(WIFEXITED(wait_status))
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
    else if(WIFSIGNALED(wait_status))
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

    if(waitpid(pid_, &wait_status, options) < 0)
    {
        error::send_errno("waitpid failed");
    }

    stop_reason reason(wait_status);
    state_ = reason.reason;
    return reason;
}
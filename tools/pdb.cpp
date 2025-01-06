#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <sstream>

#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <readline/readline.h>
#include <readline/history.h>

// namespace with no name is used when we want to restrict the programs to this file only
namespace
{

    std::vector<std::string> split(std::string_view str, char delimiter)
    {
        std::vector<std::string> out{};
        std::stringstream ss{std::string{str}};
        std::string item;

        // read the string from str till it hits a delimiter
        while (std::getline(ss, item, delimiter))
        {
            out.push_back(item);
        }

        return out;
    }

    // returns if the string is equal to prefix or not
    bool is_prefix(std::string_view str, std::string_view of)
    {
        if(str.size() > of.size()) return false;
        return std::equal(str.begin(), str.end(), of.begin());
    }


    // to continue the process 
    void resume(pid_t pid)
    {
        // PTRACE_CONT req is used to continue a given process
        if(ptrace(PTRACE_CONT, pid, nullptr, nullptr ) < 0)
        {
            std::cerr << "Couldn't continue\n";
            std::exit(-1);
        }

    }

    //wait till we get a signal 
    void wait_on_signal(pid_t pid)
    {
        int wait_status;
        int options = 0;

        while (waitpid(pid, &wait_status, options) < 0)
        {
            std::perror("waitpid failed");
            std::exit(-1);
        }
        
    }

    // handles each command
    void handle_command(pid_t pid, std::string_view line)
    {

        // split the command on spaces (can be used when multiple arguments)
        std::vector<std::string> args = split(line, ' ');

        std::string command = args[0];

        // if the prefix is continue then continue then wait for a singal
        // this signal can be hardware or software(breakpoints)

        
        if (is_prefix(command, "continue"))
        {
            resume(pid);
            wait_on_signal(pid);
        }
        // if not recognized then we print errir
        else
        {
            std::cerr << "Unknown command\n";
        }
    }

}

namespace
{
    pid_t attach(int argc, const char **argv)
    {
        pid_t pid = 0;

        if (argc == 1 && argv[1] == std::string_view("-p"))
        {
            pid = std::atoi(argv[2]);
            if (pid <= 0)
            {
                std::cerr << "Invalid pid\n";
                return -1;
            }
            // attach a running process
            if (ptrace(PTRACE_ATTACH, pid, /*addr=*/nullptr, /*data=*/nullptr) < 0)
            {
                std::perror("Could not attach");
                return -1;
            }
        }
        else
        {
            const char *program_path = argv[1];

            if ((pid = fork()) < 0)
            {
                std::perror("fork failed");
                return -1;
            }

            // when we call fork this program gets duplicated into new process
            // the two process differs in pid ie the child has 0 and parent has pid of child
            if (pid == 0)
            {
                // we attach this process(child) by PTRACE_TRACEME
                if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0)
                {
                    std::perror("Tracing failed");
                    return -1;
                }

                // execute the program passed in arguments
                if (execlp(program_path, program_path, nullptr) < 0)
                {
                    std::perror("Exec failed");
                    return -1;
                }
            }
        }

        return pid;
    }
}

int main(int argc, const char **argv)
{
    if (argc == 1)
    {
        std::cerr << "No arguments give, Format-\n";
        std::cerr << "1. pdb <filename>\n";
        std::cerr << "2. pdb -p <pid>\n";
        return -1;
    }

    pid_t pid = attach(argc, argv);

    int wait_status;
    // helps to pass flags
    int options = 0;

    // here we wait for the process to stop after attach to it
    if (waitpid(pid, &wait_status, options) < 0)
    {
        std::perror("waitpid failed");
    }

    char *line = nullptr;

    // read new lines from command line
    while ((line = readline("pdb> ")) != nullptr)
    {
        // common variable for storing the command
        std::string line_str;

        // check if the command is empty
        if (line == std::string_view(""))
        {
            free(line);

            // get the last instruction [feature]
            // history_list and history_length = readline feature
            if (history_length > 0)
            {
                line_str = history_list()[history_length - 1]->line;
            }
        }
        else
        {
            line_str = line;

            // we add this command to searchable history
            add_history(line);

            // cleanup memory
            free(line);
        }

        if (!line_str.empty())
        {
            // handle the prompt given
            handle_command(pid, line);
        }
    }
}
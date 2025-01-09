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

#include <libpdb/process.hpp>
#include <libpdb/error.hpp>

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
        if (str.size() > of.size())
            return false;
        return std::equal(str.begin(), str.end(), of.begin());
    }

    // whenever a child process or inferior stops we infer or print the reason here
    void print_stop_reason(const pdb::process &process, pdb::stop_reason reason)
    {
        std::cout << "Process " << process.pid() << ' ';

        switch (reason.reason)
        {
        case pdb::process_state::exited:
            std::cout << "Exited with status " << static_cast<int>(reason.info);
            break;

        // sigabbrev_np gets abbreviation for the signal code or we could also use sys_siglist[reason.info]
        case pdb::process_state::terminated:
            std::cout << "Terminated with signal " << sigabbrev_np(reason.info);
            break;

        case pdb::process_state::stopped:
            std::cout << "Stopped with signal " << sigabbrev_np(reason.info);

        default:
            break;
        }

        std::cout << std::endl;
    }

    // handles each command passed through cmd
    void handle_command(std::unique_ptr<pdb::process> &process, std::string_view line)
    {

        // split the command on spaces (can be used when multiple arguments)
        std::vector<std::string> args = split(line, ' ');

        std::string command = args[0];

        // if the prefix is continue then continue
        // this signal can be hardware or software(breakpoints)
        if (is_prefix(command, "continue"))
        {
            process->resume();
            // then we wait for the child process to stopped or terminated
            pdb::stop_reason reason = process->wait_on_signal();

            // print the reason
            print_stop_reason(*process, reason);
        }
        // if not recognized then we print errir
        else
        {
            std::cerr << "Unknown command\n";
        }
    }

    std::unique_ptr<pdb::process> attach(int argc, const char **argv)
    {

        if (argc == 3 && argv[1] == std::string_view("-p"))
        {
            pid_t pid = std::atoi(argv[2]);
            // we attach a process
            return pdb::process::attach(pid);
        }
        else
        {
            const char *program_path = argv[1];
            // launch the new program and attach
            return pdb::process::launch(program_path);
        }
    }

    void main_loop(std::unique_ptr<pdb::process> &process)
    {
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
                try
                {
                    handle_command(process, line_str);
                }
                catch (const pdb::error& err)
                {
                    std::cout << err.what() << '\n';
                }
            }
        }
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

    try
    {
        // attach to the inferior 
        std::unique_ptr<pdb::process> process = attach(argc, argv);
        
        // start executing the debugger 
        main_loop(process);
    }
    catch (const pdb::error& err)
    {
        std::cout << err.what() << '\n';
    }
}
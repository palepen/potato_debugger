#include <catch2/catch_test_macros.hpp>
#include <libpdb/process.hpp>
#include <sys/types.h>
#include <signal.h>
#include <libpdb/error.hpp>
#include <fstream>

using namespace pdb;

namespace
{
    char get_process_status(pid_t pid)
    {
        // open the stat file for the given pid
        std::ifstream stat("/proc/" + std::to_string(pid) + "/stat");

        std::string data;
        // read the line into the data var
        std::getline(stat, data);

        // find the index of the indicator
        auto index_of_status_indicator = data.rfind(')') + 2;
        return data[index_of_status_indicator];
    }

    // checks if a process exist or not
    bool process_exists(pid_t pid)
    {
        auto ret = kill(pid, 0);
        // when kill fails bcoz process does not exits it returns -1 and ESRCH is set
        return ret != -1 and errno != ESRCH;
    }
}

// define testcase for launch
TEST_CASE("process::launch success", "[process]")
{
    auto proc = process::launch("yes");
    REQUIRE(process_exists(proc->pid()));
}

// check for non existence of program
TEST_CASE("process::launch no such program", "[process]")
{
    REQUIRE_THROWS_AS(process::launch("potato_are_good"), error);
}

TEST_CASE("process::attach invalid PID", "[process]")
{
    REQUIRE_THROWS_AS(process::attach(0), error);
}

TEST_CASE("process::resume already terminated", "[process]")
{
    
    auto proc = process::launch("targets/end_immediately");
    proc->resume();
    proc->wait_on_signal();
    REQUIRE_THROWS_AS(proc->resume(), error);
}

TEST_CASE("process::attach success", "[process]")
{
    auto target = process::launch("targets/run_endlessly", false);
    auto proc = process::attach(target->pid());
    REQUIRE(get_process_status(target->pid()) == 't');
}

TEST_CASE("process::resume sucess", "[process]")
{
    {
        auto proc = process::launch("targets/run_endlessly");

        proc->resume();
        auto status = get_process_status(proc->pid());

        auto sucess = status == 'R' or status == 'S';
        REQUIRE(sucess);
    }
    {
        auto target = process::launch("targets/run_endlessly", false);
        auto proc = process::attach(target->pid());
        proc->resume();
        auto status = get_process_status(proc->pid());

        auto sucess = status == 'R' or status == 'S';
        REQUIRE(sucess);
    }
}

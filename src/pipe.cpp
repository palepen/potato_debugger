#include <unistd.h>
#include <fcntl.h>
#include <libpdb/error.hpp>
#include <libpdb/pipe.hpp>
#include <utility>

pdb::pipe::pipe(bool close_on_exec)
{
    // this pipe2 is used to create a new pipe and O_CLOEXEC teels to close when we call execlp

    if(pipe2(fds_, close_on_exec ?  O_CLOEXEC : 0) < 0)
    {
        error::send_errno("Pipe Creation failed");
    }
}

pdb::pipe::~pipe()
{
    close_read();
    close_write();   
}


// std::exchange swaps the fd
int pdb::pipe::release_read()
{
    return std::exchange(fds_[write_fd], -1);
}

int pdb::pipe::release_write()
{
    return std::exchange(fds_[write_fd], -1);
}

void pdb::pipe::close_read()
{
    if(fds_[read_fd] != -1)
    {
        close(fds_[read_fd]);
        fds_[read_fd] = -1;
    }
}

void pdb::pipe::close_write()
{
    if(fds_[write_fd] != -1)
    {
        close(fds_[write_fd]);
        fds_[write_fd] = -1;
    }
}

// we read in this function and it returns the data as array of bytes
std::vector<std::byte> pdb::pipe::read()
{
    char buf[1024];
    int char_read;

    // reads from the fd onto the buffer
    // We call ::read to fill the buffer from the read end of the pipe
    // :: also ensure that we definitely call the read function in the global namespace rather than some other one
    if((char_read = ::read(fds_[read_fd], buf, sizeof(buf))) < 0)
    {
        error::send_errno("Could not read from pipe");
    } 

    // reinterpret_cast : unless the original type we cant access data 
    auto bytes = reinterpret_cast<std::byte*> (buf);
    return std::vector<std::byte>(bytes, bytes + char_read);
}

// write in the fd
void pdb::pipe::write(std::byte* from, std::size_t bytes)
{
    if(::write(fds_[write_fd], from, bytes) < 0)
    {
        error::send_errno("Could not write to pipe");
    }
}


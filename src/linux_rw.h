#ifndef LINUX_RW_H_
#define LINUX_RW_H_

#include <unistd.h>
#include <errno.h>
#include <poll.h>

namespace linux_rw
{
    /**
     * It waits for the given file descriptor to become ready to perform read
     * @param fd File descriptor to read from
     * @param timeout_ms Timeout in milliseconds
     * @return Zero is returned on success, -ETIMEDOUT if the timeout expired, otherwise a negative errno
     */
    int poll_read(int fd, int timeout_ms)
    {
        struct pollfd fds[1];
        fds[0].fd = fd;
        fds[0].events = POLLIN;
        int poll_res = poll(fds, 1, timeout_ms);
        if( poll_res == 1)          { return 0; } 
        else if( poll_res == 0 )    { return -ETIMEDOUT; }
        else                        { return -errno; }
    }
    /**
     * It waits for the given file descriptor to become ready to perform write
     * @param fd File descriptor to write to
     * @param timeout_ms Timeout in milliseconds
     * @return Zero is returned on success, -ETIMEDOUT if the timeout expired, otherwise a negative errno
     */
    int poll_write(int fd, int timeout_ms)
    {
        struct pollfd fds[1];
        fds[0].fd = fd;
        fds[0].events = POLLOUT;
        int poll_res = poll(fds, 1, timeout_ms);
        if( poll_res == 1)          { return 0; } 
        else if( poll_res == 0 )    { return -ETIMEDOUT; }
        else                        { return -errno; }
    }
    /**
     * Read up to 'size' bytes from file descriptor fd into the buffer starting at 'buffer'
     * @param fd File descriptor to read from
     * @param buffer Read buffer
     * @param size Size to read into buffer
     * @return On success 'size' is returned. On error the number of already read bytes is returned 
     * and errno is set appropriately
     */
    inline ssize_t read_all(int fd, void* buffer, size_t size)
    {
        unsigned char* cursor = (unsigned char*)buffer;
        ssize_t read_size = 0;
        size_t remaining_size = size;
        errno = 0;//reset errno
        while(remaining_size > 0)
        {
            read_size = read(fd, cursor, remaining_size);
            if(read_size <= 0) { return (size - remaining_size); }
            remaining_size -= read_size;
            cursor += read_size;
        }
        return size;
    }
    /**
     * 
     */
    inline ssize_t write_all(int fd, const void *buffer, size_t size, int timeout_ms = -1, int retry_ms = 100)
    {
        unsigned char* cursor = (unsigned char*)buffer;
        ssize_t written_size = 0;
        size_t remaining_size = size;
        errno = 0;//reset errno
        while(remaining_size > 0)
        {
            written_size = write(fd, cursor, remaining_size);
            if(written_size == 0) 
            {
                int retry_cnt = -1;
                if(timeout_ms > 0)
                { retry_cnt = timeout_ms / retry_ms; }
                //cannot take more data
                int poll_res = -ETIMEDOUT;
                while( (poll_res = poll_write(fd, retry_ms)) != 0 )
                {
                    if( retry_cnt == 0 )         
                    { 
                        errno = ETIMEDOUT;
                        return (size - remaining_size);
                    }
                    if( poll_res != -ETIMEDOUT ) { return (size - remaining_size); }
                    if( retry_cnt > 0 ) { --retry_cnt; }
                }
                continue;
            }
            else if( written_size < 0 ) { return (size - remaining_size); } //on error
            remaining_size -= written_size;
            cursor += written_size;
        }
        return size;
    }
}

#endif

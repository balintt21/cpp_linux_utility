#ifndef IPC_DATAGRAM_SOCKET_H_
#define IPC_DATAGRAM_SOCKET_H_

#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

namespace linux_ipc
{
    /**
     * Tells if the given file descriptor is open and socket type
     * @param fd file descriptor
     * @return On success true is returned, otherwise false
     */
    inline bool isOpenedSocket(int fd)
    {
        struct stat st;
        return ( (fcntl(fd, F_GETFD) >= 0) && (fstat(fd, &st) == 0) && S_ISSOCK(st.st_mode) );
    }

    class DatagramSocket
    {
    protected:
        int mUnixSocket;
    public:
        DatagramSocket(int fd) : mUnixSocket(fd) { }
        explicit operator bool() const noexcept { return isOpenedSocket(mUnixSocket); }
        /**
         * Send message on a unix datagram type socket
         * @param data - Pointer to the data to be sent
         * @param size - Data size
         * @return On success the number of sent bytes is returned, otherwise a negative errno value
         */
        inline ssize_t sendMessage(void* data, size_t size)
        {
            struct msghdr msg_hdr;
            struct iovec  msg;
            memset(&msg_hdr, 0, sizeof(msghdr));
            msg.iov_base = data;
            msg.iov_len = size;
            msg_hdr.msg_iov = &msg;
            msg_hdr.msg_iovlen = 1;
            ssize_t res = sendmsg(mUnixSocket, &msg_hdr, 0);
            if(res >= 0) { return res; }
            else { return -errno; }
        }
        /**
         * Receive message from a unix datagram type socket
         * @param buffer - Receive buffer
         * @param size - Receive buffer size
         * @return On success the number of received bytes is returned, otherwise a negative errno value
         */
        inline ssize_t recvMessage(void* buffer, size_t size)
        {
            struct msghdr msg_hdr;
            struct iovec  msg;
            memset(&msg_hdr, 0, sizeof(msghdr));
            msg.iov_base = buffer;
            msg.iov_len = size;
            msg_hdr.msg_iov = &msg;
            msg_hdr.msg_iovlen = 1;
            ssize_t res = recvmsg(mUnixSocket, &msg_hdr, 0);
            if(res >= 0) { return res; }
            else { return -errno; }
        }
    };
}

#endif

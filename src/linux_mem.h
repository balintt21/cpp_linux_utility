#ifndef LINUX_MEMORY_H_
#define LINUX_MEMORY_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string>

namespace linux_mem
{
    class PhysicalMemory
    {
    private:
        int         mMemoryDeviceFd;
        size_t      mVirtualSize;
        void*       mVirtualAddress;
        size_t      mMapSize;
        error_t     mError;
        const bool  mReadOnly;
    public:
        PhysicalMemory(off_t offset, size_t size, bool read_only = false) 
            : mMemoryDeviceFd(-1), mVirtualSize(size), mVirtualAddress(MAP_FAILED), mMapSize(size), mError(0), mReadOnly(read_only)
        {
            mMemoryDeviceFd = open("/dev/mem", (read_only ? O_RDONLY : O_RDWR));
            if(mMemoryDeviceFd >= 0)
            {
                int page_size = getpagesize();
                size_t remainder = size % page_size;
                mMapSize = size + ( (remainder > 0) ? (page_size - remainder) : (0) ); 
                mVirtualAddress = mmap(nullptr, mMapSize, (read_only ? (PROT_READ) : (PROT_READ | PROT_WRITE)), MAP_SHARED, mMemoryDeviceFd, offset );
                if(mVirtualAddress == MAP_FAILED)
                { mError = errno; }
            } 
            else 
            { mError = errno; }
        }
        ~PhysicalMemory()
        { 
            if(mVirtualAddress != MAP_FAILED)
            { munmap(mVirtualAddress, mMapSize); }
            if(mMemoryDeviceFd >= 0)
            { close(mMemoryDeviceFd); }
        }
        explicit operator bool() const noexcept { return (mVirtualAddress != MAP_FAILED); }
        inline const void* get() const noexcept { return mVirtualAddress; }
        inline void* get() noexcept { return ( !mReadOnly ? mVirtualAddress : nullptr ); }
        inline size_t size() const noexcept { return mVirtualSize; }
        inline error_t error() const noexcept { return mError; }
    };

    class SharedMemory
    {
    private:
        std::string mPath;
        int         mSharedMemoryFd;
        void*       mSharedMemoryPtr;
        size_t      mSharedMemorySize;
        error_t     mErrno;
    public:
        SharedMemory(const char* path, size_t size, bool read_only = false) 
            : mPath(path), mSharedMemoryFd(-1), mSharedMemoryPtr(MAP_FAILED), mSharedMemorySize(size), mErrno(0)
        {
            constexpr mode_t mode = S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH;
            const int oflags = ( read_only ? (O_RDONLY) : (O_RDWR | O_CREAT) );
            struct stat st;
            const bool do_truncate = ( stat(path, &st) != 0 ) && ( errno == ENOENT );
            mSharedMemoryFd = shm_open(path, oflags, mode);
            bool failure = true;
            do
            {
                if(mSharedMemoryFd >= 0)
                {
                    int page_size = getpagesize();
                    size_t remainder = mSharedMemorySize % page_size;
                    size_t sm_size = mSharedMemorySize + ( ( remainder > 0 ) ? (page_size - remainder) : (0) );
                    if( do_truncate && ( ftruncate(mSharedMemoryFd, sm_size) != 0 ) ) { break; }
                    mSharedMemoryPtr = mmap(nullptr, sm_size, PROT_READ | ( read_only ? (0) : (PROT_WRITE) ), MAP_SHARED, mSharedMemoryFd, 0);
                    if(mSharedMemoryPtr == MAP_FAILED) { break; }
                    failure = false;
                }

            } while(false);

            if( (mSharedMemoryFd >= 0) && failure )
            { close(mSharedMemoryFd); }
        }
        ~SharedMemory()
        {}

        explicit operator bool() const noexcept { return ((mSharedMemoryFd >= 0) && (mSharedMemoryPtr > 0)); }
        inline void* get() const noexcept { return mSharedMemoryPtr; }
        inline size_t size() const noexcept { return mSharedMemorySize; }
        inline int unlink()
        {
            if( ::unlink(mPath.c_str()) != 0 )
            { return -errno; }
            return 0;
        }
    };
}

#endif
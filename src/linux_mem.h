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
        void*       mVirtualAddress;
        uint32_t    mVirtualOffset;
        uint32_t    mVirtualSize;
        uint32_t    mMapSize;
        error_t     mError;
    protected:
        inline void map(uint32_t physical_addr, size_t size, bool read_only)
        {
            if(mMemoryDeviceFd < 0)
            { mMemoryDeviceFd = open("/dev/mem", O_RDWR); }

            if(mMemoryDeviceFd > 0)
            {
                uint32_t page_size = getpagesize();
                uint32_t page_mask = std::numeric_limits<uint32_t>::max() + 1 - page_size;
                uint32_t page_address = (physical_addr & page_mask);
                uint32_t addr_offset = (physical_addr - page_address);
                uint32_t map_size = ((size + addr_offset) & page_mask) + page_size;
                void* virtual_address = mmap(nullptr, map_size, ( read_only ? (PROT_READ) : (PROT_READ | PROT_WRITE) ), MAP_SHARED, mMemoryDeviceFd, page_address);
                if(virtual_address != MAP_FAILED)
                {
                    mVirtualAddress = virtual_address;
                    mVirtualOffset = addr_offset;
                    mVirtualSize = size;
                    mMapSize = map_size;
                } else {
                    mError = errno;
                }
            }
        }
        inline void unmap()
        { 
            if(mVirtualAddress != MAP_FAILED)
            { 
                munmap(mVirtualAddress, mMapSize);
                mVirtualAddress = MAP_FAILED;
                mVirtualOffset = 0;
                mVirtualSize = 0;
                mMapSize = 0;
            } 
        }
    public:
        PhysicalMemory(uint32_t physical_addr, size_t size, bool read_only = false)
            : mMemoryDeviceFd(-1), mVirtualAddress(MAP_FAILED), mVirtualOffset(0), mVirtualSize(0), mMapSize(0), mError(0) 
        { map(physical_addr, size, read_only); }
        ~PhysicalMemory() 
        { 
            unmap(); 
            if(mMemoryDeviceFd >= 0)
            { close(mMemoryDeviceFd); }
        }
        explicit operator bool() const noexcept { return (mVirtualAddress != MAP_FAILED); }
        inline void* get() const noexcept { return mVirtualAddress + mVirtualOffset; }
        inline size_t size() const noexcept { return mVirtualSize; }
        inline bool remap(uint32_t physical_addr, size_t size, bool read_only = false) { unmap(); map(physical_addr, size, read_only); }
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

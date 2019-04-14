#ifndef PHYSICAL_MEMORY_H_
#define PHYSICAL_MEMORY_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

namespace memory
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
        inline void* get() const noexcept { return mVirtualAddress; }
        inline size_t size() const noexcept { return mVirtualSize; }
        inline error_t error() const noexcept { return mError; }
    };
}

#endif
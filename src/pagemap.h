#ifndef CORE_LINUX_MEMORY_PAGEMAP_H
#define CORE_LINUX_MEMORY_PAGEMAP_H

#include <stdint.h>

namespace memory
{
    //RAII class
    class PageMap
    {
    public:
        static constexpr uint64_t INVALID_ADDRESS = -1;
        /**
         * Opens and holds pagemap during it's lifetime for the calling process
         */
        PageMap();
        virtual ~PageMap();
        /**
         * The returned physical address is the physical (CPU) mapping 
         * for the memory address given of the calling process
         */
        uint64_t virt_to_phys(uint64_t virtaddr);
    private:
        int mFd;
    };
}

#endif

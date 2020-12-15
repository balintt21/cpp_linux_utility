#include "pagemap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>

namespace memory
{

PageMap::PageMap() : mFd(-1)
{
    std::string pagemap_path = "/proc/" + std::to_string(getpid()) + "/pagemap";
    mFd = open(pagemap_path.c_str(), O_RDONLY);
}

PageMap::~PageMap()
{
    if(mFd >= 0)
    {
        close(mFd);
    }
}

uint64_t PageMap::virt_to_phys(uint64_t virtaddr)
{
    if(mFd >= 0)
    {
        int pagesize;
        uint64_t tbloff, tblen, pageaddr, physaddr;
        off_t offset;
        ssize_t nr;

        uint64_t tbl_present;
        uint64_t tbl_swapped;
        uint64_t tbl_shared;
        uint64_t tbl_pte_dirty;
        uint64_t tbl_swap_offset;
        uint64_t tbl_swap_type;

        //1PAGE = typically 4KB, 1entry = 8bytes
        pagesize = (int)sysconf(_SC_PAGESIZE);
        //see: linux/Documentation/vm/pagemap.txt
        tbloff = virtaddr / pagesize * sizeof(uint64_t);

        offset = lseek(mFd, tbloff, SEEK_SET);
        if((offset == (off_t)-1) || (offset != tbloff)) 
        { return INVALID_ADDRESS; }

        if(read(mFd, &tblen, sizeof(uint64_t)) != sizeof(uint64_t)) 
        { return INVALID_ADDRESS; }

        tbl_present   = (tblen >> 63) & 0x1;
        tbl_swapped   = (tblen >> 62) & 0x1;
        tbl_shared    = (tblen >> 61) & 0x1;
        tbl_pte_dirty = (tblen >> 55) & 0x1;
        if (!tbl_swapped) 
        { tbl_swap_offset = (tblen >> 0) & 0x7fffffffffffffULL;} 
        else 
        {
            tbl_swap_offset = (tblen >> 5) & 0x3ffffffffffffULL;
            tbl_swap_type = (tblen >> 0) & 0x1f;
        }

        pageaddr = tbl_swap_offset * pagesize;
	    physaddr = (uint64_t)pageaddr | (virtaddr & (pagesize - 1));

        if(tbl_present)
        { return physaddr; }
    }
    return INVALID_ADDRESS;
}

}

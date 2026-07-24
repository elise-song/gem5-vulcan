#include "mem/cache/cache.hh"

#ifndef __PRIVATE_CACHE_HH__
#define __PRIVATE_CACHE_HH__

namespace gem5
{
class PrivateCache : public Cache {

    void lookup (Addr vLineAddr, int pid) {
        int offset = vLineAddr & ((1 << OFFSET_BITS) - 1);
        int vPage = vLineAddr >> OFFSET_BITS;
        int pPage = pageLookup(vPage, pid);
        Addr pLineAddr = (((pPage << OFFSET_BITS) + offset) << PID_BITS) + pid;
        int tag = pLineAddr >> SET_BITS;
        int set = vLineAddr & ((1 << SET_BITS) - 1);
    }
};
}
#endif // __PRIVATE_CACHE_HH__

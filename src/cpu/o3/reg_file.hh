#ifndef __CPU_O3_REG_FILE_HH__
#define __CPU_O3_REG_FILE_HH__

#include "cpu/reg_class.hh"
#include <cstdlib>

namespace gem5
{

namespace o3
{


class RegFile
{
    int freeCount;

    void init() {
        freeCount = NUM_PREGS;
    }

    int rename(int pid, RegId areg) {
        if(freeCount == 0) return STALL;
        RegVal* preg = (RegVal*) calloc(1, sizeof(RegVal));
        freeCount--;
        map[pid][areg] = preg;
        return 0;
    }

    void free(RegVal* preg) {
        ::free(preg);
        freeCount++;
    }
};
}
}

#endif
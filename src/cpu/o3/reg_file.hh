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
    RegVal* pregs;
    RegVal* freeList[NUM_PREGS];
    int freeHead, freeTail;
    int freeCount;

    void init() {
        pregs = (RegVal*) calloc(NUM_PREGS, sizeof(RegVal));
        freeHead = 0;
        freeTail = 0;
        freeCount = NUM_PREGS;
        for (int i = 0; i < NUM_PREGS; i++) {
            freeList[i] = &pregs[i];
        }
    }

    int rename(int pid, RegId areg) {
        if(freeCount == 0) return STALL;
        RegVal* preg = freeList[freeHead];
        freeHead = (freeHead + 1) % NUM_PREGS;
        freeCount--;
        map[pid][areg] = preg;
        return 0;
    }

    void free(RegVal* preg) {
        freeList[freeTail] = preg;
        freeTail = (freeTail + 1) % NUM_PREGS;
        freeCount++;
    }
};
}
}

#endif
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
    RegVal free[NUM_PREGS];
    RegVal *freeHead, *freeTail;


    void init() {
        RegVal* pregs = calloc(NUM_PREGS, sizeof(RegVal));
        freeHead = freeTail = 0;
        for(int i = 0; i < NUM_PREGS; i++) {
            free[i] = pregs + i*sizeof(RegVal);
        }
    }

    int rename(int pid, RegId areg) {
        if(freeHead == freeTail) return STALL;
        RegVal preg = free[freeHead];
        freeHead = (freeHead + 1) % NUM_PREGS;
        map[pid][areg] = preg;
        return 0;
    }

    void free(PhysRegIdPtr preg) {
        free[freeTail] = preg;
        freeTail = (freeTail + 1) % NUM_PREGS;
    }
};
}
}

#endif
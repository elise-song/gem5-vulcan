
#example line: 
#77022567: board.cache_hierarchy.l1dcache: Block addr 
#split here: 0xcafe200 (ns) moving from  to state: 6 (E) writable: 1 readable: 1 dirty: 0 prefetched: 0 | tag: 0x32bf secure: 0 valid: 1 | set: 
#split here: 0x88 way: 0

def CheckMapping(victim_accesses, file):
    # for each addr in victim_accesses, 
    #     calculate expected set index
    #     in the debug output, check if there is a cache miss during the probe phase 
    #     increment count if the cache miss's set matches the calculated set 
    # return success rate = count / len(victim_accesses)
    total = 0
    count = 0
    with open(file, "r") as f:
        for line in f:
            if "Block addr" in line and "set:" in line:
                total += 1
                addr_split = line.split("addr")[1]
                addr_value = addr_split.split()[0]
                addr = int(addr_value, 16)
                set_index = (addr >> 6) & 0xff
                print("actual set: ", set_index)

                set_split = line.split("set:")[1]
                set_value = set_split.split()[0]
                remapped_set = int(set_value, 16)
                print("remapped set ", remapped_set)

                if remapped_set == set_index:
                    count += 1

    if total == 0:
        return 0

    return count/total


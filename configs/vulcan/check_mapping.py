
#example line: 
#77022567: board.cache_hierarchy.l1dcache: Block addr 
#split here: 0xcafe200 (ns) moving from  to state: 6 (E) writable: 1 readable: 1 dirty: 0 prefetched: 0 | tag: 0x32bf secure: 0 valid: 1 | set: 
#split here: 0x88 way: 0

def CheckMapping(victim_accesses, file, out):
    # for each addr in victim_accesses, 
    #     calculate expected set index
    #     in the debug output, check if there is a cache miss during the probe phase 
    #     increment count if the cache miss's set matches the calculated set 
    # return success rate = count / len(victim_accesses)
    if len(victim_accesses) == 0:
        return 0
    
    count = 0
    accesses = 0
    missed_addr = None

    victim_sets = {(addr >> 6) & 0xff for addr in victim_accesses}

    with open(out, "w") as w:
        w.write('[{}]\n'.format(', '.join(hex(x) for x in victim_accesses)))

        with open(file, "r") as f:
            for line in f:
                if "access for ReadReq" in line or "access for WriteReq" in line:
                    accesses += 1
                    if accesses > 513:
                        accesses = 1
                        
                    if "miss" in line:
                        #example line: access for ReadReq [2c80:2c83] miss
                        bracket_split = line.split("[")[1]
                        addr_str = bracket_split.split(":")[0]
                        missed_addr = int(addr_str, 16)
                    else:
                        missed_addr = None
                #check if miss in probe phase
                if accesses > 257 and accesses <= 513 and missed_addr is not None:
                    if "Block addr" in line and "set:" in line:
                        addr = missed_addr
                        w.write(f"missed addr: {hex(addr)}\n")
                        set_index = (addr >> 6) & 0xff
                        w.write(f"actual set: {hex(set_index)}\n")

                        set_split = line.split("set:")[1]
                        set_value = set_split.split()[0]
                        remapped_set = int(set_value, 16)
                        w.write(f"remapped set  { hex(remapped_set)}\n")

                        if remapped_set in victim_sets:
                            count += 1
                            w.write("success\n")

                        missed_addr = None

    
        w.write(f"{count/len(victim_accesses)}\n")
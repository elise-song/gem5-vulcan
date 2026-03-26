import sys
#example line: 
#77022567: board.cache_hierarchy.l1dcache: Block addr 
#split here: 0xcafe200 (ns) moving from  to state: 6 (E) writable: 1 readable: 1 dirty: 0 prefetched: 0 | tag: 0x32bf secure: 0 valid: 1 | set: 
#split here: 0x88 way: 0

def report(file, out, num_runs):
 
    index = 0
    success = 0
    cant_recover_set = 0
    all_miss = 0
    all_hit = 0
    multiple_map = 0
    other = 0

    success_addrs = []
    cant_recover_set_addrs = []
    all_miss_addrs = []
    all_hit_addrs = []
    multiple_map_addrs = []
    other_addrs = []
    

    probe_hits = []
    probe_misses = []
    match_victim_sets = []

    prime_sets = []

    victim_addrs = []
    victim_sets = []

    probe = False
    prime = True
    victim = False
    finish = False

    with open(out, "w") as w:
        with open(file, "r") as f:
            for line in f:
                if "access for" in line:
                    bracket_split = line.split("[")[1]
                    addr_str = bracket_split.split(":")[0]
                    addr = int(addr_str, 16) 
                    if prime:
                        if addr % 64 != 0 or addr // 64 > 255 :
                            victim_addrs.append(addr)
                            victim = True
                            prime = False
                    elif probe:
                        if "miss" in line:
                            probe_misses.append(addr)
                        elif "hit" in line:
                            probe_hits.append(addr)
                            if addr == 0x3fc0:
                                finish = True
                if "Block addr" in line:
                    addr_split = line.split("addr")[1]
                    addr_str = addr_split.split()[0]
                    addr = int(addr_str, 16)
                    set_split = line.split("set:")[1]
                    set_value = set_split.split()[0]
                    cache_set = int(set_value, 16)
                    if prime:
                        prime_sets.append(cache_set)
                    elif victim:
                        victim_sets.append(cache_set)
                        victim = False
                        probe = True
                    elif probe:
                        if cache_set == victim_sets[-1]:
                            match_victim_sets.append(addr)
                        if addr == 0x3fc0:
                            finish = True
                if finish:
                    finish = False
                    probe = False
                    prime = True
                    if len(probe_misses) == 1 and len(probe_hits) == 255:
                        if victim_sets[-1] == (probe_misses[0] >> 6) & 0xff:
                            success += 1
                            success_addrs.append(victim_addrs[-1])
                        elif len(match_victim_sets) > 1 :
                            multiple_map += 1
                            multiple_map_addrs.append(victim_addrs[-1])
                            w.write(f'WARNING: multiple map {hex(victim_addrs[-1])} ')
                            w.write('[{}]\n'.format(', '.join(hex(x) for x in match_victim_sets)))
                            w.write(f"#misses: {len(probe_misses)} #hits: {len(probe_hits)}\n")
                        else:
                            cant_recover_set += 1
                            cant_recover_set_addrs.append(victim_addrs[-1])

                    elif len(probe_misses) == 256:
                        all_miss += 1
                        all_miss_addrs.append(victim_addrs[-1])
                        w.write(f"{len(set(prime_sets))} sets: {sorted(set(prime_sets))}\n\n")

                    elif len(probe_hits) == 256:
                        all_hit += 1
                        all_hit_addrs.append(victim_addrs[-1])
                    else:
                        other += 1
                        other_addrs.append(victim_addrs[-1])
                        w.write(f"{len(set(prime_sets))} sets: {sorted(set(prime_sets))}\n")
                        w.write(f"#misses: {len(probe_misses)} #hits: {len(probe_hits)}\n")
                        if len(probe_misses) + len(probe_hits) != 256:
                            w.write(f"WARNING: not 256 accesses\n")

                    index += 1
                    probe_hits = []
                    probe_misses = []
                    match_victim_sets = []
                    prime_sets = []

                        
        
        w.write('[{}]\n'.format(', '.join(hex(x) for x in victim_sets)))
        w.write('[{}]\n'.format(', '.join(hex(x) for x in victim_addrs)))

        w.write(f"\nsuccess: {success}\n")
        w.write(f"cant recover set: {cant_recover_set}\n")
        w.write(f"all miss: {all_miss}\n")
        w.write(f"all hit: {all_hit}\n")
        w.write(f"multiple map: {multiple_map}\n")
        w.write(f"other: {other}\n")
        w.write(f"total: {index}\n")
        if index != int(num_runs):
            w.write(f"WARNING: bad total\n")

        w.write('\nsuccess: [{}]\n'.format(', '.join(hex(x) for x in success_addrs)))
        w.write('cant recover set: [{}]\n'.format(', '.join(hex(x) for x in cant_recover_set_addrs)))
        w.write('all miss: [{}]\n'.format(', '.join(hex(x) for x in all_miss_addrs)))
        w.write('all hit: [{}]\n'.format(', '.join(hex(x) for x in all_hit_addrs)))
        w.write('multiple map: [{}]\n'.format(', '.join(hex(x) for x in multiple_map_addrs)))
        w.write('other: [{}]\n'.format(', '.join(hex(x) for x in other_addrs)))






report(sys.argv[1], sys.argv[2], sys.argv[3])
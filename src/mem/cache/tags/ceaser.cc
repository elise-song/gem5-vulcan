#include "mem/cache/tags/ceaser.hh"
#include "mem/cache/tags/tagged_entry.hh"
#include "params/Ceaser.hh"
#include "debug/Cache.hh"

namespace gem5
{

uint32_t
Ceaser::extractSet(const KeyType &key) const
{
    DPRINTF(Cache, "CEASER\n");
    return (key.address >> setShift) & setMask;
}


Ceaser::Ceaser(const Params &p)
    : TaggedIndexingPolicy(p, p.size / p.entry_size, floorLog2(p.entry_size))
{}

std::vector<ReplaceableEntry*>
Ceaser::getPossibleEntries(const KeyType &key) const 
{
    DPRINTF(Cache, "CEASER\n");
    return sets[extractSet(key)];
}

Addr
Ceaser::regenerateAddr(const KeyType &key,
                   const ReplaceableEntry *entry) const 
{
    DPRINTF(Cache, "CEASER\n");
    return (key.address << tagShift) | (entry->getSet() << setShift);
}


} // namespace gem5



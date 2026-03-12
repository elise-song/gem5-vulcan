#include "mem/cache/tags/ceaser.hh"
#include "mem/cache/tags/tagged_entry.hh"
#include "params/Ceaser.hh"
#include "debug/Cache.hh"

namespace gem5
{
Ceaser::Ceaser(const Params &p)
    : TaggedIndexingPolicy(p, p.size / p.entry_size, floorLog2(p.entry_size))
{}

uint16_t 
Ceaser::round(const uint16_t input, const uint16_t key) const
{
    // s box
    uint16_t substituted = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t quarter = (input >> (i * 4)) & 0xF;
        substituted |= ((uint16_t)sbox[quarter]) << (i * 4);
    }

    // p box
    uint16_t permuted = 0;
    for (int i = 0; i < 16; i++) {
        int src = (i * 4 + i / 4) % 16;
        uint16_t bit = (substituted >> src) & 1;
        permuted |= bit << i;
    }

    // xor with key
    return permuted ^ key;
}

Addr 
Ceaser::encrypt(const Addr addr) const
{
    DPRINTF(Cache, "CEASER B4 ENCRYPT %llx\n", addr);
    uint16_t left0 = bits<Addr>(addr, 15, 0);
    uint16_t right0 = bits<Addr>(addr, 31, 16);
    // first stage
    uint16_t right1 = left0;
    uint16_t left1 = round(left0, ceaser_key[0]) ^ right0;
    // second stage
    uint16_t right2 = left1;
    uint16_t left2 = round(left1, ceaser_key[1]) ^ right1;
    // third stage
    uint16_t right3 = left2;
    uint16_t left3 = round(left2, ceaser_key[2]) ^ right2;
    // fourth stage
    uint16_t right4 = left3;
    uint16_t left4 = round(left3, ceaser_key[3]) ^ right3;
    // concat left' and right'
    Addr encrypted_addr = ((Addr)left4 << 16) | right4;
    DPRINTF(Cache, "CEASER ENCRYPT %llx\n", encrypted_addr);
    return addr;
}

uint32_t
Ceaser::extractSet(const KeyType &key) const
{
    return (encrypt(key.address) >> setShift) & setMask;
}


std::vector<ReplaceableEntry*>
Ceaser::getPossibleEntries(const KeyType &key) const 
{
    DPRINTF(Cache, "CEASER getPossibleEntries %llx\n", key.address);
    return sets[extractSet(key)];
}

Addr
Ceaser::regenerateAddr(const KeyType &key,
                   const ReplaceableEntry *entry) const 
{
    DPRINTF(Cache, "CEASER regenerateAddr key.address %llx\n", key.address);
    Addr regenerated =  (key.address << tagShift) | (entry->getSet() << setShift);
    DPRINTF(Cache, "CEASER regenerateAddr regenerated%llx\n", regenerated);

}


} // namespace gem5



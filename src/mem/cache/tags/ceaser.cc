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
    return input ^ key;
}

uint32_t 
Ceaser::encrypt(const uint32_t line_addr) const
{
    // line address is 30-6 = 24 bits
    uint16_t left = bits<Addr>(line_addr, 23, 12);
    uint16_t right = bits<Addr>(line_addr, 11, 0);
    // DPRINTF(Cache, "CEASER: left= %x right= %x\n",  left, right);
    uint16_t temp;
    for (int i = 0; i < 4; i++) {
        temp = left;
        left = round(left, ceaser_key[i]) ^ right;
        right = temp;
        // DPRINTF(Cache, "CEASER round %d: left= %x right= %x\n", i, left, right);
    }
    // concat left' and right'
    Addr encrypted_addr = (left << 12) | right;
    DPRINTF(Cache, "CEASER encrypt %llx\n", encrypted_addr);
    return encrypted_addr;
}

uint32_t 
Ceaser::decrypt(const uint32_t line_addr) const
{
    // line address is 30-6 = 24 bits
    uint16_t left = bits<Addr>(line_addr, 23, 12);
    uint16_t right = bits<Addr>(line_addr, 11, 0);
    uint16_t temp;
    // DPRINTF(Cache, "CEASER : left= %x right= %x\n", left, right);

    for (int i = 3; i >=0; i--) {
        temp = right;
        right = round(right, ceaser_key[i]) ^ left;
        left = temp;
        // DPRINTF(Cache, "CEASER round %d: left= %x right= %x\n", i, left, right);

    }
    // concat left' and right'
    Addr decrypted_addr = (left << 12) | right;
    return decrypted_addr;
}
uint32_t previous_addr = 0;

uint32_t
Ceaser::extractSet(const KeyType &key) const
{
    // set shift = # offset bits
    // set mask is applied after removing offset
    uint32_t set = (encrypt(key.address >> setShift)) & setMask; 
    DPRINTF(Cache, "CEASER extractSet %llx\n", set);

    return set;
}


std::vector<ReplaceableEntry*>
Ceaser::getPossibleEntries(const KeyType &key) const 
{
    std::vector<ReplaceableEntry*> entries = sets[extractSet(key)];
    for (auto entry : entries) {
        entry->setDecryptSet((key.address >> setShift) & setMask);
    }
    return entries;
}

Addr
Ceaser::regenerateAddr(const KeyType &key,
                   const ReplaceableEntry *entry) const 
{
    DPRINTF(Cache, "CEASER regenerateAddr decrypt set %llx\n", entry->getDecryptSet());
    Addr regenerated =  (key.address << tagShift) | (entry->getDecryptSet() << setShift);
    DPRINTF(Cache, "CEASER regenerateAddr %llx\n", regenerated);
    return regenerated;

}


} // namespace gem5



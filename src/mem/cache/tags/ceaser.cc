#include "mem/cache/tags/ceaser.hh"
#include "mem/cache/tags/tagged_entry.hh"
#include "params/Ceaser.hh"
#include "debug/Ceaser.hh"
#include <random>
#include <cmath>

namespace gem5
{
Ceaser::Ceaser(const Params &p)
    : TaggedIndexingPolicy(p, p.size / p.entry_size, floorLog2(p.entry_size))
{
    // set random sbox and pbox
    // std::random_device entropy_source;
	// std::mt19937 generator(entropy_source()); 
    // // generate 24 bit number 
	// std::uniform_real_distribution<double> dist(0, std::pow(2, 24));
    // for (int i = 0; i < 12; i++){
    //     sbox[i] = uint32_t(dist(generator)) & 0xffffff;
    // }
    // std::shuffle(pbox.begin(), pbox.end(), generator);

    // pbox = {9, 4, 11, 10, 6, 0, 1, 7, 3, 8, 5, 2};
}

uint16_t
Ceaser::substitute(const uint32_t input) const
{
    uint16_t output = 0;
    for (auto s : sbox){
        uint8_t bit = 0;
        for (int i = 0; i < 24; i++){
            if ((s >> i) & 1){
                bit = bit ^ ((input >> i) & 1);
            }
        }
        output = (output << 1) | bit;
    }
    return output;
}

uint16_t
Ceaser::permutate(const uint16_t input) const
{
    int i = 0;
    uint16_t output = 0;
    for (auto p : pbox) {
        uint16_t bit = ((input >> i) & 1) << p;
        output = output | bit;
        i++;
    }
    return output;
}

uint16_t 
Ceaser::round(const uint16_t input, const uint16_t key) const
{
    uint32_t concat = input << 12 | key;
    DPRINTF(Ceaser, "round concat %llx\n", concat);
    uint16_t sub = substitute(concat);
    DPRINTF(Ceaser, "round sub %llx\n", sub);
    uint16_t per = permutate(sub);
    DPRINTF(Ceaser, "round per %llx\n", per);

    return per;
}

uint32_t 
Ceaser::encrypt(const uint32_t line_addr) const
{
    // line address is 30-6 = 24 bits
    uint16_t left = bits<Addr>(line_addr, 23, 12);
    uint16_t right = bits<Addr>(line_addr, 11, 0);
    DPRINTF(Ceaser, "left= %x right= %x\n",  left, right);
    uint16_t temp;
    for (int i = 0; i < 4; i++) {
        temp = left;
        left = round(left, ceaser_key[i]) ^ right;
        right = temp;
        DPRINTF(Ceaser, "round %d: left= %x right= %x\n", i, left, right);
    }
    // concat left' and right'
    Addr encrypted_addr = (left << 12) | right;
    DPRINTF(Ceaser, "encrypt %llx\n", encrypted_addr);
    return encrypted_addr;
}

uint32_t 
Ceaser::decrypt(const uint32_t line_addr) const
{
    // line address is 30-6 = 24 bits
    uint16_t left = bits<Addr>(line_addr, 23, 12);
    uint16_t right = bits<Addr>(line_addr, 11, 0);
    uint16_t temp;
    DPRINTF(Ceaser, "left= %x right= %x\n", left, right);

    for (int i = 3; i >=0; i--) {
        temp = right;
        right = round(right, ceaser_key[i]) ^ left;
        left = temp;
        DPRINTF(Ceaser, "round %d: left= %x right= %x\n", i, left, right);

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
    DPRINTF(Ceaser, "extractSet %llx\n", set);

    return set;
}


std::vector<ReplaceableEntry*>
Ceaser::getPossibleEntries(const KeyType &key) const 
{
    std::vector<ReplaceableEntry*> entries = sets[extractSet(key)];
    DPRINTF(Ceaser, "getPossibleEntries %d\n", entries.size());
    // only works for direct mapped
    for (auto entry : entries) {
        entry->setDecryptSet((key.address >> setShift) & setMask);
    }
    return entries;
}

Addr
Ceaser::regenerateAddr(const KeyType &key,
                   const ReplaceableEntry *entry) const 
{
    DPRINTF(Ceaser, "regenerateAddr decrypt set %llx\n", entry->getDecryptSet());
    Addr regenerated =  (key.address << tagShift) | (entry->getDecryptSet() << setShift);
    DPRINTF(Ceaser, "regenerateAddr %llx\n", regenerated);
    return regenerated;

}


} // namespace gem5



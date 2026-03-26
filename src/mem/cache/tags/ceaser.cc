#include "mem/cache/tags/ceaser.hh"
#include "mem/cache/tags/tagged_entry.hh"
#include "params/Ceaser.hh"
#include "debug/Ceaser.hh"
#include "debug/Cache.hh"
#include <random>
#include <cmath>

namespace gem5
{
Ceaser::Ceaser(const Params &p)
  : TaggedIndexingPolicy(p, p.size / p.entry_size, floorLog2(p.entry_size)),
    tagShift(floorLog2(p.entry_size))
{
    // set random sbox and pbox
    std::random_device entropy_source;
	std::mt19937 generator(entropy_source()); 
    // generate 24 bit number 
	std::uniform_int_distribution<uint32_t> dist32(0, 0xFFFFFF);  // 24-bit
    std::uniform_int_distribution<uint16_t> dist16(0, 0xFFF);     // 12-bit

    for (int i = 0; i < num_stages; i++){
        ceaser_key[i] = dist16(generator);
        // for (int j = 0; j < ceaser_size; j++){
        //     sbox[i][j] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23};
        //     std::shuffle(sbox[i][j].begin(), sbox[i][j].end(), generator);
        //     sbox[i][j].erase(sbox[i][j].begin() + ceaser_size, sbox[i][j].end());
        // }
        // pbox[i] = {0,1,2,3,4,5,6,7,8,9,10,11};
        // std::shuffle(pbox[i].begin(), pbox[i].end(), generator);
    }

    // Build string for sbox and pbox
    std::stringstream ss;
    ss << "S-box:\n";
    for (int i = 0; i < num_stages; i++) {
        for (int j = 0; j < ceaser_size; j++) {
            ss << "{";
            for (auto val : sbox[i][j]) {
                ss << val << ", ";
            }
            ss << "}, ";
        }
        ss << "\n";

    }
    
    ss << "\nP-box: \n";
    for (int i = 0; i < num_stages; i++) {
        for (auto val : pbox[i]) {
            ss << val << ", ";
        }
        ss << "\n";
    }
    DPRINTF(Cache, "%s", ss.str().c_str());
}

uint16_t
Ceaser::substitute(const uint32_t input, int stage) const
{
    uint16_t output = 0;
    for (auto s : sbox[stage]){
        uint8_t bit = 0;
        for (auto pos : s){
            bit = bit ^ ((input >> pos) & 1);
        }
        output = (output << 1) | bit;
    }
    return output;
}

uint16_t
Ceaser::permutate(const uint16_t input, int stage) const
{
    int i = 0;
    uint16_t output = 0;
    for (auto p : pbox[stage]) {
        uint16_t bit = ((input >> i) & 1) << p;
        output = output | bit;
        i++;
    }
    return output;
}

uint16_t 
Ceaser::round(const uint16_t input, int stage) const
{
    uint32_t concat = (input << ceaser_size) | ceaser_key[stage];
    DPRINTF(Ceaser, "round concat %llx\n", concat);
    uint16_t sub = substitute(concat, stage);
    DPRINTF(Ceaser, "round sub %llx\n", sub);
    uint16_t per = permutate(sub, stage);
    DPRINTF(Ceaser, "round per %llx\n", per);

    return per;
}

uint32_t 
Ceaser::encrypt(const uint32_t line_addr) const
{
    // line address is 30-6 = 24 bits
    uint16_t left = bits<Addr>(line_addr, ceaser_size * 2 - 1, ceaser_size);
    uint16_t right = bits<Addr>(line_addr, ceaser_size - 1, 0);
    DPRINTF(Ceaser, "left= %x right= %x\n",  left, right);
    uint16_t temp;
    for (int i = 0; i < num_stages; i++) {
        temp = right;
        right = round(left, i) ^ right;
        left = temp;
        DPRINTF(Ceaser, "round %d: left= %x right= %x\n", i, left, right);
    }
    // concat left' and right'
    Addr encrypted_addr = (left << ceaser_size) | right;
    DPRINTF(Ceaser, "encrypt %llx\n", encrypted_addr);
    return encrypted_addr;
}

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
    return entries;
}

Addr
Ceaser::regenerateAddr(const KeyType &key,
                   const ReplaceableEntry *entry) const 
{
    Addr regenerated =  (key.address << tagShift); 
    DPRINTF(Ceaser, "regenerateAddr %llx\n", regenerated);
    return regenerated;

}

Addr
Ceaser::extractTag(const Addr addr) const
{
    return (addr >> tagShift);
}


} // namespace gem5



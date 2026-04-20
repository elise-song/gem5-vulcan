#include "mem/cache/tags/ceaser.hh"
#include "mem/cache/tags/tagged_entry.hh"
#include "params/Ceaser.hh"
#include "debug/Ceaser.hh"
#include "debug/Cache.hh"
#include "mem/cache/cache_blk.hh"
#include <random>
#include <cmath>

namespace gem5
{
Ceaser::Ceaser(const Params &p)
  : TaggedIndexingPolicy(p, p.size / p.entry_size, floorLog2(p.entry_size)),
    tagShift(floorLog2(p.entry_size))
{
    set random sbox and pbox
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
Ceaser::round(const uint16_t input, int stage, const uint16_t key[4]) const
{
    uint32_t concat = (input << ceaser_size) | key[stage];
    DPRINTF(Ceaser, "round concat %llx\n", concat);
    uint16_t sub = substitute(concat, stage);
    DPRINTF(Ceaser, "round sub %llx\n", sub);
    uint16_t per = permutate(sub, stage);
    DPRINTF(Ceaser, "round per %llx\n", per);
    return per;
}

uint32_t 
Ceaser::encrypt(const uint32_t line_addr, const uint16_t key[4]) const
{
    // line address is 30-6 = 24 bits
    uint16_t left = bits<Addr>(line_addr, ceaser_size * 2 - 1, ceaser_size);
    uint16_t right = bits<Addr>(line_addr, ceaser_size - 1, 0);
    DPRINTF(Ceaser, "left= %x right= %x\n",  left, right);
    uint16_t temp;
    for (int i = 0; i < 4; i++) {
        temp = left;
        left = round(left, i, key) ^ right;
        right = temp;
        DPRINTF(Ceaser, "round %d: left= %x right= %x\n", i, left, right);
    }
    // concat left' and right'
    Addr encrypted_addr = (left << 12) | right;
    DPRINTF(Ceaser, "encrypt %llx\n", encrypted_addr);
    return encrypted_addr;
}

uint32_t 
Ceaser::decrypt(const uint32_t line_addr, const uint16_t key[4]) const
{
    // line address is 30-6 = 24 bits
    uint16_t left = bits<Addr>(line_addr, 23, 12);
    uint16_t right = bits<Addr>(line_addr, 11, 0);
    uint16_t temp;
    DPRINTF(Ceaser, "left= %x right= %x\n", left, right);

    for (int i = 3; i >=0; i--) {
        temp = right;
        right = round(right, i, key) ^ left;
        left = temp;
        DPRINTF(Ceaser, "round %d: left= %x right= %x\n", i, left, right);
    }
    // concat left' and right'
    Addr encrypted_addr = (left << ceaser_size) | right;
    DPRINTF(Ceaser, "encrypt %llx\n", encrypted_addr);
    return encrypted_addr;
}

//uint32_t previous_addr = 0;

uint32_t
Ceaser::extractSet(const KeyType &key) const
{
    // set shift = # offset bits
    // set mask is applied after removing offset
    //uint32_t set = (encrypt(key.address >> setShift)) & setMask; 
    uint32_t addr = key.address >> setShift;
    //printf("currkey %x %x %x %x\n", ceaser_key[0], ceaser_key[1], ceaser_key[2], ceaser_key[3]);
    //printf("nextkey %x %x %x %x\n", nextKey[0], nextKey[1], nextKey[2], nextKey[3]);
    uint32_t currSet = (encrypt(addr, ceaser_key)) & setMask;
    uint32_t nextSet = (encrypt(addr, nextKey)) & setMask;
    DPRINTF(Ceaser, "extractSet %llx\n", currSet);
    //printf("sptr %d\n", SPtr);
    //printf("currset %d\n", currSet);
    //printf("nextset %d\n", nextSet);
    //printf("extractSet: addr=%x currSet=%d nextSet=%d SPtr=%d\n", addr, currSet, nextSet, SPtr);
    if (currSet <= SPtr) { 
        return nextSet;
    }
    return currSet;
}


std::vector<ReplaceableEntry*>
Ceaser::getPossibleEntries(const KeyType &key) const 
{
    DPRINTF(Ceaser, "getPossibleEntries key.address=%llx setShift=%d addr>>setShift=%llx\n",
        key.address, setShift, key.address >> setShift);
    access();
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

void
Ceaser::access() const{
    std::random_device entropy_source;
	std::mt19937 generator(entropy_source()); 
    // generate 24 bit number 
	std::uniform_int_distribution<uint32_t> dist32(0, 0xFFFFFF);  // 24-bit
    std::uniform_int_distribution<uint16_t> dist16(0, 0xFFF);     // 12-bit
    ACtr++;
    if (ACtr >= APLR){
        remap(SPtr);
        SPtr++;
        ACtr= 0;
        if (SPtr == numSets){
            printf("reached generate new key\n");
            for (int i = 0; i < 4; i++) {
                ceaser_key[i] = nextKey[i];
            }
            for (int i = 0; i < 4; i++) {
                nextKey[i] = dist16(generator);
            }
            for (int s = 0; s < numSets; s++) {
                for (auto entry : sets[s]) {
                    auto tagged = dynamic_cast<TaggedEntry*>(entry);
                    if (!tagged) {
                        panic("bad\n");
                    }
                    if (tagged->isValid())
                        tagged->setEID(0);
                }
            }
            SPtr = 0;
        }
    }
}

Addr
Ceaser::extractTag(const Addr addr) const
{
    return (addr >> tagShift);
}

void
Ceaser::remap(uint32_t set) const
{
    auto &entries = sets[set];
    for (auto entry: entries) {
        auto tagged = dynamic_cast<TaggedEntry*>(entry);
        if (!tagged) {
            panic("bad\n");
        }
        if (!tagged->isValid())
            continue;

        //skip lines that are already remapped
        if (tagged->getEID() == 1)
            continue;

        auto block = dynamic_cast<CacheBlk*>(entry);
        if (!block)
            panic("bad\n");
    
        //DEBUGGING PURPOSES
        Addr tag = tagged->getTag();      
        uint32_t numSetBits = floorLog2(numSets);  
        Addr enc_line_addr = (tag << numSetBits) | set;  
        Addr original_line_addr = decrypt(enc_line_addr, ceaser_key);
        uint32_t updated = encrypt(original_line_addr, nextKey);
        uint32_t new_set = updated & setMask;
        Addr new_tag = updated >> numSetBits; 
        KeyType verifyKey = {original_line_addr << setShift, false};
        uint32_t extractedSet = extractSet(verifyKey);
        printf("remap set=%d old_tag=%lx new_tag=%lx new_set=%d extractSet=%d match=%d\n", 
            set, tag, new_tag, new_set, extractedSet, extractedSet == new_set);
        fflush(stdout);

        if (block->isSet(CacheBlk::DirtyBit)){
            printf("dirty\n");
            tagged->setEID(1);
            continue;
            printf("remap: invalidating set=%d tag=%lx\n", set, tagged->getTag());

        block->invalidate();
        }    
    }        
}
}
// namespace gem5



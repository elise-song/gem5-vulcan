#ifndef __MEM_CACHE_TAGS_CEASER_HH__
#define __MEM_CACHE_TAGS_CEASER_HH__

#include "mem/cache/tags/tagged_entry.hh"
#include "params/Ceaser.hh"
#include "debug/Cache.hh"

namespace gem5
{

class Ceaser : public TaggedIndexingPolicy
{
  protected:
    virtual uint32_t
    extractSet(const KeyType &key) const;
  
    private:
      uint16_t ceaser_key[4] = {0xE, 0xEE, 0xEEE, 0xEEEE};
       // s box
      uint8_t sbox[16] = {
        0x0, 0x1, 0x2, 0x3,   
        0x4, 0x5, 0x6, 0x7,   
        0x8, 0x9, 0xA, 0xB,   
        0xC, 0xD, 0xE, 0xF    
      };

      Addr
      encrypt(const Addr addr) const;

      uint16_t
      round(const uint16_t input, const uint16_t key) const;

  public:
    PARAMS(Ceaser);
    Ceaser(const Params &p);

    

    std::vector<ReplaceableEntry*>
    getPossibleEntries(const KeyType &key) const override;

    Addr
    regenerateAddr(const KeyType &key,
                   const ReplaceableEntry *entry) const override;
};

} // namespace gem5


#endif // __MEM_CACHE_TAGS_CEASER_HH__

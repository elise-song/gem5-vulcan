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

      uint16_t ceaser_key[4] = {0x325, 0x7AB, 0xC1D, 0x9E2};

      uint32_t 
      encrypt(const uint32_t line_addr) const;
      
      uint32_t 
      decrypt(const uint32_t line_addr) const;

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

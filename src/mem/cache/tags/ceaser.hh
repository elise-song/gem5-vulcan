#ifndef __MEM_CACHE_TAGS_CEASER_HH__
#define __MEM_CACHE_TAGS_CEASER_HH__

#include "mem/cache/tags/tagged_entry.hh"
#include "params/Ceaser.hh"
#include "debug/Cache.hh"
#include <vector>
#include <string>

namespace gem5
{

class Ceaser : public TaggedIndexingPolicy
{
  protected:
    virtual uint32_t
    extractSet(const KeyType &key) const;
  
    private:
      const static int ceaser_size = 16;

      int num_stages = 4;

      uint16_t ceaser_key[4];

      std::vector<int> sbox[4][ceaser_size]; 
      

      std::vector<int> pbox[4];

      const std::string boxFile;
     
      uint32_t 
      encrypt(const uint32_t line_addr) const;

      uint16_t
      round(const uint16_t input, int stage) const;

      uint16_t
      substitute(const uint32_t input, int stage) const;

      uint16_t
      permutate(const uint16_t input, int stage) const;

      void 
      parseBoxFile(const std::string &path);


  public:
    const int tagShift;
    PARAMS(Ceaser);
    Ceaser(const Params &p);

    

    std::vector<ReplaceableEntry*>
    getPossibleEntries(const KeyType &key) const override;

    Addr
    regenerateAddr(const KeyType &key,
                   const ReplaceableEntry *entry) const override;

    virtual Addr
    extractTag(const Addr addr) const override;
};

} // namespace gem5


#endif // __MEM_CACHE_TAGS_CEASER_HH__

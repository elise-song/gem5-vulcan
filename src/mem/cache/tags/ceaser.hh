#ifndef __MEM_CACHE_TAGS_CEASER_HH__
#define __MEM_CACHE_TAGS_CEASER_HH__

#include "mem/cache/tags/tagged_entry.hh"
#include "params/Ceaser.hh"
#include "debug/Cache.hh"
#include <vector>

namespace gem5
{

class Ceaser : public TaggedIndexingPolicy
{
  protected:
    virtual uint32_t
    extractSet(const KeyType &key) const;
  
    private:
      int ceaser_size = 12;

      uint16_t ceaser_key[4] = {0x325, 0x7AB, 0xC1D, 0x9E2};

      uint32_t sbox[12] = {0x9d0222, 0x18e7d1, 0x6a3b7e, 0x3f7fdd, 0xc6ff21, 0xe2b187, 0xbda80f, 0x516694, 0x42e5cf, 0xc16ba5, 0x96215b, 0x625894};

      std::vector<int> pbox = {9, 4, 11, 10, 6, 0, 1, 7, 3, 8, 5, 2};
      // uint16_t ceaser_key[4] = {0x1A35, 0x0D7B, 0x1F4C, 0x0B96};
      // uint32_t sbox[13] = {0x2a3f512, 0x17c84be, 0x3d91e6f, 0x24b2c83, 0x1e57a91, 0x32f0dbc, 0x081c6e4, 0x25d3a70, 0x3f428b5, 0x1039e2d, 0x2c71bf6, 0x16e94a8, 0x3b5d027};
      // std::vector<int> pbox = {7, 2, 10, 5, 0, 12, 9, 3, 11, 6, 1, 8, 4};

      uint32_t 
      encrypt(const uint32_t line_addr) const;
      
      uint32_t 
      decrypt(const uint32_t line_addr) const;

      uint16_t
      round(const uint16_t input, const uint16_t key) const;

      uint16_t
      substitute(const uint32_t input) const;

      uint16_t
      permutate(const uint16_t input) const;

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

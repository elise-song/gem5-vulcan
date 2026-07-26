#include "mem/cache/tags/ceaser.hh"
#include "mem/cache/tags/tagged_entry.hh"
#include "params/Ceaser.hh"
#include "debug/Ceaser.hh"
#include "debug/Cache.hh"
#include <random>
#include <cmath>
#include <fstream>
#include <sstream>
#include <cctype>

namespace gem5
{
Ceaser::Ceaser(const Params &p)
  : TaggedIndexingPolicy(p, p.size / p.entry_size, floorLog2(p.entry_size)),
    boxFile(p.box_file),
    tagShift(floorLog2(p.entry_size))
{
    // set random key
    std::random_device entropy_source;
    std::mt19937 generator(entropy_source());
    // generate 24 bit number
    std::uniform_int_distribution<uint32_t> dist32(0, 0xFFFFFFFF); // 32-bit
    std::uniform_int_distribution<uint16_t> dist16(0, 0xFFFF);     // 16-bit

    if (!boxFile.empty()) {
        parseBoxFile(boxFile);
    }
    // generate random sbox and pbox
    for (int i = 0; i < num_stages; i++){
        ceaser_key[i] = dist16(generator);
        if (boxFile.empty()) {
            for (int j = 0; j < ceaser_size; j++) {
                sbox[i][j] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                              11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                              22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
                std::shuffle(sbox[i][j].begin(), sbox[i][j].end(), generator);
                sbox[i][j].erase(sbox[i][j].begin() + ceaser_size,
                                 sbox[i][j].end());
            }
            pbox[i] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
            std::shuffle(pbox[i].begin(), pbox[i].end(), generator);
        }
    }

    // Precompute permutate()'s rearrangement according to the fixed pbox, so
    // it becomes a single array lookup instead of a 16 iteration loop.
    for (int s = 0; s < num_stages; s++) {
        permuteLUT[s].resize(1u << ceaser_size);
        for (uint32_t in = 0; in < (1u << ceaser_size); in++) {
            uint16_t output = 0;
            int i = 0;
            for (auto p : pbox[s]) {
                uint16_t bit = ((in >> i) & 1) << p;
                output = output | bit;
                i++;
            }
            permuteLUT[s][in] = output;
        }
    }

    // Build string for sbox and pbox
    std::stringstream ss;
    ss << "\nS-box:\n{";
    for (int i = 0; i < num_stages; i++) {
        ss << "{";
        for (int j = 0; j < ceaser_size; j++) {
            ss << "{";
            for (auto val : sbox[i][j]) {
                ss << val << ", ";
            }
            ss << "}, ";
        }
        ss << "}, ";
        ss << "\n";
    }
    ss << "}\n\n";
    ss << "P-box: \n{";
    for (int i = 0; i < num_stages; i++) {
        ss << "{";
        for (auto val : pbox[i]) {
            ss << val << ", ";
        }
        ss << "}, ";
        ss << "\n";
    }
    ss << "}\n";
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
    return permuteLUT[stage][input];
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

std::vector<ReplaceableEntry *>
Ceaser::getPossibleEntries(const KeyType &key) const
{
    std::vector<ReplaceableEntry*> entries = sets[extractSet(key)];
    DPRINTF(Ceaser, "getPossibleEntries %d\n", entries.size());
    return entries;
}

Addr
Ceaser::regenerateAddr(const KeyType &key, const ReplaceableEntry *entry) const
{
    Addr regenerated = (key.address << tagShift);
    DPRINTF(Ceaser, "regenerateAddr %llx\n", regenerated);
    return regenerated;

}

Addr
Ceaser::extractTag(const Addr addr) const
{
    return (addr >> tagShift);
}

// Return the index just past the closing '}' that matches the '{' at `start`.
static std::size_t findMatchingBrace(const std::string &s, std::size_t start)
{
    int depth = 0;
    for (std::size_t i = start; i < s.size(); i++) {
        if      (s[i] == '{') depth++;
        else if (s[i] == '}') { if (--depth == 0) return i + 1; }
    }
    return std::string::npos;
}

// Parse a flat "{v, v, v, ...}" string (no nested braces) into a vector<int>.
static std::vector<int> parseLeafList(const std::string &s)
{
    std::vector<int> vals;
    std::size_t i = 0;
    while (i < s.size() && s[i] != '{') i++;
    i++; // consume '{'
    std::string cur;
    while (i < s.size() && s[i] != '}') {
        char c = s[i++];
        if (c == ',') {
            std::string t;
            for (char x : cur)
                if (!std::isspace(static_cast<unsigned char>(x))) t += x;
            if (!t.empty()) {
                try { vals.push_back(std::stoi(t)); } catch (...) {}
            }
            cur.clear();
        } else {
            cur += c;
        }
    }
    // trailing value before '}'
    std::string t;
    for (char x : cur)
        if (!std::isspace(static_cast<unsigned char>(x))) t += x;
    if (!t.empty()) {
        try { vals.push_back(std::stoi(t)); } catch (...) {}
    }
    return vals;
}

// Recursively collect every innermost (leaf) {…} block inside `s`.
static void collectLeafLists(const std::string &s,
                              std::vector<std::vector<int>> &out)
{
    std::size_t pos = 0;
    while (pos < s.size()) {
        std::size_t open = s.find('{', pos);
        if (open == std::string::npos) break;

        std::size_t close = findMatchingBrace(s, open);
        if (close == std::string::npos) break;

        std::string block = s.substr(open, close - open); // includes braces

        // Leaf block: no nested '{' inside the outer braces
        if (block.find('{', 1) == std::string::npos) {
            out.push_back(parseLeafList(block));
        } else {
            // Recurse into the contents (strip outer braces)
            collectLeafLists(block.substr(1, block.size() - 2), out);
        }
        pos = close;
    }
}

void
Ceaser::parseBoxFile(const std::string &path)
{
    std::ifstream f(path);
    fatal_if(!f.is_open(),
        "Ceaser: cannot open box file '%s'", path.c_str());

    // Slurp the whole file into one string
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    // ---- locate section boundaries ----------------------------------------
    std::size_t sboxPos = content.find("S-box:");
    std::size_t pboxPos = content.find("P-box:");
    fatal_if(sboxPos == std::string::npos,
        "Ceaser: 'S-box:' not found in '%s'", path.c_str());
    fatal_if(pboxPos == std::string::npos,
        "Ceaser: 'P-box:' not found in '%s'", path.c_str());

    std::string sboxSection = content.substr(sboxPos + 6,
                                             pboxPos - sboxPos - 6);
    std::string pboxSection = content.substr(pboxPos + 6);

    // ---- parse S-box -------------------------------------------------------
    // Expect num_stages * ceaser_size leaf lists in row-major order.
    std::vector<std::vector<int>> sboxLeaves;
    collectLeafLists(sboxSection, sboxLeaves);

    fatal_if((int)sboxLeaves.size() != num_stages * ceaser_size,
        "Ceaser: expected %d sbox leaf lists, got %zu, in '%s'",
        num_stages * ceaser_size, sboxLeaves.size(), path.c_str());

    for (int i = 0; i < num_stages; i++)
        for (int j = 0; j < ceaser_size; j++)
            sbox[i][j] = sboxLeaves[i * ceaser_size + j];

    // ---- parse P-box -------------------------------------------------------
    // Expect num_stages leaf lists, one per stage.
    std::vector<std::vector<int>> pboxLeaves;
    collectLeafLists(pboxSection, pboxLeaves);

    fatal_if((int)pboxLeaves.size() != num_stages,
        "Ceaser: expected %d pbox leaf lists, got %zu, in '%s'",
        num_stages, pboxLeaves.size(), path.c_str());

    for (int i = 0; i < num_stages; i++)
        pbox[i] = pboxLeaves[i];

    // ---- sanity check ------------------------------------------------------
    for (int i = 0; i < num_stages; i++) {
        for (int j = 0; j < ceaser_size; j++) {
            fatal_if(sbox[i][j].empty(),
                "Ceaser: sbox[%d][%d] empty after parsing '%s'",
                i, j, path.c_str());
        }
        fatal_if(pbox[i].empty(),
            "Ceaser: pbox[%d] empty after parsing '%s'",
            i, path.c_str());
    }

    DPRINTF(Cache, "Ceaser: loaded S-box/P-box from '%s'\n", path.c_str());
}

} // namespace gem5

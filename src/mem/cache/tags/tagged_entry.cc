/*
 * Copyright (c) 2018 Inria
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * Definitions of the tagged set associative indexing policy, including the
 * optional keyed set index.
 */

#include "mem/cache/tags/tagged_entry.hh"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <random>
#include <sstream>

#include "base/bitfield.hh"
#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/Cache.hh"
#include "debug/KeyedIndex.hh"
#include "params/TaggedSetAssociative.hh"

namespace gem5
{
TaggedSetAssociative::TaggedSetAssociative(const Params &p)
  : TaggedIndexingPolicy(p, p.size / p.entry_size, floorLog2(p.entry_size)),
    keyedIndex(p.keyed_index),
    boxFile(p.box_file)
{
    if (!keyedIndex) {
        return;
    }

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
    for (int i = 0; i < numRounds; i++){
        roundKeys[i] = dist16(generator);
        if (boxFile.empty()) {
            for (int j = 0; j < halfWidth; j++) {
                sbox[i][j] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                              11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                              22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
                std::shuffle(sbox[i][j].begin(), sbox[i][j].end(), generator);
                sbox[i][j].erase(sbox[i][j].begin() + halfWidth,
                                 sbox[i][j].end());
            }
            pbox[i] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
            std::shuffle(pbox[i].begin(), pbox[i].end(), generator);
        }
    }

    // Build string for sbox and pbox
    std::stringstream ss;
    ss << "\nS-box:\n{";
    for (int i = 0; i < numRounds; i++) {
        ss << "{";
        for (int j = 0; j < halfWidth; j++) {
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
    for (int i = 0; i < numRounds; i++) {
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
TaggedSetAssociative::substitute(const uint32_t input, int stage) const
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
TaggedSetAssociative::permutate(const uint16_t input, int stage) const
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
TaggedSetAssociative::round(const uint16_t input, int stage) const
{
    uint32_t concat = (input << halfWidth) | roundKeys[stage];
    DPRINTF(KeyedIndex, "round concat %llx\n", concat);
    uint16_t sub = substitute(concat, stage);
    DPRINTF(KeyedIndex, "round sub %llx\n", sub);
    uint16_t per = permutate(sub, stage);
    DPRINTF(KeyedIndex, "round per %llx\n", per);

    return per;
}

uint32_t
TaggedSetAssociative::encrypt(const uint32_t line_addr) const
{
    // line address is 30-6 = 24 bits
    uint16_t left = bits<Addr>(line_addr, halfWidth * 2 - 1, halfWidth);
    uint16_t right = bits<Addr>(line_addr, halfWidth - 1, 0);
    DPRINTF(KeyedIndex, "left= %x right= %x\n",  left, right);
    uint16_t temp;
    for (int i = 0; i < numRounds; i++) {
        temp = right;
        right = round(left, i) ^ right;
        left = temp;
        DPRINTF(KeyedIndex, "round %d: left= %x right= %x\n", i, left, right);
    }
    // concat left' and right'
    Addr encrypted_addr = (left << halfWidth) | right;
    DPRINTF(KeyedIndex, "encrypt %llx\n", encrypted_addr);
    return encrypted_addr;
}

uint32_t
TaggedSetAssociative::extractSet(const KeyType &key) const
{
    if (!keyedIndex) {
        return (key.address >> setShift) & setMask;
    }
    // set shift = # offset bits
    // set mask is applied after removing offset
    uint32_t set = (encrypt(key.address >> setShift)) & setMask;
    DPRINTF(KeyedIndex, "extractSet %llx\n", set);

    return set;
}

std::vector<ReplaceableEntry *>
TaggedSetAssociative::getPossibleEntries(const KeyType &key) const
{
    std::vector<ReplaceableEntry*> entries = sets[extractSet(key)];
    DPRINTF(KeyedIndex, "getPossibleEntries %d\n", entries.size());
    return entries;
}

Addr
TaggedSetAssociative::regenerateAddr(const KeyType &key,
                                     const ReplaceableEntry *entry) const
{
    if (!keyedIndex) {
        return (key.address << tagShift) | (entry->getSet() << setShift);
    }
    // the tag is the whole line address
    Addr regenerated = (key.address << tagShift);
    DPRINTF(KeyedIndex, "regenerateAddr %llx\n", regenerated);
    return regenerated;
}

Addr
TaggedSetAssociative::extractTag(const Addr addr) const
{
    if (!keyedIndex) {
        return TaggedIndexingPolicy::extractTag(addr);
    }
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
TaggedSetAssociative::parseBoxFile(const std::string &path)
{
    std::ifstream f(path);
    fatal_if(!f.is_open(),
        "keyed index: cannot open box file '%s'", path.c_str());

    // Slurp the whole file into one string
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    // ---- locate section boundaries ----------------------------------------
    std::size_t sboxPos = content.find("S-box:");
    std::size_t pboxPos = content.find("P-box:");
    fatal_if(sboxPos == std::string::npos,
        "keyed index: 'S-box:' not found in '%s'", path.c_str());
    fatal_if(pboxPos == std::string::npos,
        "keyed index: 'P-box:' not found in '%s'", path.c_str());

    std::string sboxSection = content.substr(sboxPos + 6,
                                             pboxPos - sboxPos - 6);
    std::string pboxSection = content.substr(pboxPos + 6);

    // ---- parse S-box -------------------------------------------------------
    // Expect numRounds * halfWidth leaf lists in row-major order.
    std::vector<std::vector<int>> sboxLeaves;
    collectLeafLists(sboxSection, sboxLeaves);

    fatal_if((int)sboxLeaves.size() != numRounds * halfWidth,
        "keyed index: expected %d sbox leaf lists, got %zu, in '%s'",
        numRounds * halfWidth, sboxLeaves.size(), path.c_str());

    for (int i = 0; i < numRounds; i++)
        for (int j = 0; j < halfWidth; j++)
            sbox[i][j] = sboxLeaves[i * halfWidth + j];

    // ---- parse P-box -------------------------------------------------------
    // Expect numRounds leaf lists, one per stage.
    std::vector<std::vector<int>> pboxLeaves;
    collectLeafLists(pboxSection, pboxLeaves);

    fatal_if((int)pboxLeaves.size() != numRounds,
        "keyed index: expected %d pbox leaf lists, got %zu, in '%s'",
        numRounds, pboxLeaves.size(), path.c_str());

    for (int i = 0; i < numRounds; i++)
        pbox[i] = pboxLeaves[i];

    // ---- sanity check ------------------------------------------------------
    for (int i = 0; i < numRounds; i++) {
        for (int j = 0; j < halfWidth; j++) {
            fatal_if(sbox[i][j].empty(),
                "keyed index: sbox[%d][%d] empty after parsing '%s'",
                i, j, path.c_str());
        }
        fatal_if(pbox[i].empty(),
            "keyed index: pbox[%d] empty after parsing '%s'",
            i, path.c_str());
    }

    DPRINTF(Cache, "keyed index: loaded S-box/P-box from '%s'\n", path.c_str());
}

} // namespace gem5

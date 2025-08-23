#include "lz4.h"
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
void appendLength(std::vector<uint8_t> &out, size_t len)
{
    while(len >= 255)
    {
        out.push_back(255);
        len -= 255;
    }
    out.push_back(static_cast<uint8_t>(len));
}

void SimpleLZ4::encodeToken(std::vector<uint8_t> &output, int literalLength, int matchLength, int offset, const std::vector<uint8_t> &literals) 
{
    // Token : [literal length] (4 bits)  [matchLength - MIN_MATCH_LENGTH] (4 bits)
    uint8_t lit = std::min(literalLength, 15);
    uint8_t mat = matchLength >= MIN_MATCH_LENGTH ? std::min(matchLength - MIN_MATCH_LENGTH, 15) : 0;
    uint8_t token = (lit << 4) | mat;
    output.push_back(token);
    if(literalLength >= 15) appendLength(output, literalLength - 15);
    output.insert(output.end(), literals.begin(), literals.end());
    if(matchLength >= MIN_MATCH_LENGTH)
    {
        output.push_back(offset & 0xFF);
        output.push_back((offset >> 8) & 0xFF); // little-endian
        if(matchLength >= MIN_MATCH_LENGTH + 15)
            appendLength(output, matchLength - 15 - MIN_MATCH_LENGTH);
    }
}

bool SimpleLZ4::findLongestMatch(const std::vector<uint8_t> &input, size_t currPos, size_t windowStart, size_t &matchPos, size_t &matchLen)
{
    constexpr int HASH_BITS = 16;
    constexpr int HASH_SIZE = 1<<16;
    static std::vector<int> hashTable(HASH_SIZE, -1);

    if(currPos + MIN_MATCH_LENGTH > input.size()) return false;
    unsigned int sequence = 0;
    std::memcpy(&sequence, &input[currPos], MIN_MATCH_LENGTH);
    unsigned int hash = ((sequence*2654435761U) >> (32 - HASH_BITS)) & (HASH_BITS - 1);
    int candidate = hashTable[hash];
    hashTable[hash] = static_cast<int>(currPos);
    
    matchLen = 0;
    matchPos = 0;
    if(candidate < 0 || static_cast<size_t>(candidate) <= windowStart || candidate >= static_cast<int>(currPos))
        return 0;
    
    size_t maxLen = input.size() - currPos;
    while(matchLen < maxLen && input[candidate + matchLen] == input[currPos + matchLen])
    {
        ++matchLen;
    }
    if(matchLen >= MIN_MATCH_LENGTH)
    {
        matchPos = candidate;
        return true;
    }
    return false;
}

std::vector<uint8_t> SimpleLZ4::compress(const std::vector<uint8_t> &input) 
{
    constexpr size_t WINDOW_SIZE = 65535; // 64 KB
    std::vector<uint8_t> output;
    std::vector<uint8_t> literals;

    size_t pos = 0;
    size_t inputSize = input.size();
    while(pos < inputSize)
    {
        size_t matchPos = 0, matchLen = 0;
        size_t windowStart = (pos > WINDOW_SIZE) ? pos - WINDOW_SIZE : 0;

        bool found = (pos + MIN_MATCH_LENGTH <= inputSize) && findLongestMatch(input, pos, windowStart, matchPos, matchLen);
        if(found)
        {
            int offset = static_cast<int> (pos - matchPos);
            encodeToken(output, literals.size(), matchLen, offset, literals);
            literals.clear();
            pos += matchLen;
        }
        else
        {
            literals.push_back(input[pos]);
            ++pos;
            if(literals.size() == 65535)
            {
                encodeToken(output, literals.size(), 0, 0, literals);
                literals.clear();
            }
        }
    } 
    if(!literals.empty())
        encodeToken(output, literals.size(), 0, 0, literals);
    return output;
}

size_t SimpleLZ4::decodeLength(const std::vector<uint8_t>& input, size_t& pos) {
    size_t len = 0;
    while (pos < input.size() && input[pos] == 255) {
        len += 255;
        ++pos;
    }
    if (pos < input.size()) {
        len += input[pos];
        ++pos;
    }
    return len;
}


std::vector<uint8_t> SimpleLZ4::decompress(const std::vector<uint8_t>& compressed) {
    std::vector<uint8_t> output;
    size_t pos = 0;

    while (pos < compressed.size()) {
        // Read token byte
        uint8_t token = compressed[pos++];
        size_t literalLength = token >> 4;
        size_t matchLength = (token & 0x0F) + MIN_MATCH_LENGTH;

        // Decode literal length extension if applicable
        if (literalLength == 15) {
            literalLength += decodeLength(compressed, pos);
        }

        // Bounds check for literal section
        if (pos + literalLength > compressed.size()) {
            throw std::runtime_error("Literal length out of bounds during decompression.");
        }

        // Copy literals verbatim
        output.insert(output.end(), compressed.begin() + pos, compressed.begin() + pos + literalLength);
        pos += literalLength;

        // If at end of input after literals, decompression finished
        if (pos >= compressed.size()) break;

        // Check for sufficient bytes for offset
        if (pos + 2 > compressed.size()) {
            throw std::runtime_error("Unexpected end of input when reading offset.");
        }

        // Read offset (little-endian)
        uint16_t offset = compressed[pos] | (compressed[pos + 1] << 8);
        pos += 2;

        // Validate offset
        if (offset == 0 || offset > output.size()) {
            throw std::runtime_error("Invalid offset in decompression.");
        }

        // Decode match length extension if applicable
        if ((token & 0x0F) == 15) {
            matchLength += decodeLength(compressed, pos);
        }

        // Copy match bytes (handle overlapping matches)
        size_t matchPos = output.size() - offset;
        for (size_t i = 0; i < matchLength; ++i) {
            output.push_back(output[matchPos + i]);
        }
    }
    return output;
}

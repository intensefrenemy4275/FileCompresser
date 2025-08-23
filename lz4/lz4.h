#ifndef LZ4_H
#define LZ4_H
#include <vector>
#include <cstdint>

class SimpleLZ4
{
public:
    std::vector<uint8_t> compress(const std::vector<uint8_t> &input);
    std::vector<uint8_t> decompress(const std::vector<uint8_t> &compressed);

private:
    static constexpr int MIN_MATCH_LENGTH = 4;
    static void encodeToken(std::vector<uint8_t> &output, int literalLength, int matchLength, int offSet, const std::vector<uint8_t> &literals);
    static size_t decodeLength(const std::vector<uint8_t> &input, size_t &pos);
    static bool findLongestMatch(const std::vector<uint8_t> &input, size_t currPos, size_t windowStart, size_t &matchPos, size_t &matchLen);
};
#endif
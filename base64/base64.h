#ifndef BASE64_H
#define BASE64_H
#include <string>
#include <vector>

class base64 {
public:
    static void encode(const std::string &inFile, const std::string &outFile, size_t threads);
    static void decode(const std::string &inFile, const std::string &outFile, size_t threads);
};
#endif
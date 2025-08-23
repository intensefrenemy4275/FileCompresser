#include <chrono>
#include <fstream>
#include <iostream>
#include "lz4.h"
#include <vector>

constexpr size_t CHUNK_SIZE = 1<<20; // 1MB

std::vector<uint8_t> readFile(const std::string &path)
{
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if(!in) throw std::runtime_error("Cannot open file : " + path);
    std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    if(!in.read(reinterpret_cast<char*> (buffer.data()), size)) {
        throw std::runtime_error("Cannot read file : " + path);
    }
    return buffer;
}

void writeFile(const std::string &path, const std::vector<uint8_t> &data)
{
    std::ofstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot open file: " + path);
    if (!file.write(reinterpret_cast<const char*>(data.data()), data.size())) {
        throw std::runtime_error("Failed to write file: " + path);
    }
}

int main(int argc, char*argv[])
{
    if (argc != 4) {
        std::cerr << "Usage: lz4 <compress|decompress> <input_file> <output_file>\n";
        return 1;
    }
    std::string mode = argv[1];
    std::string inputFile = argv[2];
    std::string outputFile = argv[3];

    SimpleLZ4 codec;
    try
    {
        auto start = std::chrono::steady_clock::now();
        std::vector<uint8_t> inputData = readFile(inputFile);
        std::vector<uint8_t> outputData;

        if (mode == "compress") {
            outputData = codec.compress(inputData);
        } else if (mode == "decompress") {
            outputData = codec.decompress(inputData);
        } else {
            std::cerr << "Invalid mode: choose 'compress' or 'decompress'\n";
            return 1;
        }

        writeFile(outputFile, outputData);
        auto end = std::chrono::steady_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        std::cout << mode << "ion completed in " << ms << " ms\n";
    }
    catch(const std::exception &e)
    {
        std::cerr << "Error : " << e.what() << '\n';
        return 1;
    }

    return 0;
}
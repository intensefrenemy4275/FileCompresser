#include "base64.h"
#include <iostream>
#include <string>
int main(int argc, char* argv[])
{
    if(argc < 4)
    {
        std::cerr << "Insufficient Arguments!\nUsage : [operation] [inputFile] [outputFile] [threads]\n";
        return 1; 
    }
    std::string op = argv[1];
    if(op == "encode")
        base64::encode(argv[2], argv[3], std::stoi(argv[4]));
    else if(op == "decode")
        base64::decode(argv[2], argv[3], std::stoi(argv[4]));
    else 
    {
        std::cerr << "Invalid Use!\nUsage : [operation] [inputFile] [outputFile] [threads]\n";
        return 1; 
    }
    return 0;
}
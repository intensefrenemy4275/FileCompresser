#include "HuffmanCoding.hpp"
#include <iostream>
#include <string.h>

using namespace std;
int main(int argc,char** argv){
    cout<<argv[1]<<" "<<argv[2]<<endl;
    HuffmanCoding obj;
     if (strcmp(argv[1], "compress") == 0) {
        obj.encode(argv[2]);
    } else {
        obj.decode(argv[2]);
    }
    return 0;
}
#ifndef HUFFMAN_CODING_HPP
#define HUFFMAN_CODING_HPP

#include <string>
#include <unordered_map>
#include <queue>
#include <memory>
using namespace std;

class HuffmanCoding {
public:
    HuffmanCoding();                         
    ~HuffmanCoding();                        


    // Encodes the input text using the generated Huffman codes
    int encode(const std::string& text) ;

    // Decodes the encoded string back to original text
    int decode(const std::string& encodedText) ;


private:
    struct Node {
        char ch;
        unsigned long long freq;
        Node *left;
        Node *right;

        Node(char character, int frequency);
        bool isLeaf() const;
    };

   struct Compare {
        bool operator()(const Node* a, const Node* b);
    };

    Node *root;
    unordered_map<char, string> huffmanCodes;
    unordered_map<char, unsigned long long> freqTable;

    void buildFrequencyTable(const string& text);
    void buildTree();
    string compress();
    string getHeader();
    void generateCodes(Node *node, const string& code);
    void freeTree(Node* node) ;
};

#endif 

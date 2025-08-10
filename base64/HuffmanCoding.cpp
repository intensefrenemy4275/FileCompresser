#include "HuffmanCoding.hpp"
#include <fstream>
#include <iostream>
#include <bitset>

// Constructor
HuffmanCoding::HuffmanCoding() {

 }

// Destructor
HuffmanCoding::~HuffmanCoding() {
    // Optional: implement recursive delete if no smart pointers are used
}

// Build frequency table from text
void HuffmanCoding::buildFrequencyTable(const string& text) {
    freqTable.clear();
    for (char ch : text) {
        freqTable[ch]++;
    }
}

// Build Huffman Tree from frequency table
void HuffmanCoding::buildTree() {
    priority_queue<Node*, vector<Node*>, Compare> pq;

    for (auto& pair : freqTable) {
        pq.push(new Node(pair.first, pair.second));
    }

    while (pq.size() > 1) {
        Node* left = pq.top(); pq.pop();
        Node* right = pq.top(); pq.pop();

        Node* parent = new Node('$', left->freq + right->freq);
        parent->left = left;
        parent->right = right;
        pq.push(parent);
    }

    root= pq.empty() ? nullptr : pq.top();
}

// Recursively generate Huffman codes
void HuffmanCoding::generateCodes(Node* node, const string& code) {
    if (!node) return;

    if (node->isLeaf()) {
        huffmanCodes[node->ch] = code;
        return;
    }

    generateCodes(node->left, code + "0");
    generateCodes(node->right, code + "1");
}

// Encodes the input text using the generated Huffman codes

string HuffmanCoding::getHeader(int offset) {
    string headerStr="`";
    for (const auto& pair : huffmanCodes) {
        headerStr += pair.first; 
        headerStr +=pair.second + "`"; 
    }
    headerStr += to_string(offset);
    return headerStr;
}
int HuffmanCoding::encode(const string& fileName) {
    ifstream in(fileName);
    if (!in) {
        cerr << "Failed to open input file: " << fileName << endl;
        return 1;
    }
    string content((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    in.close();
    buildFrequencyTable(content);
    buildTree();  // make sure you store root
    generateCodes(root, "");

    ofstream outputFile("output.txt", std::ios::binary);
    if (!outputFile) {
        cerr << "Failed to open output file.\n";
        return 0;
    }

    string encodedStr;
    unsigned char buffer = 0;
    int bitsInBuffer = 0;

    for (char c : content) {
        const string& code = huffmanCodes.at(c);
        for (char bit : code) {
            buffer = (buffer << 1) | (bit - '0');
            bitsInBuffer++;

            if (bitsInBuffer == 8) {
                outputFile.put(buffer);
                buffer = 0;
                bitsInBuffer = 0;
            }
        }
    }

    int padding = 0;
    if (bitsInBuffer > 0) {
        padding = 8 - bitsInBuffer;
        buffer <<= padding; // pad remaining bits with 0s
        outputFile.put(buffer);
    }

    // Save padding info as last byte
    string codesHeader = getHeader(padding);
    for (char c : codesHeader) {
        outputFile.put(c);
    }

    outputFile.close();

    return 0;
}

string readBinaryDataFromFile(const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file) {
        cerr << "Error opening file: " << filename << endl;
        return "";
    }
    string data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    return data;
}

// Decode the encoded string using the Huffman tree
int HuffmanCoding::decode(const string& filename)  {
    string fileContent = readBinaryDataFromFile(filename);
    if (fileContent.empty()) return 0;

    size_t i = 0;
    string encodedBytes;
    
    // Read encoded bytes (until first '`')
    while (i < fileContent.size() && fileContent[i] != '`') {
        encodedBytes += fileContent[i++];
    }

    ++i; // skip the '`' delimiter

    // Read Huffman map
    unordered_map<string, char> codeToChar;
    string temp;
    while (i < fileContent.size()) {
        if (fileContent[i] == '`') {
            // temp format: ch + code => ex: A1001 => map["1001"] = 'A'
            codeToChar[temp.substr(1)] = temp[0];
            temp.clear();
        } else {
            temp += fileContent[i];
        }
        ++i;
    }

    // Get padding (offset) from last part of temp
    int offset = 8 - stoi(temp);

    // Convert encoded bytes to binary string
    string binaryStr;
    for (char c : encodedBytes) {
        binaryStr += bitset<8>(static_cast<unsigned char>(c)).to_string();
    }

    // Decode binary string using Huffman map
    string decodedText;
    string key;
    for (size_t j = 0; j < binaryStr.size() - offset; ++j) {
        key += binaryStr[j];
        if (codeToChar.count(key)) {
            decodedText += codeToChar[key];
            key.clear();
        }
    }

    // Write output to a decoded file
    ofstream output("decoded_output.txt");
    if (!output) {
        cerr << "Failed to write decoded output." << endl;
        return 1;
    }
    output << decodedText;
    output.close();
    return 1;
}

// ---------- Node Methods ----------

HuffmanCoding::Node::Node(char character, int frequency)
    : ch(character), freq(frequency), left(nullptr), right(nullptr) { }

bool HuffmanCoding::Node::isLeaf() const {
    return !left && !right;
}

// ---------- Compare Struct ----------

bool HuffmanCoding::Compare::operator()(const Node* a, const Node* b) {
    return a->freq > b->freq; // Min-heap: lower freq = higher priority
}

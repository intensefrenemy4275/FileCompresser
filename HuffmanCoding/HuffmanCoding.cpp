#include "HuffmanCoding.hpp"
#include <fstream>
#include <iostream>
#include <bitset>
#include <mutex>
#include <thread>

std::mutex freqMutex;

// ---------- Constructor / Destructor ----------

HuffmanCoding::HuffmanCoding() : root(nullptr) {}

HuffmanCoding::~HuffmanCoding() {
    freeTree(root);
}

void HuffmanCoding::freeTree(Node* node) {
    if (!node) return;
    freeTree(node->left);
    freeTree(node->right);
    delete node;
}

// ---------- Build Frequency Table (Multithreaded) ----------

void HuffmanCoding::buildFrequencyTable(const std::string& text) {
    freqTable.clear();

    int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;

    int chunkSize = text.size() / numThreads;
    std::vector<std::thread> threads;

    auto worker = [&](int start, int end) {
        std::unordered_map<char, int> localFreq;
        for (int i = start; i < end; i++) {
            localFreq[text[i]]++;
        }
        std::lock_guard<std::mutex> lock(freqMutex);
        for (auto& p : localFreq) freqTable[p.first] += p.second;
    };

    for (int i = 0; i < numThreads; i++) {
        int start = i * chunkSize;
        int end = (i == numThreads - 1) ? text.size() : start + chunkSize;
        threads.emplace_back(worker, start, end);
    }

    for (auto& t : threads) t.join();
}

// ---------- Build Huffman Tree ----------

void HuffmanCoding::buildTree() {
    std::priority_queue<Node*, std::vector<Node*>, Compare> pq;
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

    root = pq.empty() ? nullptr : pq.top();
}

// ---------- Generate Huffman Codes ----------

void HuffmanCoding::generateCodes(Node* node, const std::string& code) {
    if (!node) return;

    if (node->isLeaf()) {
        huffmanCodes[node->ch] = code.empty() ? "0" : code; // Edge case: single char
        return;
    }

    generateCodes(node->left, code + "0");
    generateCodes(node->right, code + "1");
}

// ---------- Encode ----------

std::string HuffmanCoding::getHeader() {
    std::string headerStr;
    for (const auto& pair : huffmanCodes) {
        headerStr += pair.first;         // store character
        headerStr += '|';                // separator before code
        headerStr += pair.second;        // store its code
        headerStr += '`';                // separator after code
    }
    headerStr += "~~"; // marks end of header
    return headerStr;
}

int HuffmanCoding::encode(const std::string& fileName) {
    std::ifstream in(fileName);
    if (!in) {
        std::cerr << "Failed to open input file: " << fileName << std::endl;
        return 1;
    }

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    buildFrequencyTable(content);
    buildTree();
    generateCodes(root, "");

    std::ofstream outputFile("output.txt", std::ios::binary);
    if (!outputFile) {
        std::cerr << "Failed to open output file.\n";
        return 1;
    }

    // Write header
    std::string codesHeader = getHeader();
    for (char c : codesHeader) outputFile.put(c);

    // Encode content
    unsigned char buffer = 0;
    int bitsInBuffer = 0;

    for (char c : content) {
        const std::string& code = huffmanCodes.at(c);
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
        buffer <<= padding;
        outputFile.put(buffer);
    }

    // Save padding info as last byte
    outputFile.put(static_cast<char>(padding));

    outputFile.close();
    return 0;
}

// ---------- Decode ----------

std::string readBinaryDataFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

int HuffmanCoding::decode(const std::string& filename) {
    std::string fileContent = readBinaryDataFromFile(filename);
    if (fileContent.empty()) return 1;

    // Parse header - CORRECTED LOGIC
    size_t i = 0;
    std::unordered_map<std::string, char> codeToChar;
    
    while (i < fileContent.size() - 1) {
        // Check for header end marker
        if (fileContent[i] == '~' && fileContent[i + 1] == '~') {
            i += 2; // skip both ~ characters
            break;
        }
        
        // Read character
        char character = fileContent[i++];
        
        // Expect separator '|'
        if (i >= fileContent.size() || fileContent[i] != '|') {
            std::cerr << "Invalid header format: expected '|' separator" << std::endl;
            return 1;
        }
        i++; // skip '|'
        
        // Read code until '`' separator
        std::string code;
        while (i < fileContent.size() && fileContent[i] != '`') {
            code += fileContent[i];
            i++;
        }
        
        if (i >= fileContent.size() || fileContent[i] != '`') {
            std::cerr << "Invalid header format: expected '`' separator" << std::endl;
            return 1;
        }
        i++; // skip '`'
        
        // Store the mapping
        codeToChar[code] = character;
    }

    if (codeToChar.empty()) {
        std::cerr << "No codes found in header" << std::endl;
        return 1;
    }

    // Extract padding info (last byte)
    if (i >= fileContent.size()) {
        std::cerr << "Invalid file format: no data section" << std::endl;
        return 1;
    }
    
    int padding = static_cast<unsigned char>(fileContent.back());
    
    // Extract encoded data (everything except the last byte which is padding)
    std::string binaryStr;
    for (size_t j = i; j < fileContent.size() - 1; j++) {
        unsigned char byte = static_cast<unsigned char>(fileContent[j]);
        binaryStr += std::bitset<8>(byte).to_string();
    }

    // Remove padding bits
    if (padding > 0 && padding < 8 && padding <= (int)binaryStr.size()) {
        binaryStr = binaryStr.substr(0, binaryStr.size() - padding);
    }

    // Decode using the code-to-character mapping
    std::string decodedText;
    std::string currentCode;
    
    for (char bit : binaryStr) {
        currentCode += bit;
        
        // Check if current code matches any Huffman code
        auto it = codeToChar.find(currentCode);
        if (it != codeToChar.end()) {
            decodedText += it->second;
            currentCode.clear();
        }
    }

    // Check if there are any unmatched bits (shouldn't happen with correct encoding)
    if (!currentCode.empty()) {
        std::cerr << "Warning: Unmatched code bits remaining: " << currentCode << std::endl;
    }

    // Write decoded output
    std::ofstream output("decoded_output.txt");
    if (!output) {
        std::cerr << "Failed to write decoded output." << std::endl;
        return 1;
    }
    output << decodedText;
    output.close();
    
    std::cout << "Decoding completed successfully. Output written to decoded_output.txt" << std::endl;
    return 0;
}

// ---------- Node Methods ----------

HuffmanCoding::Node::Node(char character, int frequency)
    : ch(character), freq(frequency), left(nullptr), right(nullptr) {}

bool HuffmanCoding::Node::isLeaf() const {
    return !left && !right;
}

// ---------- Compare ----------

bool HuffmanCoding::Compare::operator()(const Node* a, const Node* b) {
    return a->freq > b->freq; // Min-heap
}
#include "base64.h"
#include <fstream>
#include <future>
#include <vector>

const size_t chunkSize = 1024 * 1024; // 1MB
static const std::string b64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string encode_chunk(const std::vector<unsigned char> &data){
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    for(size_t i = 0; i < data.size(); i+=3)
    {
        unsigned char a = data[i];
        unsigned char b = (i + 1 < data.size()) ? data[i+1] : 0;
        unsigned char c = (i + 2 < data.size()) ? data[i+2] : 0;

        out.push_back(b64_chars[a >> 2]);
        out.push_back(b64_chars[((a & 0x03) << 4) | (b >> 4)]);
        out.push_back(i + 1 < data.size() ? b64_chars[((b & 0x0F) << 2) | (c >> 6)] : '=');
        out.push_back(i + 2 < data.size() ? b64_chars[c & 0x3F] : '=');
    }
    return out;
}

void base64::encode(const std::string &inFile, const std::string &outFile, size_t threads)
{
    std::ifstream in(inFile, std::ios::binary);
    std::ofstream out(outFile);
    if(!in || !out) throw std::runtime_error("File Couldn't be Opened");

    std::vector<std::future<std::string>> futures;
    while(true)
    {
        std::vector<unsigned char> buf(chunkSize);
        in.read(reinterpret_cast<char*>(buf.data()), buf.size());
        size_t readBytes = in.gcount();
        if(readBytes == 0) break;
        buf.resize(readBytes);
        futures.push_back(std::async(std::launch::async, encode_chunk, buf));

        if(futures.size() >= threads)
        {
            out << futures.front().get();
            futures.erase(futures.begin());
        }
    }
    for(auto& f: futures) out << f.get();
}

static inline int b64_index(char c)
{
    auto pos = b64_chars.find(c);
    return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
}

static std::vector<unsigned char> decode_chunk(const std::string &input)
{
    std::vector<unsigned char> out;
    out.reserve((input.size() / 4) * 3);

    unsigned char a3[3];
    unsigned char a4[4];
    size_t i = 0;

    for(char c : input)
    {
        if(c == '=' || b64_index(c) != -1)
        {
            a4[i++] = static_cast<unsigned char>(c);
            if(i == 4)
            {
                int vals[4];
                for(int k = 0; k<4; k++)
                    vals[k] = (a4[k] == '=') ? 0 : b64_index(a4[k]);
                a3[0] = (vals[0] << 2) | ((vals[1] & 0x30) >> 4);
                a3[1] = ((vals[1] & 0xf) << 4) | ((vals[2] & 0x3c) >> 2);
                a3[2] = ((vals[2] & 0x3) << 6) | vals[3];

                out.push_back(a3[0]);
                if(a4[2] != '=') out.push_back(a3[1]);
                if(a4[3] != '=') out.push_back(a3[2]);
                i = 0;
            }
        }
    }
    return out;
}

void base64::decode(const std::string &inFile, const std::string &outFile, size_t threads)
{
    std::ifstream in(inFile);
    std::ofstream out(outFile, std::ios::binary);
    if (!in || !out) throw std::runtime_error("File open failed");

    std::vector<std::future<std::vector<unsigned char>>> futures;
    while(true)
    {
        std::string buf(chunkSize, '\0');
        in.read(&buf[0], buf.size());
        int bytesRead = in.gcount();
        if(bytesRead == 0) break;
        buf.resize(bytesRead);

        while((buf.size() % 4 != 0) && in) // Don't break a 4-char block
        {
            char extra;
            if(!in.get(extra)) break;
            buf.push_back(extra);
        }

        futures.push_back(std::async(std::launch::async, decode_chunk, buf));

        if(futures.size() >= threads)
        {
            const auto &decoded = futures.front().get();
            out.write(reinterpret_cast<const char*>(decoded.data()), decoded.size());
            futures.erase(futures.begin());
        }
    }
    for(auto &f : futures)
    {
        const auto &decoded = f.get();
        out.write(reinterpret_cast<const char*>(decoded.data()), decoded.size());
    }
}
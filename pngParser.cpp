#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>

class Parser {
public:
    ~Parser() = default;
    std::vector<char> compressedIDAT;

    bool parse(const std::string& filepath, unsigned int* rWidth, unsigned int* rHeight, unsigned int* BPP) {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file) {
            std::cerr << "Failed to open file " << filepath << "\n";
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) {
            std::cerr << "Failed to read from file\n";
            return false;
        }

        const unsigned char pngHeader[8] = {
            0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A
        };
        if (std::memcmp(buffer.data(), pngHeader, 8) != 0) {
            std::cerr << "The provided file " << filepath << " is not a valid PNG file\n";
            return false;
        }

        const char* reader = buffer.data() + 8;
        const char* end = buffer.data() + size;

        while (reader + 8 <= end) {
            uint32_t length = readBigEndian32(reader);
            reader += 4;

            std::string chunkType(reader, 4);
            reader += 4;

            if (chunkType == "IHDR" && reader + 13 <= end) {
                *rWidth = readBigEndian32(reader);
                *rHeight = readBigEndian32(reader + 4);
                *BPP = static_cast<unsigned char>(reader[8]);
            }
            else if (chunkType == "IDAT") {
                compressedIDAT.insert(compressedIDAT.end(), reader, reader + length);
            }

            reader += length + 4; // skip data and CRC
        }

        return true;
    }

private:
    static uint32_t readBigEndian32(const char* data) {
        return (static_cast<unsigned char>(data[0]) << 24) |
               (static_cast<unsigned char>(data[1]) << 16) |
               (static_cast<unsigned char>(data[2]) << 8)  |
               (static_cast<unsigned char>(data[3]));
    }
};

int main() {
    const std::string filepath = "..\\dbh.png";
    Parser parser;

    unsigned int width = 0, height = 0, BPP = 0;
    if (parser.parse(filepath, &width, &height, &BPP)) {
        std::cout << "Width: " << width << "\n";
        std::cout << "Height: " << height << "\n";
        std::cout << "Bits per pixel: " << BPP << "\n";
        std::cout << "Compressed IDAT size: " << parser.compressedIDAT.size()/(1024.f*1024.f) << " MB\n";
    }

    return 0;
}

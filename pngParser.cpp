#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
struct ParsedData{
    unsigned int width;
    unsigned int height;
    unsigned char bpp;
    unsigned char colorType;
    unsigned char compressionMethod;
    unsigned char filterMethod;
    unsigned char interlaceMethod;
};
class Parser {
public:
    ~Parser() = default;
    std::vector<char> compressedIDAT;

    bool parse(const std::string& filepath, ParsedData& parsedData) {
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
                parsedData.width = readBigEndian32(&reader[0]);
                parsedData.height = readBigEndian32(&reader[4]);
                parsedData.bpp = static_cast<unsigned char>(reader[8]);
                parsedData.colorType = static_cast<unsigned char>(reader[9]);
                parsedData.compressionMethod = static_cast<unsigned char>(reader[10]);
                parsedData.filterMethod = static_cast<unsigned char>(reader[11]);
                parsedData.interlaceMethod = static_cast<unsigned char>(reader[12]);

                //TODO:(Fahad):Check if it follows the PNG standard
            }
            else if (chunkType == "IDAT") {
                //TODO:(Fahad):Confirm that the chunk metadata is not being transferred to imagedata
                compressedIDAT.insert(compressedIDAT.end(), reader, reader + length);
            }else if (chunkType == "IEND"){
                //TODO:(Fahad):Decompress and Organize data in a buffer
            }else{
                std::cout << "Chunktype "<<chunkType<<" Encountered!\n";
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

    ParsedData parsedData;
    if (parser.parse(filepath, parsedData)) {
        std::cout << "Width: " << parsedData.width << "\n";
        std::cout << "Height: " << parsedData.height << "\n";
        std::cout << "Bits per channel: " << static_cast<int>(parsedData.bpp) << "\n";
        std::cout << "Color type: " << static_cast<int>(parsedData.colorType) << "\n";
        std::cout << "Compression Method: " << static_cast<int>(parsedData.compressionMethod) << "\n";
        std::cout << "Filter Method: " << static_cast<int>(parsedData.filterMethod) << "\n";
        std::cout << "Interlace Method: " << static_cast<int>(parsedData.interlaceMethod) << "\n";
        std::cout << "Compressed IDAT size: " << parser.compressedIDAT.size()/(1024.f*1024.f) << " MB\n";
    }else{
        std::cerr << "Failed to load the png " << filepath << "\n";
    }

    return 0;
}

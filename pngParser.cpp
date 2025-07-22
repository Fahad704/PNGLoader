#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <math.h>

typedef unsigned int u32;

struct ParsedData{
    unsigned int width;
    unsigned int height;
    unsigned char bpp;
    unsigned char colorType;
    unsigned char compressionMethod;
    unsigned char filterMethod;
    unsigned char interlaceMethod;
    std::vector<char> compressedData;
};
class Parser {
public:
    ~Parser() = default;

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
                parsedData.compressedData.insert(parsedData.compressedData.end(), reader, reader + length);
            }else if (chunkType == "IEND"){
                //TODO:(Fahad):Decompress and Organize data in a buffer
            }else{
                std::cout << "Chunktype "<<chunkType<<" Encountered!\n";
            }

            reader += length + 4; // skip data and CRC
        }
        if(decompressData(parsedData)){
            return true;
        }
        else{
            std::cout << "Error : unable to decompress data\n";
            return false;
        }
    }
    std::string byteAsStr(char value){
        std::string result = "";
        for (int i=7;i>-1;i--){
            int mask = pow(2,i);
            result += std::to_string(((value & mask) != 0));
        }
        return result;
    }
    bool decompressData(ParsedData& parsedData){
        const char* reader = parsedData.compressedData.data();
        const char* end = parsedData.compressedData.data() + parsedData.compressedData.size();

        char header = reader[0];

        bool lastblockbit = (header & 1);
        char compressionType = ((header & 2) | (header & 4));
        std::cout<<"---Deflate--info---\n";
        std::cout << "Header byte : "<< byteAsStr(header) << "\n";
        std::cout << "Last block in stream : "<<lastblockbit<<"\n";
        std::cout << "Compression Type : "<<int(compressionType) <<"\n";
        
        reader += 1;
        
        //TODO(Fahad):Decompress deflate stream
        
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
        std::cout<<"---PNG--info---\n";
        std::cout << "Width: " << parsedData.width << "\n";
        std::cout << "Height: " << parsedData.height << "\n";
        std::cout << "Bits per channel: " << static_cast<int>(parsedData.bpp) << "\n";
        std::cout << "Color type: " << static_cast<int>(parsedData.colorType) << "\n";
        std::cout << "Compression Method: " << static_cast<int>(parsedData.compressionMethod) << "\n";
        std::cout << "Filter Method: " << static_cast<int>(parsedData.filterMethod) << "\n";
        std::cout << "Interlace Method: " << static_cast<int>(parsedData.interlaceMethod) << "\n";
        std::cout << "Compressed IDAT size: " << parsedData.compressedData.size()/(1024.f*1024.f) << " MB\n";
    }else{
        std::cerr << "Failed to load the png " << filepath << "\n";
    }
    return 0;
}

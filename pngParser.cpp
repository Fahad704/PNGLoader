#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <math.h>

typedef unsigned int u32;
typedef unsigned char u8;
typedef unsigned short u16;

struct ParsedData{
    unsigned int width;
    unsigned int height;
    unsigned char bpp;
    unsigned char colorType;
    unsigned char compressionMethod;
    unsigned char filterMethod;
    unsigned char interlaceMethod;
    std::vector<char> compressedData;
    std::vector<char> imageData;
};
char colorTypes[7][20] = {
    "Grayscale",            // 0
    "ERROR",
    "Truecolor",            // 2
    "Indexed",              // 3
    "Grayscale and Alpha",  // 4
    "ERROR",
    "Truecolor and Alpha"   // 6
};
char compressionNames[3][16] = {
    "No Compression","Fixed Huffman","Dynamic Huffman"
};
class Parser {
public:
    ~Parser() = default;

    bool parse(const std::string& filepath, ParsedData& parsedData) {
        bool result=true;
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
            uint32_t length = readLittleEndian32(reader);
            reader += 4;

            std::string chunkType(reader, 4);
            reader += 4;

            if (chunkType == "IHDR" && reader + 13 <= end) {
                parsedData.width = readLittleEndian32(&reader[0]);
                parsedData.height = readLittleEndian32(&reader[4]);
                parsedData.bpp = static_cast<unsigned char>(reader[8]);
                parsedData.colorType = static_cast<unsigned char>(reader[9]);
                parsedData.compressionMethod = static_cast<unsigned char>(reader[10]);
                parsedData.filterMethod = static_cast<unsigned char>(reader[11]);
                parsedData.interlaceMethod = static_cast<unsigned char>(reader[12]);

                //TODO:(Fahad):Check if it follows the PNG standard
            }
            else if (chunkType == "IDAT") {
                //std::cout << length << " Bytes inserted in compressedData\n";
                parsedData.compressedData.insert(parsedData.compressedData.end(), reader, reader + length);
            }else if (chunkType == "IEND"){
                //TODO:(Fahad):Decompress and Organize data in a buffer
                if(!decompressData(parsedData)){
                    std::cout << "Error : unable to decompress data\n";
                    result = false;
                }
            }else{
                std::cout << "Unhandled Chunktype "<<chunkType<<" Encountered\n";
            }

            reader += length + 4; // skip data and CRC
        }
        
        return result;
    }
    std::string byteAsBin(char value){
        std::string result = "";
        for (int i=7;i>-1;i--){
            int mask = pow(2,i);
            result += std::to_string(((value & mask) != 0));
        }
        return result;
    }
    bool decompressData(ParsedData& parsedData){
        //Block compression types
        enum BlockType{
            BTYPE_NO_COMPRESSION=0,
            BTYPE_FIXED_HUFFMAN,
            BTYPE_DYNAMIC_HUFFMAN,
            BTYPE_RESERVED
        };
        const char* reader = parsedData.compressedData.data();
        const char* end = parsedData.compressedData.data() + parsedData.compressedData.size();
        //skip zlib header
        reader += 2;
        std::cout << "Compressed data size: "<<parsedData.compressedData.size()<<" Bytes \n";

        while((reader + 8) <= end){
            //Block header
            char header = reader[0];

            bool lastblockbit = (header & 1);// Header & 00000001
            int compressionType = (header & 2) | ((header & 4) >> 2);
            std::cout<<"---Deflate--Block---\n";
            std::cout << "Header byte : "<< byteAsBin(header) << "\n";
            std::cout << "Last block in stream : "<<lastblockbit<<"\n";
            std::cout << "Compression Type : "<< compressionType << " (" << compressionNames[compressionType]<< ")\n";
            reader += 1;
            u16 blockLength = 0;
            if(compressionType == BTYPE_NO_COMPRESSION){
                blockLength = readBigEndian16(&reader[0]);
                u16 blockLengthComplement = readBigEndian16(&reader[2]);
                std::cout<<"Block length : "<<blockLength<<"\n";
                std::cout<<"Block Complement length : "<< blockLengthComplement <<"\n";
                std::cout << "Block length(b):"<<byteAsBin(char(blockLength & 0xFF))<<" "<<byteAsBin(char((blockLength >> 8) & 0xFF))<<"\n";
                std::cout << "Block comp(b):\t"<<byteAsBin(char(blockLengthComplement & 0xFF))<<" "<<byteAsBin(char((blockLengthComplement >> 8) & 0xFF))<<"\n";
                //skip length data
                reader += 4;
                //skip data
                parsedData.imageData.insert(parsedData.imageData.end(),reader,reader+blockLength);
                reader += blockLength;
                continue;
            }else if(compressionType == BTYPE_FIXED_HUFFMAN){
                return true;
            }else if(compressionType == BTYPE_DYNAMIC_HUFFMAN){
                return true;
            }else{
                std::cout << "ERROR : INVALID COMPRESSION TYPE\n";
                return false;
            }
            
        }

        //TODO(Fahad):Decompress deflate stream
        
        return true;
    }
    
    private:
    //Little endian
    static u32 readLittleEndian32(const char* data) {
        return (static_cast<unsigned char>(data[0]) << 24) |
        (static_cast<unsigned char>(data[1]) << 16) |
        (static_cast<unsigned char>(data[2]) << 8)  |
        (static_cast<unsigned char>(data[3]));
    }
    static u16 readLittleEndian16(const char* data){
        return (static_cast<u16>(data[0]) << 8)|
        (static_cast<u16>(data[1]));    
    }
    //Big endian
    static u32 readBigEndian32(const char* data) {
        return (static_cast<unsigned char>(data[0])) |
        (static_cast<unsigned char>(data[1]) << 8) |
        (static_cast<unsigned char>(data[2]) << 16)  |
        (static_cast<unsigned char>(data[3]) << 24);
    }
    static u16 readBigEndian16(const char* data) {
        return (static_cast<unsigned char>(data[0])) |
        (static_cast<unsigned char>(data[1]) << 8);
    }
};
void printCompressedData(const char* data,u32 size){
    std::ofstream ofs;
    ofs.open("compressedrawdata",std::ios::binary);

    if(!ofs.is_open()){
        std::cout << "Failed to open compressedrawdata\n";
        return;
    }else{
        ofs.write(data,size);
    }
    ofs.close();
}
void outputToPPM(const char* buffer,u32 width,u32 height){
    const char* reader = buffer;
    
    std::ofstream ofs;
    ofs.open("imageoutput.ppm");
    if(!ofs.is_open()){
        std::cout << "Failed to open imageoutput.ppm\n";
        return;
    }

    ofs << "P3\n" << width << " " <<  height <<"\n255\n";
    for(int i=0;i<height;i++){
        reader += 1; // skip filter byte
        for(int j=0;j<width;j++){
            ofs << int(u8(*reader)) << ' ' << int(u8(*(reader + 1))) << ' ' << int(u8(*(reader + 2))) << '\n';
            reader += 3;
        }
    }
    ofs.close();
}
int main() {
    const std::string filepath = "..\\simple.png";
    Parser parser;
    ParsedData parsedData;
    
    if (parser.parse(filepath, parsedData)) {
        std::cout<<"---PNG--info---\n";
        std::cout << "Width: " << parsedData.width << "\n";
        std::cout << "Height: " << parsedData.height << "\n";
        std::cout << "Bits per channel: " << int(parsedData.bpp) << "\n";
        std::cout << "Color type: " << int(parsedData.colorType) << " (" << colorTypes[parsedData.colorType] << ")\n";
        std::cout << "Compression Method: " << int(parsedData.compressionMethod) << "\n";
        std::cout << "Filter Method: " << int(parsedData.filterMethod) << "\n";
        std::cout << "Interlace Method: " << int(parsedData.interlaceMethod) << (parsedData.interlaceMethod?(" (Adam7 Interlace)"):(" (No interlace)")) << "\n";
        std::cout << std::fixed << "IDAT size: " << parsedData.compressedData.size() << " Bytes\n";
    }else{
        std::cerr << "Failed to load the png " << filepath << "\n";
    }
    
    outputToPPM(parsedData.imageData.data(),parsedData.width,parsedData.height);
    return 0;
}

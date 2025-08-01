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
            //std::cout << "Header byte : "<< byteAsBin(header) << "\n";
            std::cout << "Last block in stream : "<<lastblockbit<<"\n";
            std::cout << "Compression Type : "<< compressionType << " (" << compressionNames[compressionType]<< ")\n";
            reader += 1;
            u16 blockLength = 0;
            if(compressionType == BTYPE_NO_COMPRESSION){
                blockLength = readBigEndian16(&reader[0]);
                //u16 blockLengthComplement = readBigEndian16(&reader[2]);
                std::cout<<"Block length : "<<blockLength<<"\n";
                //std::cout<<"Block Complement length : "<< blockLengthComplement <<"\n";
                //std::cout << "Block length(b):"<<byteAsBin(char(blockLength & 0xFF))<<" "<<byteAsBin(char((blockLength >> 8) & 0xFF))<<"\n";
                //std::cout << "Block comp(b):\t"<<byteAsBin(char(blockLengthComplement & 0xFF))<<" "<<byteAsBin(char((blockLengthComplement >> 8) & 0xFF))<<"\n";
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
void exportData(const u8* data,u32 size){
    std::ofstream ofs;
    ofs.open("exportedData",std::ios::binary);

    if(!ofs.is_open()){
        std::cout << "Failed to open compressedrawdata\n";
        return;
    }else{
        ofs.write((char*)data,size);
    }
    ofs.close();
}
u32 paethPredictor(u8 a,u8 b,u8 c){
    u32 pr = 0;
    int p = a + b - c;
    u32 pa = abs(p - a);
    u32 pb = abs(p - b);
    u32 pc = abs(p - c);
    if(pa <= pb && pa <= pc)pr = a;
    else if(pb <= pc)pr = b;
    else pr = c;

    return pr;
}
const u8* createFilteredBuffer(const char* buffer,u32 width,u32 height){
    const u8* reader = (u8*)buffer;
    u8* returnBuffer = (u8*)malloc(sizeof(u8)*(width*height*3));
    u8* writer = returnBuffer;
    u8 prevByte = 0;
    std::vector<u8> prevScanline((width*3),0);
    //Per scanline
    for(u32 y=0;y<height;y++){
        int filterType = u32(reader[0]);
        std::vector<u8> currentScanline(width*3,0);
        reader += 1;
        prevByte = 0;
        if(y > 80 && y < 104){
            std::cout << "Selected y:"<<y << " has filter type "<<filterType<<"\n";
        }
        //Per Pixel
        for(u32 x=0;x<width;x++){
            //Per channel
            for(u32 i=0;i<3;i++){
                /*
                c  |  b
                --------
                a  |  x->current pixel
                */
                u8 curByte = 0;
                u8 a = prevByte;
                u8 b = (y>0?prevScanline[(x*3)+i]:0);
                u8 c = (x>0?prevScanline[((x-1)*3)+i]:0);
                if(filterType == 1){
                    curByte = ((u32(reader[i]) + u32(a)) % 256);
                }else if(filterType == 2){
                    curByte = ((u32(reader[i]) + (b)) % 256);
                }else if(filterType == 4){
                    curByte = ((u32(reader[i]) + paethPredictor(a,b,c)) % 256);
                }else if(filterType == 3){
                    u32 result = floor((u32(a) + u32(b)) / 2.f);
                    curByte = ((u32(reader[i]) + result) % 256);
                }
                else{
                    curByte = (u32(reader[i]) % 256);
                }
                writer[i] = curByte;
                prevByte = curByte;
                currentScanline[(x*3)+i] = curByte;
            }
            reader+=3;
            writer+=3;
        }
        prevScanline = currentScanline;
    }
    return returnBuffer;
}
void deleteBuffer(const u8* buffer){
    free((void*)buffer);
}
void outputToPPM(const char* buffer,u32 width,u32 height){
    
    std::ofstream ofs;
    ofs.open("imageoutput.ppm");
    if(!ofs.is_open()){
        std::cout << "Failed to open imageoutput.ppm\n";
        return;
    }
    const u8* rgbBuffer = createFilteredBuffer(buffer,width,height);
    exportData(rgbBuffer,(width*height*3));
    const u8* reader = rgbBuffer;
    ofs << "P3\n" << width << " " <<  height <<"\n255\n";
    //Per Scanline
    for(u32 y=0;y<height;y++){
        //Per Pixel
        for(u32 x=0;x<width;x++){
            //Per channel
            for(u32 i=0;i<3;i++){
                ofs << u32(reader[i]) << (i==2?"":" ");
            }
            reader += 3;
            ofs << '\n';
        }
    }
    
    deleteBuffer(rgbBuffer);
    ofs.close();
}

int main() {
    const std::string filepath = "..\\stars.png";
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
        std::cout << std::fixed << "image data size: " << parsedData.imageData.size()<< " Bytes\n";
    }else{
        std::cerr << "Failed to load the png " << filepath << "\n";
    }
    
    outputToPPM(parsedData.imageData.data(),parsedData.width,parsedData.height);
    //exportData(parsedData.imageData.data(),parsedData.imageData.size());
    std::cout<<"Program reached end\n";
    system("imageoutput.ppm");
    return 0;
}

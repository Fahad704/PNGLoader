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
struct Pixel{
    u8 R;
    u8 G;
    u8 B;
    u8 getChannel(u8 i){
        if(i == 0){
            return R;
        }else if(i == 1){
            return G;
        }else if(i == 2){
            return B;
        }else{
            return NULL;
        }
    }
    void setChannel(u8 i,u8 value){
        if(i == 0){
            R = value;
        }else if(i == 1){
            G = value;
        }else if(i == 2){
            B = value;
        }
    }
};
struct ParsedData{
    u32 width;
    u32 height;
    u8 bpp;
    u8 colorType;
    u8 compressionMethod;
    u8 filterMethod;
    u8 interlaceMethod;
    std::vector<char> compressedData;
    std::vector<u8> imageData;
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

                bool supported = (
                    (parsedData.bpp == 8) &&
                    (parsedData.colorType == 2) &&
                    (parsedData.filterMethod == 0) &&
                    (parsedData.interlaceMethod == 0) &&
                    (parsedData.compressionMethod == 0) 
                );

                if(!supported){
                    std::cerr<<"PNG file is not supported or does not follow the PNG standard\n";
                    return false;
                }
            }
            else if (chunkType == "IDAT") {
                parsedData.compressedData.insert(parsedData.compressedData.end(), reader, reader + length);
            }else if (chunkType == "IEND"){
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
            reader += 1;
            u16 blockLength = 0;
            if(compressionType == BTYPE_NO_COMPRESSION){
                //Read LEN(2B)
                blockLength = readBigEndian16(&reader[0]);
                std::cout<<"Block length : "<<blockLength<<"\n";
                //skip LEN(2B) and NLEN(2B)
                reader += 4;
                parsedData.imageData.insert(parsedData.imageData.end(),(u8*)reader,(u8*)reader+blockLength);
                //skip data
                reader += blockLength;
            }else if(compressionType == BTYPE_FIXED_HUFFMAN){
                std::cout <<"Unhandled deflate block:BTYPE_FIXED_HUFFMAN\n";
                return true;
            }else if(compressionType == BTYPE_DYNAMIC_HUFFMAN){
                std::cout <<"Unhandled deflate block:BTYPE_DYNAMIC_HUFFMAN\n";
                return true;
            }else{
                std::cout << "ERROR : INVALID COMPRESSION TYPE\n";
                return false;
            }
            if(lastblockbit)std::cout << "(Last Block)\n";
        }
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
const u8* createFilteredBuffer(const u8* buffer,u32 width,u32 height){
    const u8* reader = buffer;
    u8* returnBuffer = (u8*)malloc(sizeof(u8)*(width*height*3));
    u8* writer = returnBuffer;
    Pixel prevPixel;
    std::vector<Pixel> prevScanline(width,{0,0,0});
    //Per scanline
    for(u32 y=0;y<height;y++){
        int filterType = u32(reader[0]);
        std::vector<Pixel> currentScanline(width,{0,0,0});
        //skip filter byte
        reader += 1;
        prevPixel = {0,0,0};
        //Per Pixel
        for(u32 x=0;x<width;x++){
            //Per channel
            Pixel curPixel = {0,0,0};
            for(u32 i=0;i<3;i++){
                u8 value=0;
                u8 a = (x>0?prevPixel.getChannel(i):0);
                u8 b = (y>0?prevScanline[x].getChannel(i):0);
                u8 c = (x>0?prevScanline[x-1].getChannel(i):0);
                if(filterType == 1){
                    value = reader[i] + a;
                }else if(filterType == 2){
                    value = reader[i] + b;
                }else if(filterType == 3){
                    value = reader[i] + floor((a + b)/ 2.f);
                }else if(filterType == 4){
                    value = reader[i] + paethPredictor(a,b,c);
                }else{
                    value = reader[i];
                }
                curPixel.setChannel(i,value);
                writer[i] = curPixel.getChannel(i);
            }
            prevPixel = curPixel;
            currentScanline[x] = curPixel;
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
void outputToPPM(const u8* buffer,u32 width,u32 height){
    
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
    std::cout<<"Program reached end\n";
    //temperory for quick access
    system("imageoutput.ppm");
    return 0;
}

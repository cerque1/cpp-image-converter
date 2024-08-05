#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>
#include <iostream>

using namespace std;

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader{
    // поля заголовка Bitmap File Header
    string signature = "BM";
    unsigned size;
    int reserved = 0000;
    unsigned indent = 54;
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader{
    unsigned size = 40;
    int width;
    int height;
    unsigned short count_planes = 1;
    unsigned short count_bits = 24;
    unsigned compression = 0;
    unsigned count_byte_in_data;
    unsigned horizontal_resolution = 11811;
    unsigned vertical_resolution = 11811;
    unsigned count_used_colors = 0;
    unsigned number_of_significant_colors = 0x1000000;
}
PACKED_STRUCT_END

// функция вычисления отступа по ширине
static int GetBMPStride(int w) {
    unsigned short color_count = 3;
    unsigned short alignment = 4;
    return alignment * ((w * color_count + 3) / alignment);
}

template <typename Arg0, typename... Args>
void WriteArgsToFile(ofstream& file, Arg0 arg, Args... args) {
    file.write(reinterpret_cast<char*>(&arg), sizeof(arg));

    if constexpr (sizeof...(args) != 0) {
        WriteArgsToFile(file, args...);
    }
}
    
template<typename Arg0, typename... Args>
void ReadArgsFromFile(ifstream& file, Arg0 arg, Args... args) {
    file.read((char*)(arg), sizeof(*arg));
    
    if constexpr (sizeof...(args) != 0) {
        ReadArgsFromFile(file, args...);
    }
}

// напишите эту функцию
bool SaveBMP(const Path& file, const Image& image) {
    ofstream out(file, ios::binary | ios::out);
    int padding = GetBMPStride(image.GetStep());

    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;

    int resolution = padding * image.GetHeight();
    file_header.size = resolution + 54;
    info_header.width = image.GetStep();
    info_header.height = image.GetHeight();
    info_header.count_byte_in_data = resolution;

    //Bitmap File Header write

    out << file_header.signature;
    WriteArgsToFile(out, file_header.size, file_header.reserved, file_header.indent, info_header.size, info_header.width, info_header.height, info_header.count_planes,
        info_header.count_bits, info_header.compression, info_header.count_byte_in_data, info_header.horizontal_resolution, info_header.vertical_resolution, info_header.count_used_colors,
        info_header.number_of_significant_colors);

    //body write

    vector<char> buffer(GetBMPStride(image.GetStep()));

    for (int y = image.GetHeight() - 1; y >= 0; --y) {
        for (int x = 0; x < image.GetStep(); ++x) {
            const Color& pixel = image.GetPixel(x, y);
            buffer[x * 3 + 0] = static_cast<char>(pixel.b);
            buffer[x * 3 + 1] = static_cast<char>(pixel.g);
            buffer[x * 3 + 2] = static_cast<char>(pixel.r);
        }
        out.write(buffer.data(), GetBMPStride(image.GetStep()));

        for (int i = padding - (image.GetStep() * 3); i < 0; ++i) {
            out << static_cast<char>(byte());
        }
    }

    if (out.fail()) {
        return false;
    }

    return true;
}

// напишите эту функцию
Image LoadBMP(const Path& file) {
    ifstream in(file, ios::in | ios::binary);

    //Bitmap File Header read
    std::string valid_signature = "BM";
    vector<char> format(2);
    unsigned size_header, ident;
    int reserved;

    in.read(format.data(), 2);

    if (in.fail()) {
        return {};
    }

    if (!equal(format.begin(), format.end(), valid_signature.begin(), valid_signature.end())) {
        return {};
    }

    //Bitmap Info Header read
    unsigned short count_planes, count_bit, count_bits;
    unsigned compression, count_byte_in_data, horizontal_resolution, vertical_resolution, count_used_colors, number_of_significant_colors, size_info;
    int w, h;

    ReadArgsFromFile(in, &size_header, &reserved, &ident, &size_info, &w, &h, &count_planes, &count_bits, &compression, &count_byte_in_data, &horizontal_resolution, 
        &vertical_resolution, &count_used_colors, &number_of_significant_colors);

    if (in.fail()) {
        return {};
    }
    //body read
    Image image(w, h, Color::Black());
    vector<char> buffer(GetBMPStride(w));

    for (int y = h - 1; y >= 0; --y) {
        Color* line = image.GetLine(y);
        in.read(buffer.data(), GetBMPStride(w));

        for (int x = 0; x < w; ++x) {
            line[x].b = static_cast<byte>(buffer[x * 3 + 0]);
            line[x].g = static_cast<byte>(buffer[x * 3 + 1]);
            line[x].r = static_cast<byte>(buffer[x * 3 + 2]);
        }
    }

    return image;
}

}  // namespace img_lib
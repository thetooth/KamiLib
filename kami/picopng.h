// picoPNG version 20101224
// Copyright (c) 2005-2010 Lode Vandevenne
// 
// picoPNG is a PNG decoder in one C++ function of around 500 lines. Use picoPNG for
// programs that need only 1 .cpp file. Since it's a single function, it's very limited,
// it can convert a PNG to raw pixel data either converted to 32-bit RGBA color or
// with no color conversion at all. For anything more complex, another tiny library
// is available: LodePNG (lodepng.c(pp)), which is a single source and header file.
// Apologies for the compact code style, it's to make this tiny.

#include <algorithm>
#include <functional>
#include <vector>
int decodePNG(std::vector<unsigned char>& out_image, unsigned long& image_width, unsigned long& image_height, const unsigned char* in_png, size_t in_size, bool convert_to_rgba32 = true);

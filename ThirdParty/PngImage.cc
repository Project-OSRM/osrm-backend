/*===========================================================================
  This library is released under the MIT license.

Copyright (c) 2009 Juha Nieminen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
=============================================================================*/

#include "PngImage.hh"

#include <png.h>

#include <cstdio>
#include <csetjmp>
#include <cstring>

PngImage::PngImage():
    mImageData(0), mImageWidth(0), mImageHeight(0)
{}

PngImage::PngImage(int width, int height,
                   unsigned char red, unsigned char green,
                   unsigned char blue, unsigned char alpha):
    mImageData(0), mImageWidth(0), mImageHeight(0)
{
    createImage(width, height, red, green, blue, alpha);
}

PngImage::~PngImage()
{
    delete[] mImageData;
}

PngImage::PngImage(const PngImage& rhs):
    mImageData(0),
    mImageWidth(rhs.mImageWidth), mImageHeight(rhs.mImageHeight)
{
    if(rhs.mImageData)
    {
        const int size = mImageWidth*mImageHeight*4;
        mImageData = new unsigned char[size];
        std::memcpy(mImageData, rhs.mImageData, size);
    }
}

PngImage& PngImage::operator=(const PngImage& rhs)
{
    if(mImageData != rhs.mImageData)
    {
        delete[] mImageData;
        mImageWidth = rhs.mImageWidth;
        mImageHeight = rhs.mImageHeight;
        if(rhs.mImageData)
        {
            const int size = mImageWidth*mImageHeight*4;
            mImageData = new unsigned char[size];
            std::memcpy(mImageData, rhs.mImageData, size);
        }
        else
            mImageData = 0;
    }
    return *this;
}

void PngImage::deleteImageData()
{
    delete[] mImageData;
    mImageData = 0;
    mImageWidth = mImageHeight = 0;
}

void PngImage::createImage(int width, int height,
                           unsigned char red, unsigned char green,
                           unsigned char blue, unsigned char alpha)
{
    assert(width > 0 && height > 0);

    delete[] mImageData;
    const int size = width*height*4;
    mImageData = new unsigned char[size];
    mImageWidth = width;
    mImageHeight = height;

    for(int i = 0; i < size; i += 4)
    {
        mImageData[i] = red;
        mImageData[i+1] = green;
        mImageData[i+2] = blue;
        mImageData[i+3] = alpha;
    }
}

namespace
{
    struct FilePtr
    {
        std::FILE* fp;

        FilePtr(): fp(0) {}
        ~FilePtr() { if(fp) std::fclose(fp); }
    };

    struct PNGStructPtrs
    {
        png_structp read;
        png_structp write;
        png_infop info;

        PNGStructPtrs(): read(0), write(0), info(0) {}

        ~PNGStructPtrs()
        {
            if(read)
            {
                if(info) png_destroy_read_struct(&read, &info, 0);
                else png_destroy_read_struct(&read, 0, 0);
            }
            else if(write)
            {
                if(info) png_destroy_write_struct(&write, &info);
                else png_destroy_write_struct(&write, 0);
            }
        }
    };

    void readPngData(PNGStructPtrs& ptrs,
                     unsigned char* & imageData,
                     int& imageWidth, int& imageHeight)
    {
        png_read_info(ptrs.read, ptrs.info);
		
		imageWidth = png_get_image_width(ptrs.read, ptrs.info);
		imageHeight = png_get_image_height(ptrs.read, ptrs.info);
        imageData = new unsigned char[4 * imageWidth * imageHeight];

        png_set_expand(ptrs.read);
        png_set_filler(ptrs.read, 0xff, PNG_FILLER_AFTER);
        png_set_palette_to_rgb(ptrs.read);
        png_set_gray_to_rgb(ptrs.read);
        png_set_strip_16(ptrs.read);

        unsigned char* addr = imageData;
        for(int i = 0; i < imageHeight; ++i)
        {
            png_read_rows(ptrs.read, (png_bytepp) &addr, 0, 1);
            addr += imageWidth * 4;
        }

        png_read_end(ptrs.read, ptrs.info);
    }
}

bool PngImage::loadImage(const char* filename, bool printErrorMsg)
{
    deleteImageData();

    FilePtr iFile;
    iFile.fp = std::fopen(filename, "rb");
    if(!iFile.fp)
    {
        if(printErrorMsg)
        {
            std::fprintf(stderr, "Can't open ");
            std::perror(filename);
        }
        return false;
    }

    png_byte header[8];
    std::fread(header, 1, 8, iFile.fp);
    if(png_sig_cmp(header, 0, 8))
    {
        if(printErrorMsg)
            std::fprintf(stderr, "%s is not a PNG file.\n", filename);
        return false;
    }
    std::fseek(iFile.fp, 0, SEEK_SET);

    PNGStructPtrs ptrs;
    ptrs.read = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if(!ptrs.read)
    {
        if(printErrorMsg)
            std::fprintf(stderr, "Initializing libpng failed.\n");
        return false;
    }

    ptrs.info = png_create_info_struct(ptrs.read);
    if(!ptrs.info)
    {
        if(printErrorMsg)
            std::fprintf(stderr, "Initializing libpng failed.\n");
        return false;
    }

    if(setjmp(png_jmpbuf(ptrs.read))) return false;

    png_init_io(ptrs.read, iFile.fp);
    readPngData(ptrs, mImageData, mImageWidth, mImageHeight);
    return true;
}

bool PngImage::loadImage(const std::string& filename, bool printErrorMsg)
{
    return loadImage(filename.c_str(), printErrorMsg);
}

namespace
{
    const unsigned char* gPngData;
    size_t gPngDataSize, gCurrentPngDataIndex;

    void pngDataReader(png_structp png_ptr, png_bytep data,
                       png_size_t length)
    {
        if(gCurrentPngDataIndex + length > gPngDataSize)
            png_error(png_ptr, "Read error");
        else
        {
            std::memcpy(data, gPngData+gCurrentPngDataIndex, length);
            gCurrentPngDataIndex += length;
        }
    }
}

bool PngImage::loadImageFromMemory(const unsigned char* data, unsigned dataSize)
{
    deleteImageData();

    gPngData = data;
    gPngDataSize = dataSize;
    gCurrentPngDataIndex = 0;

    PNGStructPtrs ptrs;
    ptrs.read = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if(!ptrs.read) return false;

    png_set_read_fn(ptrs.read, 0, pngDataReader);

    ptrs.info = png_create_info_struct(ptrs.read);
    if(!ptrs.info) return false;

    if(setjmp(png_jmpbuf(ptrs.read))) return false;
    readPngData(ptrs, mImageData, mImageWidth, mImageHeight);
    return true;
}

bool PngImage::saveImage(const char* filename, bool printErrorMsg) const
{
    if(!mImageData) return true;

    FilePtr oFile;
    oFile.fp = std::fopen(filename, "wb");
    if(!oFile.fp)
    {
        if(printErrorMsg)
        {
            std::fprintf(stderr, "Can't write to ");
            std::perror(filename);
        }
        return false;
    }

    PNGStructPtrs ptrs;
    ptrs.write = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if(!ptrs.write)
    {
        if(printErrorMsg)
            std::fprintf(stderr, "Initializing libpng failed.\n");
        return false;
    }

    ptrs.info = png_create_info_struct(ptrs.write);
    if(!ptrs.info)
    {
        if(printErrorMsg)
            std::fprintf(stderr, "Initializing libpng failed.\n");
        return false;
    }

    if(setjmp(png_jmpbuf(ptrs.write))) return false;

    png_init_io(ptrs.write, oFile.fp);

    png_set_IHDR(ptrs.write, ptrs.info, mImageWidth, mImageHeight, 8,
                 PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(ptrs.write, ptrs.info);

    for(int i = 0; i < mImageHeight; ++i)
        png_write_row(ptrs.write, mImageData + i*mImageWidth*4);

    png_write_end(ptrs.write, 0);
    return true;
}

bool PngImage::saveImage(const std::string& filename, bool printErrorMsg) const
{
    return saveImage(filename.c_str(), printErrorMsg);
}

void PngImage::drawImageAt(const PngImage& src, int x, int y)
{
    if(!mImageData || !src.mImageData) return;

    for(int yInd = 0; yInd < src.mImageHeight; ++yInd)
    {
        const int destY = y + yInd;
        if(destY >= 0 && destY < mImageHeight)
        {
            unsigned char* srcPtr = src.mImageData + 4*yInd*src.mImageWidth;
            for(int xInd = 0; xInd < src.mImageWidth; ++xInd)
            {
                const int destX = x + xInd;
                if(destX >= 0 && destX < mImageWidth)
                {
                    unsigned char* destPtr =
                        mImageData + 4*(destX + destY*mImageWidth);
                    for(int i = 0; i < 4; ++i)
                        destPtr[i] = srcPtr[i];
                }
                srcPtr += 4;
            }
        }
    }
}

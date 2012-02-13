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

#ifndef PNG_IMAGE_HH
#define PNG_IMAGE_HH

#include <string>
#include <cassert>

class PngImage
{
 public:
    // Constructor, destructor and copying
    // -----------------------------------
    PngImage();
    PngImage(int width, int height,
             unsigned char red = 0, unsigned char green = 0,
             unsigned char blue = 0, unsigned char alpha = 255);
    ~PngImage();

    // If there is image data, these will perform deep copying.
    PngImage(const PngImage&);
    PngImage& operator=(const PngImage&);

    // Delete the image data, freeing the memory
    void deleteImageData();


    // Load image
    // ----------
    bool loadImage(const char* filename, bool printErrorMsg = true);
    bool loadImage(const std::string& filename, bool printErrorMsg = true);

    bool loadImageFromMemory(const unsigned char* data, unsigned dataSize);


    // Create image (rather than loading one)
    // --------------------------------------
    void createImage(int width, int height,
                     unsigned char red = 0, unsigned char green = 0,
                     unsigned char blue = 0, unsigned char alpha = 255);


    // Save image
    // ----------
    bool saveImage(const char* filename, bool printErrorMsg = true) const;
    bool saveImage(const std::string& filename,
                   bool printErrorMsg = true) const;


    // Image data access
    // -----------------
    int width() const;
    int height() const;

    unsigned char* rawImagePtr();
    const unsigned char* rawImagePtr() const;

    void setPixel(int x, int y,
                  unsigned char red, unsigned char green, unsigned char blue,
                  unsigned char alpha = 255);

    unsigned char getColorComponent(int x, int y, int component) const;


    void drawImageAt(const PngImage&, int x, int y);



//--------------------------------------------------------------------------
 private:
    unsigned char* mImageData;
    int mImageWidth, mImageHeight;
};

inline int PngImage::width() const { return mImageWidth; }
inline int PngImage::height() const { return mImageHeight; }

inline unsigned char* PngImage::rawImagePtr()
{ return mImageData; }

inline const unsigned char* PngImage::rawImagePtr() const
{ return mImageData; }

inline void PngImage::setPixel(int x, int y,
                               unsigned char red, unsigned char green,
                               unsigned char blue, unsigned char alpha)
{
    assert(x >= 0 && mImageWidth > x && y >= 0 && mImageHeight > y);

    unsigned char* ptr = mImageData + 4 * (x + y*mImageWidth);
    ptr[0] = red;
    ptr[1] = green;
    ptr[2] = blue;
    ptr[3] = alpha;
}

inline unsigned char
PngImage::getColorComponent(int x, int y, int component) const
{
    assert(x >= 0 && mImageWidth > x && y >= 0 && mImageHeight > y);
    assert(component >= 0 && component < 4);

    return mImageData[4 * (x + y*mImageWidth) + component];
}

#endif

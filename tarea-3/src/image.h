#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <stdexcept>
#include <cstring> // For memcpy
#include "lib/stb_image.h"
#include "lib/stb_image_write.h"

using namespace std;

class Image {
public:
    // self allocate tells if we should allocate the image ourselves or not
    Image(const string& filename) : self_allocated(false) {
        // validate that is 8bits only
        if (stbi_is_16_bit(filename.c_str())) {
            throw runtime_error("16-bit images are not supported. Only 8-bit per channel images are allowed.");
        }

        _data = stbi_load(filename.c_str(), &_width, &_height, &_channels, 0);

        // no image there
        if (_data == nullptr) {
            throw runtime_error("Failed to load image: " + filename);
        }
    }

    Image(int w, int h, int c = 3) : _width(w), _height(h), _channels(c), self_allocated(true) {
        if (w <= 0 || h <= 0 || c <= 0) {
            throw invalid_argument("Image dimensions and _channels must be positive.");
        }

        size_t size = (size_t)w * h * c;
        _data = (unsigned char*)malloc(size);
    }

    ~Image() {
        if (_data) {
            if (self_allocated) {
                free(_data);
            } else {
                stbi_image_free(_data);
            }
        }
    }

    int width() const { return _width; }
    int height() const { return _height; }
    int channels() const { return _channels; }

    unsigned char get(int x, int y, int c) const {
        if (x < 0 || x >= _width || y < 0 || y >= _height || c < 0 || c >= _channels) {
            throw invalid_argument("Pixel coordinates or channel are out of bounds");
        }
        return _data[(y * _width + x) * _channels + c];
    }

    void set(int x, int y, int c, unsigned char value) {
        if (x < 0 || x >= _width || y < 0 || y >= _height || c < 0 || c >= _channels) {
            throw invalid_argument("Pixel coordinates or channel are out of bounds");
        }
        _data[(y * _width + x) * _channels + c] = value;
    }

    void save(const string& filename) {
        if (stbi_write_png(filename.c_str(), _width, _height, _channels, _data, _width * _channels) == 0)
            throw runtime_error("Pixel coordinates or channel are out of bounds");
    }


private:
    int _width;
    int _height;
    int _channels;
    unsigned char* _data;
    bool self_allocated;
};

#endif // IMAGE_H

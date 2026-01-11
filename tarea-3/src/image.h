#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <stdexcept>
#include <cstring> // For memcpy
#include "lib/stb_image.h"
#include "lib/stb_image_write.h"

#include <cctype>

using namespace std;

class Image {
private:
    int _width;
    int _height;
    int _channels;
    unsigned char* _data;
    // used when freeing the memory. If we create a new image we dont use stb_image
    bool self_allocated;

public:
    // constructor that loads image from storage
    Image(const string& filename) : self_allocated(false) {
        // validate that is 8bits only
        if (stbi_is_16_bit(filename.c_str())) {
            throw runtime_error("16-bit images not supported.");
        }

        _data = stbi_load(filename.c_str(), &_width, &_height, &_channels, 0);

        if (_data == nullptr)
            throw runtime_error("error when loading: " + filename);
    }

    // constructor that creates a new image from scratch without loading from storage
    Image(int w, int h, int c = 3) : _width(w), _height(h), _channels(c), self_allocated(true) {
        if (w <= 0 || h <= 0 || c <= 0) {
            throw invalid_argument("dimensions and _channels must be positive.");
        }

        size_t size = (size_t)w * h * c;
        _data = (unsigned char*)malloc(size);
    }

    // destructor. self_allocated tells us if using free or stbi_image_free
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

    // check boundaries and return value
    unsigned char get(int x, int y, int c) const {
        if (x < 0 || x >= _width || y < 0 || y >= _height || c < 0 || c >= _channels)
            throw invalid_argument("get: out of bounds");

        return _data[(y * _width + x) * _channels + c];
    }

    // check boundaries and set value
    void set(int x, int y, int c, unsigned char value) {
        if (x < 0 || x >= _width || y < 0 || y >= _height || c < 0 || c >= _channels)
            throw invalid_argument("set: out of bounds");

        _data[(y * _width + x) * _channels + c] = value;
    }

    // saves an image. we get the extension and save accordingly. 
    // this wasnt specified on the pdf but we assume we should be able to save in any format
    void save(const string& filename) {
        string ext = filename.substr(filename.find_last_of(".") + 1);

        if (ext == "png") {
            stbi_write_png(filename.c_str(), _width, _height, _channels, _data, _width * _channels);
        } else if (ext == "jpg" || ext == "jpeg") {
            stbi_write_jpg(filename.c_str(), _width, _height, _channels, _data, 100);
        } else if (ext == "bmp") {
            stbi_write_bmp(filename.c_str(), _width, _height, _channels, _data);
        } else {
            throw runtime_error("unsupported extension: " + ext);
        }
    }
};

#endif // IMAGE_H

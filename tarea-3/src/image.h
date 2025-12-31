#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <stdexcept>
#include <cstring> // For memcpy
#include "lib/stb_image.h"
#include "lib/stb_image_write.h"

// Custom exception for out of bounds access
class ImageBoundsError : public std::runtime_error {
public:
    ImageBoundsError(const std::string& message) : std::runtime_error(message) {}
};

class Image {
public:
    // Constructor to load an image from a file
    Image(const std::string& filename) : self_allocated(false) {
        if (stbi_is_16_bit(filename.c_str())) {
            throw std::runtime_error("16-bit images are not supported. Only 8-bit per channel images are allowed.");
        }
        data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
        if (data == nullptr) {
            throw std::runtime_error("Failed to load image: " + filename);
        }
    }

    // Constructor to create a blank image
    Image(int w, int h, int c = 3) : width(w), height(h), channels(c), self_allocated(true) {
        if (w <= 0 || h <= 0 || c <= 0) {
            throw std::invalid_argument("Image dimensions and channels must be positive.");
        }
        size_t size = (size_t)w * h * c;
        data = (unsigned char*)malloc(size);
        if (data == nullptr) {
            throw std::runtime_error("Failed to allocate memory for image.");
        }
    }

    // Destructor
    ~Image() {
        if (data) {
            if (self_allocated) {
                free(data);
            } else {
                stbi_image_free(data);
            }
        }
    }

    // Copy constructor
    Image(const Image& other) : width(other.width), height(other.height), channels(other.channels), self_allocated(true) {
        size_t size = (size_t)width * height * channels;
        data = (unsigned char*)malloc(size);
        if (data == nullptr) {
            throw std::runtime_error("Failed to allocate memory for image copy.");
        }
        memcpy(data, other.data, size);
    }

    // Copy assignment operator
    Image& operator=(const Image& other) {
        if (this == &other) {
            return *this;
        }

        if (data) {
            if (self_allocated) {
                free(data);
            } else {
                stbi_image_free(data);
            }
        }

        width = other.width;
        height = other.height;
        channels = other.channels;
        self_allocated = true;
        size_t size = (size_t)width * height * channels;
        data = (unsigned char*)malloc(size);
        if (data == nullptr) {
            throw std::runtime_error("Failed to allocate memory for image assignment.");
        }
        memcpy(data, other.data, size);

        return *this;
    }

    // Save the image to a file
    bool save(const std::string& filename) {
        return stbi_write_png(filename.c_str(), width, height, channels, data, width * channels) != 0;
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getChannels() const { return channels; }
    unsigned char* getData() const { return data; }

    unsigned char getPixelValue(int x, int y, int c) const {
        if (x < 0 || x >= width || y < 0 || y >= height || c < 0 || c >= channels) {
            throw ImageBoundsError("Pixel coordinates or channel are out of bounds");
        }
        return data[(y * width + x) * channels + c];
    }

    void setPixelValue(int x, int y, int c, unsigned char value) {
        if (x < 0 || x >= width || y < 0 || y >= height || c < 0 || c >= channels) {
            throw ImageBoundsError("Pixel coordinates or channel are out of bounds");
        }
        data[(y * width + x) * channels + c] = value;
    }

private:
    int width;
    int height;
    int channels;
    unsigned char* data;
    bool self_allocated;
};

#endif // IMAGE_H
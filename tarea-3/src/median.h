#ifndef MEDIAN_H
#define MEDIAN_H

#include "image.h"
#include <vector>
#include <algorithm>
#include <stdexcept>

#include <bits/stdc++.h>
using namespace std;

int partition(vector<unsigned char>& arr, int l, int r) {
    int x = arr[r], i = l;
    for (int j = l; j <= r - 1; j++) {
        if (arr[j] <= x) {
            swap(arr[i], arr[j]);
            i++;
        }
    }
    swap(arr[i], arr[r]);
    return i;
}

int kthSmallest(vector<unsigned char>& arr, int l, int r, int k) {
    if (k > 0 && k <= r - l + 1) {
        int index = partition(arr, l, r);
        if (index - l == k - 1)
            return arr[index];

        if (index - l > k - 1) 
            return kthSmallest(arr, l, index - 1, k);

        return kthSmallest(arr, index + 1, r, 
                            k - index + l - 1);
    }

    return -1;
}

static unsigned char get_median(vector<unsigned char>& vec) {
    int n = vec.size();
    return kthSmallest(vec,0, n - 1, n/2);
}

class MedianKernel {
public:
    // Constructor validates and sets the kernel size
    MedianKernel(int size) : kernel_size(size) {
        if (size <= 0 || size % 2 == 0) {
            throw invalid_argument("Kernel size must be a positive, odd integer.");
        }
    }

    // Applies the median filter to an image and returns a new image
    Image apply(const Image& input_image) const {
        Image output_image(input_image.getWidth(), input_image.getHeight(), input_image.getChannels());
        int kernel_half = kernel_size / 2;

        for (int y = 0; y < input_image.getHeight(); y++) {
            for (int x = 0; x < input_image.getWidth(); x++) {
                for (int c = 0; c < input_image.getChannels(); c++) {
                    if (c == 3) { // Alpha channel, just copy it
                        unsigned char value = input_image.getPixelValue(x, y, c);
                        output_image.setPixelValue(x, y, c, value);
                        continue;
                    }

                    vector<unsigned char> neighborhood;
                    for (int ky = -kernel_half; ky <= kernel_half; ky++) {
                        for (int kx = -kernel_half; kx <= kernel_half; kx++) {
                            int pixel_y = y + ky;
                            int pixel_x = x + kx;

                            if (pixel_y >= 0 && pixel_y < input_image.getHeight() &&
                                pixel_x >= 0 && pixel_x < input_image.getWidth()) {
                                neighborhood.push_back(input_image.getPixelValue(pixel_x, pixel_y, c));
                            }
                        }
                    }
                    output_image.setPixelValue(x, y, c, get_median(neighborhood));
                }
            }
        }
        return output_image;
    }

private:
    int kernel_size;
};

#endif // MEDIAN_H

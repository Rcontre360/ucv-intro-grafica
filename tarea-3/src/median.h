#ifndef MEDIAN_H
#define MEDIAN_H

#include "image.h"
#include <vector>
#include <algorithm>
#include <stdexcept>

#include <bits/stdc++.h>

using namespace std;

// moves all elements bigger than arr[r] to the right and lower than arr[r] to the left
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

// quick select recursive from https://www.geeksforgeeks.org/dsa/quickselect-algorithm/
// not used, just shown here as reference for the iterative version
int quick_select(vector<unsigned char>& arr, int l, int r, int k) {
    if (k > 0 && k <= r - l + 1) {
        int index = partition(arr, l, r);
        if (index - l == k - 1)
            return arr[index];

        if (index - l > k - 1) 
            return quick_select(arr, l, index - 1, k);

        return quick_select(arr, index + 1, r, 
                            k - index + l - 1);
    }

    return -1;
}

//iterative quick_select. I developed this using the recursive version and compared their results
int quick_select_iter(vector<unsigned char>& arr, int l, int r, int k) {
    while (k > 0 && k <= r - l + 1){
        int index = partition(arr, l, r);
        if (index - l == k - 1)
            return arr[index];

        if (index - l > k - 1) {
            r = index - 1;
        } else {
            k = k - (index - l + 1); 
            l = index + 1;
        }
    }

    return -1;
}

unsigned char median(vector<unsigned char>& vec) {
    int n = vec.size();
    unsigned char res = quick_select_iter(vec,0,n-1,n/2);
    return res;
}

class Median {
private:
    int kernel_size;

public:
    // constructor validates and sets the kernel size
    Median(int size) : kernel_size(size) {
        if (size <= 0 || size % 2 == 0) {
            throw invalid_argument("Kernel size must be a positive, odd integer.");
        }
    }

    // applies the median filter to an image and returns a new image
    Image apply(const Image& input_image) const {
        // create new image
        Image out(input_image.width(), input_image.height(), input_image.channels());
        int mid = kernel_size / 2;

        for (int y = 0; y < input_image.height(); y++) {
            for (int x = 0; x < input_image.width(); x++) {
                for (int c = 0; c < input_image.channels(); c++) {
                    // we just copy alpha if any
                    if (c == 3) { 
                        unsigned char value = input_image.get(x, y, c);
                        out.set(x, y, c, value);
                        continue;
                    }

                    // we run over the kernel window on the image
                    vector<unsigned char> all;
                    for (int ky = -mid; ky <= mid; ky++) {
                        for (int kx = -mid; kx <= mid; kx++) {
                            int pixel_y = y + ky;
                            int pixel_x = x + kx;

                            // if the index is valid, we add it to the kernel window set of elements
                            if (pixel_y >= 0 && pixel_y < input_image.height() &&
                                pixel_x >= 0 && pixel_x < input_image.width()) {
                                all.push_back(input_image.get(pixel_x, pixel_y, c));
                            }
                        }
                    }
                    // we set the value on that pixel with the median
                    out.set(x, y, c, median(all));
                }
            }
        }
        return out;
    }
};

#endif 

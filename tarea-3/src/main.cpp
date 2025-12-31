#include <iostream>
#include "image.h"
#include "median.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input_image> <output_image> <kernel_size>\n", argv[0]);
        return 1;
    }

    try {
        char* input_path = argv[1];
        char* output_path = argv[2];
        int kernel_size = atoi(argv[3]);

        Image input_image(input_path);
        printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n",
               input_image.getWidth(), input_image.getHeight(), input_image.getChannels());

        MedianKernel kernel(kernel_size);
        Image output_image = kernel.apply(input_image);

        if (output_image.save(output_path)) {
            printf("Image saved to %s\n", output_path);
        } else {
            printf("Error saving image to %s\n", output_path);
        }

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

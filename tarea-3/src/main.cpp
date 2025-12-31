#include <iostream>
#include "image.h"
#include "median.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input> <out> <kernel_size>\n", argv[0]);
        return 1;
    }

    try {
        char* input_path = argv[1];
        char* output_path = argv[2];
        int kernel_size = atoi(argv[3]);

        Image input(input_path);
        Median kernel(kernel_size);

        Image out = kernel.apply(input);

        out.save(output_path);
        printf("saved to %s\n", output_path);

    } catch (const exception& e) {
        cerr << "error: " << e.what() << endl;
        return 1;
    }

    return 0;
}

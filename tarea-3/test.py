import cv2
import sys

def main():
    if len(sys.argv) != 4:
        print("Usage: python test.py <input_image> <output_image> <kernel_size>")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]
    kernel_size = int(sys.argv[3])

    if kernel_size % 2 == 0:
        print("Kernel size must be an odd number.")
        sys.exit(1)

    try:
        # Read the image
        img = cv2.imread(input_path)
        if img is None:
            print(f"Error: Could not read image from {input_path}")
            sys.exit(1)

        # Apply median filter
        median_img = cv2.medianBlur(img, kernel_size)

        # Save the output image
        cv2.imwrite(output_path, median_img)
        print(f"Median filter applied with kernel size {kernel_size}. Output saved to {output_path}")

    except Exception as e:
        print(f"An error occurred: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()

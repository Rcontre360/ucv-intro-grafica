import os
from PIL import Image
import numpy as np
import png # pypng

def create_image(width, height, mode, filename, is_16bit=False):
    if is_16bit and '.png' in filename:
        if mode == 'RGB':
            # Create a 16-bit RGB image (48-bit)
            array = np.zeros((height, width * 3), dtype=np.uint16)
            for y in range(height):
                for x in range(width):
                    array[y, x * 3 + 0] = int((x / width) * 65535)
                    array[y, x * 3 + 1] = int((y / height) * 65535)
                    array[y, x * 3 + 2] = int(((x + y) / (width + height)) * 65535)
            with open(filename, 'wb') as f:
                writer = png.Writer(width, height, greyscale=False, bitdepth=16, alpha=False)
                writer.write(f, array.tolist())
        elif mode == 'RGBA':
            # Create a 16-bit RGBA image (64-bit)
            array = np.zeros((height, width * 4), dtype=np.uint16)
            for y in range(height):
                for x in range(width):
                    array[y, x * 4 + 0] = int((x / width) * 65535)
                    array[y, x * 4 + 1] = int((y / height) * 65535)
                    array[y, x * 4 + 2] = int(((x + y) / (width + height)) * 65535)
                    array[y, x * 4 + 3] = 49151 # 75% alpha
            with open(filename, 'wb') as f:
                writer = png.Writer(width, height, greyscale=False, bitdepth=16, alpha=True)
                writer.write(f, array.tolist())
        else: # Grayscale
            array = np.zeros((height, width), dtype=np.uint16)
            for y in range(height):
                for x in range(width):
                    array[y,x] = int((x/width) * 65535)
            with open(filename, 'wb') as f:
                writer = png.Writer(width, height, greyscale=True, bitdepth=16)
                writer.write(f, array.tolist())
    else: # 8bit or other formats
        img = Image.new(mode, (width, height))
        pixels = img.load()
        for x in range(width):
            for y in range(height):
                if mode == 'RGB':
                    pixels[x, y] = (x % 256, y % 256, (x + y) % 256)
                elif mode == 'RGBA':
                    pixels[x, y] = (x % 256, y % 256, (x + y) % 256, 192)
                elif mode == 'L':
                    pixels[x,y] = (x+y) % 256
        img.save(filename)
    
    print(f"Created {filename}")

if __name__ == "__main__":
    if not os.path.exists("images"):
        os.makedirs("images")

    # 16-bit images
    create_image(256, 256, 'RGB', "images/gradient_16bit.png", is_16bit=True)
    create_image(256, 256, 'RGBA', "images/gradient_16bit_alpha.png", is_16bit=True)
    create_image(256, 256, 'L', "images/gradient_16bit_gray.png", is_16bit=True)


    # 8-bit images
    create_image(256, 256, 'RGB', "images/gradient_8bit.png")
    create_image(256, 256, 'RGBA', "images/gradient_8bit_alpha.png")
    create_image(256, 256, 'RGB', "images/gradient_8bit.jpg")
    create_image(256, 256, 'RGB', "images/gradient_8bit.bmp")
    create_image(256, 256, 'L', "images/gradient_8bit_gray.png")

    # Solid color images to test alpha
    img = Image.new('RGBA', (100, 100), (255, 0, 0, 128))
    img.save('images/red_alpha.png')
    print("Created images/red_alpha.png")

    img = Image.new('RGB', (100, 100), (0, 255, 0))
    img.save('images/green.jpg')
    print("Created images/green.jpg")

    img = Image.new('RGB', (100, 100), (0, 0, 255))
    img.save('images/blue.bmp')
    print("Created images/blue.bmp")
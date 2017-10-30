import os, sys
from PIL import Image
import PIL.ImageOps

# input parameters' check
if (len(sys.argv) != 3):
    print("Error: wrong number of input parameters")
    exit(1)
    
input_file = sys.argv[1]
output_file = sys.argv[2]
if (os.path.isfile(input_file) != True):
    print("Error: the specified file does not exists")
    exit(1)
print("Converting: " + input_file + " --> " + output_file)

# load the image
image = Image.open(input_file)
width, height = image.size

# input image check
if (height%8 != 0):
    print("Error: image height must be multiple of 8")
    exit(1)

# invert black and white colors on the original image
inverted_image = PIL.ImageOps.invert(image)
# convert image to black and white (1 bit color depth)
bw_image = inverted_image.convert('1')

# generate the proper pixel map for the OLED
pixels = []
for page in range(0, int(height/8)):
    for column in range(0, width):
        pixel_byte = 0
        for bit in range(0, 8):
            pixel_byte |= ((0x01 << bit) & bw_image.getpixel((column, (bit + page * 8))))
        pixels.append(pixel_byte)

# write header file
filename = os.path.splitext(os.path.basename(input_file))[0]
header_file = open(output_file, "w")
header_file.write("#ifndef _" + filename.upper() + "_H_")
header_file.write("\n")
header_file.write("#define _" + filename.upper() + "_H_")
header_file.write("\n")
header_file.write("\n")
header_file.write("const uint8_t " + filename + "_width = " + str(width) + ";")
header_file.write("\n")
header_file.write("const uint8_t " + filename + "_height = " + str(height) + ";")
header_file.write("\n")
header_file.write("\n")

header_file.write("const uint8_t " + filename + "_data[" + str(len(pixels)) + "] = {")
header_file.write("\n")
header_file.write("\t")
for byte in range(len(pixels)):
    header_file.write("0x" + format(pixels[byte], '02x'))
    if ((byte+1) != (width*height/8)):
        header_file.write(", ")
        if ((byte+1)%16 == 0):
            header_file.write("\n")
            header_file.write("\t")

header_file.write("\n")
header_file.write("};")
header_file.write("\n")
header_file.write("\n")
header_file.write("#endif // _" + filename.upper() + "_H_")
header_file.close()

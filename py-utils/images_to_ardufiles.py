import os
from tqdm import tqdm
from PIL import Image

input_images_dir = "./output_images"

output_files_dir = "./ardu_images"


def color_to_hex(r, g, b):
    c_value = (r << 16) | (g << 8) | b
    return (f"0x{c_value:06X}")


def img_to_txt_file(input_img_path, output_img_path):
    image = Image.open(input_img_path)
    rgb_im = image.convert('RGB')
    width, height = image.size
    index = 0
    with open(output_img_path, "wb") as output:
        for y in range(height):
            for x in range(width):
                r, g, b = rgb_im.getpixel((x, y))
                print(f"{index=}, {r=}, {g=}, {b=}")
                # if you want it like text
                output.write(r.to_bytes())
                output.write(g.to_bytes())
                output.write(b.to_bytes())
                index += 1
                # output.write(bytes(pixel[:3]))
            # output.write("\n".encode()) #if you want it text like


def image_files(dir):
    images = [file for file in os.listdir(dir) if file.endswith(".bmp")]
    images.sort()
    for filepath in tqdm(images, total=len(images), desc="Processing frames", unit="frames"):
        yield os.path.join(dir, filepath)


def main():
    os.makedirs(output_files_dir, exist_ok=True)
    for i, img_path in enumerate(image_files(input_images_dir)):
        output_name = os.path.join(output_files_dir, f"img_{i}")
        img_to_txt_file(input_img_path=img_path, output_img_path=output_name)


if __name__ == "__main__":
    main()

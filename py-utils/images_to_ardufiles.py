import argparse
import logging
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import numpy as np
from tqdm import tqdm
from PIL import Image

log = logging.getLogger(__name__)
input_images_dir = "./output_images"

output_files_dir = "./ardu_images"


def color_to_hex(r, g, b):
    c_value = (r << 16) | (g << 8) | b
    return (f"0x{c_value:06X}")


def img_to_txt_file(input_img_path: Path, output_img_path: Path):
    image = Image.open(input_img_path)
    rgb_im = image.convert('RGB')
    arr = np.array(rgb_im).flatten()
    output_img_path.write_bytes(arr.tobytes())


def convert_single_image(progress: tqdm, input_file: Path, output_file: Path) -> None:
    progress.set_postfix(file=input_file.name)
    img_to_txt_file(input_file, output_file)
    progress.update()


def images_to_ardufiles(
        input_dir: Path,
        output_dir: Path,
) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    files = list(sorted(input_dir.glob("*.bmp")))
    with tqdm(total=len(files), desc="Processing images", unit="frames") as progress, ThreadPoolExecutor() as pool:
        for i, input_image in enumerate(files):
            output_image = output_dir / f"img_{i}"
            pool.submit(convert_single_image, progress, input_image, output_image)


def main():
    logging.basicConfig()
    parser = argparse.ArgumentParser("Convert images to arduino files")
    parser.add_argument("--input-dir", type=str, default=input_images_dir)
    parser.add_argument("--output-dir", type=str, default=output_files_dir)
    parser.add_argument("--debug", action="store_true")

    args = parser.parse_args()

    if args.debug:
        log.setLevel(logging.DEBUG)

    return images_to_ardufiles(
        input_dir=Path(args.input_dir),
        output_dir=Path(args.output_dir),
    )


if __name__ == "__main__":
    main()

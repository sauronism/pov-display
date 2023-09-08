import os
import time
import cv2
from video_utils import VideoReader

output_folder = 'output_images/'

video_reader_settings = {
    "video_path": "sauron.mp4",
    "width": 300,
    "height": 200,
    "duration_seconds": 160,
    "fps": 20,
}


def main():
    os.makedirs(output_folder, exist_ok=True)

    with VideoReader(**video_reader_settings) as video_reader:
        for frame_index, frame in enumerate(video_reader.iterate_with_progress):
            # for now, only save the file.
            file_name = f"frame_{frame_index}.bmp"
            output_path = os.path.join(output_folder, file_name)
            cv2.imwrite(output_path, frame)


if __name__ == "__main__":
    main()

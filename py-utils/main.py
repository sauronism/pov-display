import os
from video_utils import VideoReader

output_folder = 'output_images/'

video_reader_settings = {
    "video_path": "sauron.mp4",
    "width": 300,
    "height": 200,
    "duration_seconds": 120,
    "fps": 20,
}


def main():
    os.makedirs(output_folder, exist_ok=True)
    with VideoReader(**video_reader_settings) as video_reader:
        for frame in video_reader.iterate_with_progress():
            # Save the frame as an image
            output_path = os.path.join(
                output_folder, f"frame_{video_reader.current_frame:04d}.bmp")

            cv2.imwrite(output_path, frame)

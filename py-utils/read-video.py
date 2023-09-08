import os
import cv2


class VideoReader:
    def __init__(self, video_path, width=None, height=None, duration_seconds=None, fps=None):
        self.video_path = video_path
        self.cap = cv2.VideoCapture(video_path)
        self.width = width
        self.height = height
        self.desired_fps = fps
        self.current_frame = 0

        self.frame_count = int(self.cap.get(cv2.CAP_PROP_FRAME_COUNT))
        self.original_fps = int(self.cap.get(cv2.CAP_PROP_FPS))
        self.desired_duration_frames = int(
            duration_seconds * self.original_fps)

        self.frame_skip_factor = self.original_fps // self.desired_fps

        if not self.cap.isOpened():
            raise ValueError("Error: Couldn't open video file.")

    def __iter__(self):
        return self

    def __next__(self):
        while self.current_frame < self.desired_duration_frames:
            ret, frame = self.cap.read()
            if not ret:
                self.cap.release()
                raise StopIteration

            self.current_frame += 1

            if self.current_frame % self.frame_skip_factor == 0:
                break

        else:
            raise StopIteration

        return frame

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.cap.release()


output_folder = 'output_images/'

video_reader_settings = {
    "video_path": "sauron.mp4",
    "width": 300,
    "height": 200,
    "duration_seconds": 120,
    "fps": 20,
}


os.makedirs(output_folder, exist_ok=True)

with VideoReader(**video_reader_settings) as video_reader:
    for frame in video_reader:
        # Save the frame as an image
        output_path = os.path.join(
            output_folder, f"frame_{video_reader.current_frame:04d}.bmp")

        cv2.imwrite(output_path, frame)

import cv2
from tqdm import tqdm


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
            self.cap.release()
            raise StopIteration

        return frame

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.cap.release()

    def iterate_with_progress(self):
        for frame in tqdm(self, total=self.desired_duration_frames, desc="Processing Frames"):
            yield frame

from ctypes import Union
from typing import Tuple
import cv2
from tqdm import tqdm


class VideoReader:
    def __init__(self, video_path: str, resize: Tuple[int], duration_seconds=None, fps=None) -> None:
        self.resize = resize
        self.desired_fps = fps

        self._add_video(video_path)
        self._set_des_fps(fps)
        self._set_output_video_duration(duration_seconds)

        self._current_frame = 0
        self._output_current_frame = 0

    def _set_output_video_duration(self, duration_seconds):
        self.desired_frame_count = int(
            duration_seconds * self.desired_fps)
        self.frame_skip_factor = self.original_video_fps // self.desired_fps

    def _add_video(self, video_path):
        self.video_path = video_path
        self.cap = cv2.VideoCapture(video_path)

        if not self.cap.isOpened():
            raise ValueError("Error: Couldn't open video file.")

        self.original_video_frame_count = int(
            self.cap.get(cv2.CAP_PROP_FRAME_COUNT))
        self.original_video_fps = int(self.cap.get(cv2.CAP_PROP_FPS))

    def _set_des_fps(self, fps):
        if fps > self.original_video_fps:
            raise ValueError(
                f"Original video is {self.original_video_fps}-fps.\n"
                f"In your video, you want {self.desired_fps}-fps. Cannot make this up!!"
            )

        self.desired_fps = fps

    def __iter__(self):
        return self

    def __next__(self) -> cv2.UMat:
        while self._output_current_frame < self.desired_frame_count:
            ret, frame = self.cap.read()
            frame = cv2.resize(
                frame, self.resize)

            if not ret:
                self.cap.release()
                raise StopIteration

            self._current_frame += 1

            if self._current_frame % self.frame_skip_factor == 0:
                self._output_current_frame += 1
                break
        else:
            self.cap.release()
            raise StopIteration

        return frame

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        self.cap.release()

    @property
    def iterate_with_progress(self):
        for frame in tqdm(self, total=self.desired_frame_count, desc="Processing frames", unit="frames"):
            yield frame

import argparse
from itertools import count
from pathlib import Path
from typing import TypedDict

import cv2
import numpy as np
from tqdm import tqdm


class VideoSettings(TypedDict):
    video_path: Path
    width: int
    height: int
    gamma_correction: float
    output_dir: Path


default_settings = VideoSettings(
    video_path=Path("sauron.mp4"),
    width=288,
    height=128,
    gamma_correction=1.5,
    output_dir=Path("vid_center")
)


def make_gamma_lut(gamma: float) -> np.ndarray:
    lut_float = np.linspace(0.0, 1.0, 256, dtype=np.float32) ** gamma
    lut = np.clip(lut_float * 255, 0, 255).astype(np.uint8)
    return lut.reshape(1, 256)


def gamma_correction(frame: np.ndarray, gamma: float):
    lut = make_gamma_lut(gamma)
    return cv2.LUT(frame, lut)


def convert_video(settings: VideoSettings):
    out_dir = Path(settings["output_dir"])
    out_dir.mkdir(exist_ok=True)
    cap = cv2.VideoCapture(str(settings["video_path"]))
    num_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))

    for frame_index in tqdm(count(), total=num_frames):
        output_path = out_dir / f"frame_{frame_index:08d}.bin"
        ok, frame = cap.read()
        if not ok:
            break

        frame = cv2.cvtColor(frame, code=cv2.COLOR_BGR2RGB)
        frame = cv2.resize(frame, (settings["width"], settings["height"]))
        frame = gamma_correction(frame, settings["gamma_correction"])
        output_path.write_bytes(frame)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("video_path", default=default_settings["video_path"], type=Path)
    parser.add_argument("-W", "--width", default=default_settings["width"], type=int)
    parser.add_argument("-H", "--height", default=default_settings["height"], type=int)
    parser.add_argument("-g", "--gamma-correction", default=default_settings["gamma_correction"], type=float)
    parser.add_argument("-o", "--output-dir", default=default_settings["output_dir"], type=Path)

    args = parser.parse_args()
    settings = VideoSettings(
        video_path=args.video_path,
        width=args.width,
        height=args.height,
        gamma_correction=args.gamma_correction,
        output_dir=args.output_dir
    )
    print(settings)
    convert_video(settings)


if __name__ == "__main__":
    main()

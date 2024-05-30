from itertools import count
from pathlib import Path

import cv2
from tqdm import tqdm


output_folder = 'out/'

video_reader_settings = {
    "video_path": "sauron.mp4",
    "resize": (288, 128),
    "duration_seconds": 160,
}


def main():
    out_dir = Path(output_folder)
    out_dir.mkdir(exist_ok=True)
    cap = cv2.VideoCapture(video_reader_settings["video_path"])

    for frame_index in tqdm(count()):
        ok, frame = cap.read()
        if not ok:
            break
        # for now, only save the file.
        rgb_frame = cv2.cvtColor(frame, code=cv2.COLOR_BGR2RGB)
        rgb_frame = cv2.resize(rgb_frame, video_reader_settings["resize"])
        output_path = out_dir / f"frame_{frame_index:08d}.bin"
        output_path.write_bytes(rgb_frame)


if __name__ == "__main__":
    main()

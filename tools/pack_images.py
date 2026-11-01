#!/usr/bin/env python3
"""Builds android/app/src/main/assets/exercises.bin: two photos per exercise, as raw RGB565.

Photos come from https://github.com/yuhonas/free-exercise-db (public domain, Unlicense).
The app has no image decoder. It reads this file directly.

Usage: python3 tools/pack_images.py <cache dir> <output file>
Needs: Pillow, curl.

File format (little endian):
  8 bytes   "GYMIMG01"
  u16 width, u16 height, u32 exercise count
  u32[count] offset of each exercise's photos from the start of the file, 0 = no photos
  per exercise: 2 photos, each width * height * 2 bytes (RGB565)
"""
import json
import re
import struct
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from PIL import Image

RAW = "https://raw.githubusercontent.com/yuhonas/free-exercise-db/main"
W, H = 168, 112
ROOT = Path(__file__).resolve().parent.parent

# Our exercise name -> database id, for names that do not match exactly.
# None of these are needed for exercises whose name is the same in both lists.
MANUAL = {
    "Bench Press": "Barbell_Bench_Press_-_Medium_Grip", "Squat": "Barbell_Full_Squat",
    "Deadlift": "Barbell_Deadlift", "Overhead Press": "Standing_Military_Press",
    "Barbell Row": "Bent_Over_Barbell_Row", "Pull-up": "Pullups", "Dip": "Dips_-_Chest_Version",
    "Lateral Raise": "Side_Lateral_Raise", "Incline Bench Press": "Barbell_Incline_Bench_Press_-_Medium_Grip",
    "Decline Bench Press": "Decline_Barbell_Bench_Press", "Decline Dumbbell Press": "Decline_Dumbbell_Bench_Press",
    "Dumbbell Fly": "Dumbbell_Flyes", "Incline Dumbbell Fly": "Incline_Dumbbell_Flyes",
    "Cable Fly": "Cable_Crossover", "Low Cable Fly": "Low_Cable_Crossover", "High Cable Fly": "Cable_Crossover",
    "Pec Deck": "Butterfly", "Chest Press Machine": "Machine_Bench_Press",
    "Incline Chest Press Machine": "Leverage_Incline_Chest_Press", "Push-up": "Pushups",
    "Band Chest Press": "Bench_Press_-_With_Bands", "Assisted Pull-up": "Band_Assisted_Pull-Up",
    "Lat Pulldown": "Wide-Grip_Lat_Pulldown", "Close-Grip Lat Pulldown": "Close-Grip_Front_Lat_Pulldown",
    "Seated Cable Row": "Seated_Cable_Rows", "Single-Arm Cable Row": "Seated_One-arm_Cable_Pulley_Rows",
    "T-Bar Row": "T-Bar_Row_with_Handle", "Pendlay Row": "Bent_Over_Barbell_Row",
    "Meadows Row": "One-Arm_Long_Bar_Row", "Chest-Supported Row": "Lying_T-Bar_Row",
    "Machine Row": "Leverage_Iso_Row", "Back Extension": "Hyperextensions_Back_Extensions",
    "Rack Pull": "Rack_Pulls", "Dumbbell Pullover": "Bent-Arm_Dumbbell_Pullover",
    "Arnold Press": "Arnold_Dumbbell_Press", "Machine Shoulder Press": "Machine_Shoulder_Military_Press",
    "Cable Lateral Raise": "Cable_Seated_Lateral_Raise", "Front Raise": "Front_Dumbbell_Raise",
    "Plate Front Raise": "Front_Plate_Raise", "Rear Delt Fly": "Reverse_Flyes",
    "Reverse Pec Deck": "Reverse_Machine_Flyes", "Upright Row": "Upright_Barbell_Row",
    "Handstand Push-up": "Handstand_Push-Ups", "Band Lateral Raise": "Lateral_Raise_-_With_Bands",
    "Cable Shrug": "Cable_Shrugs", "Dumbbell Curl": "Dumbbell_Bicep_Curl", "Hammer Curl": "Hammer_Curls",
    "Concentration Curl": "Concentration_Curls", "Machine Preacher Curl": "Machine_Preacher_Curls",
    "Cable Curl": "Standing_Biceps_Cable_Curl", "Rope Hammer Curl": "Cable_Hammer_Curls_-_Rope_Attachment",
    "Rope Pushdown": "Triceps_Pushdown_-_Rope_Attachment", "Skull Crusher": "EZ-Bar_Skullcrusher",
    "Close-Grip Bench Press": "Close-Grip_Barbell_Bench_Press",
    "Overhead Triceps Extension": "Standing_Dumbbell_Triceps_Extension",
    "Cable Overhead Extension": "Cable_Rope_Overhead_Triceps_Extension",
    "Triceps Kickback": "Tricep_Dumbbell_Kickback", "Bench Dip": "Bench_Dips",
    "Diamond Push-up": "Push-Ups_-_Close_Triceps_Position", "Triceps Dip Machine": "Dip_Machine",
    "Wrist Curl": "Palms-Up_Barbell_Wrist_Curl_Over_A_Bench",
    "Reverse Wrist Curl": "Palms-Down_Wrist_Curl_Over_A_Bench", "Reverse Curl": "Reverse_Barbell_Curl",
    "Dead Hang": "One_Handed_Hang", "Crunch": "Crunches", "Bicycle Crunch": "Air_Bike",
    "V-Up": "Jackknife_Sit-Up", "Leg Raise": "Flat_Bench_Lying_Leg_Raise",
    "Hanging Knee Raise": "Knee_Hip_Raise_On_Parallel_Bars", "Side Plank": "Side_Bridge",
    "Mountain Climber": "Mountain_Climbers", "Flutter Kick": "Flutter_Kicks",
    "Cable Woodchopper": "Standing_Cable_Wood_Chop", "Ab Wheel Rollout": "Ab_Roller",
    "Front Squat": "Front_Barbell_Squat", "Zercher Squat": "Zercher_Squats", "Leg Extension": "Leg_Extensions",
    "Bulgarian Split Squat": "Split_Squat_with_Dumbbells", "Lunge": "Dumbbell_Lunges",
    "Walking Lunge": "Bodyweight_Walking_Lunge", "Step-up": "Dumbbell_Step_Ups",
    "Sissy Squat": "Weighted_Sissy_Squat", "Pistol Squat": "Kettlebell_Pistol_Squat",
    "Jump Squat": "Freehand_Jump_Squat", "Stiff-Leg Deadlift": "Stiff-Legged_Barbell_Deadlift",
    "Dumbbell Romanian Deadlift": "Stiff-Legged_Dumbbell_Deadlift", "Lying Leg Curl": "Lying_Leg_Curls",
    "Hip Thrust": "Barbell_Hip_Thrust", "Glute Bridge": "Butt_Lift_Bridge",
    "Cable Kickback": "One-Legged_Cable_Kickback", "Cable Pull-Through": "Pull_Through",
    "Donkey Kick": "Glute_Kickback", "Hip Abduction Machine": "Thigh_Abductor",
    "Hip Adduction Machine": "Thigh_Adductor", "Kettlebell Swing": "One-Arm_Kettlebell_Swings",
    "Reverse Lunge": "Dumbbell_Rear_Lunge", "Sumo Squat": "Plie_Dumbbell_Squat",
    "Banded Lateral Walk": "Monster_Walk", "Standing Calf Raise": "Standing_Calf_Raises",
    "Leg Press Calf Raise": "Calf_Press_On_The_Leg_Press_Machine", "Donkey Calf Raise": "Donkey_Calf_Raises",
    "Dumbbell Calf Raise": "Standing_Dumbbell_Calf_Raise", "Running": "Running_Treadmill",
    "Walking": "Walking_Treadmill", "Treadmill": "Jogging_Treadmill", "Stationary Bike": "Bicycling_Stationary",
    "Rowing Machine": "Rowing_Stationary", "Elliptical": "Elliptical_Trainer", "Stair Climber": "Stairmaster",
    "Jump Rope": "Rope_Jumping", "Box Jump": "Front_Box_Jump", "Battle Ropes": "Battling_Ropes",
    "Kettlebell Clean": "Two-Arm_Kettlebell_Clean", "Kettlebell Snatch": "One-Arm_Kettlebell_Snatch",
    "Turkish Get-up": "Kettlebell_Turkish_Get-Up_Squat_style", "Medicine Ball Slam": "One-Arm_Medicine_Ball_Slam",
}


def norm(s):
    return re.sub(r"[^a-z0-9]+", " ", s.lower().replace("-", " ")).strip()


def our_names():
    text = (ROOT / "src/core/exercises.hpp").read_text()
    return re.findall(r'^\s+\{"([^"]+)", \w+, \w+\},', text, re.M)


def fetch(url, dest):
    if dest.exists() and dest.stat().st_size > 0:
        return
    dest.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(["curl", "-sSf", "--retry", "3", "-m", "60", "-o", str(dest), url], check=True)


def rgb565(img):
    img = img.convert("RGB")
    out = bytearray()
    for r, g, b in img.getdata():
        out += struct.pack("<H", ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
    return bytes(out)


def cover(img):
    """Scale and crop to W x H without stretching."""
    scale = max(W / img.width, H / img.height)
    img = img.resize((max(W, round(img.width * scale)), max(H, round(img.height * scale))), Image.LANCZOS)
    left, top = (img.width - W) // 2, (img.height - H) // 2
    return img.crop((left, top, left + W, top + H))


def main(cache, out_file):
    cache = Path(cache)
    cache.mkdir(parents=True, exist_ok=True)
    fetch(f"{RAW}/dist/exercises.json", cache / "exercises.json")
    db = {e["id"]: e for e in json.loads((cache / "exercises.json").read_text())}
    by_name = {norm(e["name"]): e["id"] for e in db.values()}

    names = our_names()
    ids = {}
    for n in names:
        i = by_name.get(norm(n)) or MANUAL.get(n)
        if i in db and len(db[i]["images"]) >= 2:
            ids[n] = i
    print(f"{len(names)} exercises, {len(ids)} with photos")
    print("no photos:", ", ".join(n for n in names if n not in ids))

    unique = sorted(set(ids.values()))
    jobs = [(f"{RAW}/exercises/{p}", cache / "img" / p) for i in unique for p in db[i]["images"][:2]]
    with ThreadPoolExecutor(8) as pool:
        list(pool.map(lambda j: fetch(*j), jobs))

    data = {}
    for i in unique:
        data[i] = b"".join(rgb565(cover(Image.open(cache / "img" / p))) for p in db[i]["images"][:2])

    frame_bytes = W * H * 2 * 2
    table_end = 16 + 4 * len(names)
    offsets, blob, placed = [], bytearray(), {}
    for n in names:
        i = ids.get(n)
        if i is None:
            offsets.append(0)
            continue
        if i not in placed:  # exercises that share a photo share the data
            placed[i] = table_end + len(blob)
            blob += data[i]
        offsets.append(placed[i])
    assert all(len(data[i]) == frame_bytes for i in unique)

    Path(out_file).parent.mkdir(parents=True, exist_ok=True)
    with open(out_file, "wb") as f:
        f.write(b"GYMIMG01" + struct.pack("<HHI", W, H, len(names)))
        f.write(struct.pack(f"<{len(names)}I", *offsets))
        f.write(blob)
    print(f"wrote {out_file}: {16 + 4 * len(names) + len(blob):,} bytes")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])

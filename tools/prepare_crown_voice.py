#!/usr/bin/env python3
"""Render the bilingual Crown film cast with eSpeak NG and encode the takes."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PAIR = r'\{("(?:[^"\\]|\\.)*"),\s*("(?:[^"\\]|\\.)*")\}'
SPEAKER = r'\{(-?\d+),\s*' + PAIR


def run(*command):
    return subprocess.run(command, check=True, capture_output=True).stdout


film = (ROOT / "iceberg_crown_cutscene.inc").read_text()
opening, witnesses = film.split("if(story&reply_recorded(3))")
lines = [(int(actor), pl, en) for actor, pl, en in re.findall(SPEAKER, opening)]
lines.extend((4, pl, en) for pl, en in re.findall(PAIR, witnesses))
replies = (ROOT / "iceberg_civic_story.inc").read_text().split('{"Dołącz nazwiska świadków"', 1)[1]
replies = '{"Dołącz nazwiska świadków"' + replies.split("return replies[size_t(scene)]", 1)[0]
reply_pairs = re.findall(PAIR, replies)
assert len(lines) == 20 and len(reply_pairs) == 6
lines.extend((actor, *pair) for actor, pair in zip((-2, 4, -2, 4),
                                                  (reply_pairs[i] for i in (1, 2, 4, 5))))

# Actor voices use only eSpeak's bundled variants, with a radio filter at the quay
# and a smaller loudspeaker for the Sentinel.
cast = {
    -2: ("m3", 48, 168),
    0: ("f3", 53, 157),
    1: ("m2", 37, 156),
    3: ("f2", 49, 163),
    4: ("f4", 45, 166),
    5: ("f1", 58, 160),
    6: ("m4", 56, 167),
    7: ("robot", 37, 157),
}
durations = []
destination = ROOT / "assets/audio/crown-film"
destination.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory() as scratch:
    wav = str(Path(scratch) / "line.wav")
    for index, (actor, pl, en) in enumerate(lines):
        if actor == -1:
            durations.append((2.9, 2.9) if index == 0 else (3.4, 3.4))
            continue
        variant, pitch, speed = cast[actor]
        filters = "highpass=f=110,lowpass=f=4250"
        if index == 15 or actor == 7:
            filters = "highpass=f=300,lowpass=f=3100,acompressor=threshold=0.12:ratio=5"
        row = []
        for language, text in (("pl", pl), ("en", en)):
            path = str(destination / f"{language}-{index}.mp3")
            run("espeak-ng", "-v", f"{language}+{variant}", "-p", str(pitch), "-s", str(speed),
                "-w", wav, json.loads(text))
            run("ffmpeg", "-hide_banner", "-loglevel", "error", "-y", "-i", wav, "-af", filters,
                "-ar", "22050", "-ac", "1", "-b:a", "24k", path)
            row.append(float(run("ffprobe", "-v", "error", "-show_entries", "format=duration",
                                 "-of", "default=nw=1:nk=1", path)))
        durations.append(row)

header = ROOT / "iceberg_crown_voice.hpp"
header.write_text("#pragma once\n// Timings from tools/prepare_crown_voice.py, Polish then English.\n"
                  "namespace iceberg {\n"
                  "inline constexpr double CrownVoiceSeconds[][2] = {\n" +
                  "".join(f"  {{{pl:.3f}, {en:.3f}}},\n" for pl, en in durations) +
                  "};\n}\n")


manifest_path = destination.parent / "manifest.json"
manifest = [entry for entry in json.loads(manifest_path.read_text())
            if entry["file"] != "crown-film/"]
manifest.append({
    "file": "crown-film/",
    "source": ["iceberg_crown_cutscene.inc", "iceberg_civic_story.inc"],
    "author": "Terminal Velocity",
    "changes": "Bilingual game dialogue rendered offline with stock eSpeak NG voices, filtered and encoded as 22.05 kHz mono MP3.",
    "sha256": {path.name: hashlib.sha256(path.read_bytes()).hexdigest()
               for path in sorted(destination.glob("*.mp3"))},
})
manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")

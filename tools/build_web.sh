#!/bin/sh
set -eu
cd -- "$(dirname -- "$0")/.."
EMXX=${EMXX:-em++}
mkdir -p web/dist

steam_dir=${TV_STEAM_SOUNDS:-${HOME:-}/.steam/root/steamui/sounds}
steam_cue="$steam_dir/deck_ui_achievement_toast.wav"
set --
if [ -f "$steam_cue" ]; then
  set -- --preload-file "$steam_cue@/steam-sounds/deck_ui_achievement_toast.wav"
elif [ -n "${TV_STEAM_SOUNDS:-}" ]; then
  echo "Steam achievement WAV missing from TV_STEAM_SOUNDS" >&2
  exit 1
fi

"$EMXX" -std=c++20 -O2 web_frontend.cpp audio.cpp -I . \
  -sALLOW_MEMORY_GROWTH=1 -sENVIRONMENT=web -sFORCE_FILESYSTEM=1 \
  -sEXPORTED_RUNTIME_METHODS='["FS","IDBFS","ccall"]' -lidbfs.js \
  "$@" --preload-file assets/audio@/assets/audio \
  -o web/dist/terminal-velocity.js

cp web/index.html web/game.css web/game.js web/accounts.js web/multiplayer.js web/dist/
cp web/server/language.php web/server/pl-address-ranges.bin web/dist/
cp assets/cell-font.ttf web/dist/
cp assets/audio/CREDITS.txt web/dist/AUDIO-CREDITS.txt
if [ "$#" -gt 0 ]; then
  printf '\nBrowser achievement chime: Valve Steam Big Picture WAV, unmodified.\n' >> web/dist/AUDIO-CREDITS.txt
fi
cp assets/audio/manifest.json web/dist/audio-manifest.json
cp assets/FONT-LICENSE.txt web/dist/FONT-LICENSE.txt
cp vendor/MINIAUDIO-LICENSE web/dist/MINIAUDIO-LICENSE.txt

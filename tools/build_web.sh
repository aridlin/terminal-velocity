#!/bin/sh
set -eu
cd -- "$(dirname -- "$0")/.."
EMXX=${EMXX:-em++}
mkdir -p web/dist
"$EMXX" -std=c++20 -O2 web_frontend.cpp audio.cpp -I . \
  -sALLOW_MEMORY_GROWTH=1 -sENVIRONMENT=web -sFORCE_FILESYSTEM=1 \
  -sEXPORTED_RUNTIME_METHODS='["FS","IDBFS","ccall"]' -lidbfs.js \
  --preload-file assets/audio@/assets/audio \
  -o web/dist/terminal-velocity.js

cp web/index.html web/game.css web/game.js web/accounts.js web/dist/
cp assets/cell-font.ttf web/dist/
cp assets/audio/CREDITS.txt web/dist/AUDIO-CREDITS.txt
cp assets/audio/manifest.json web/dist/audio-manifest.json
cp assets/FONT-LICENSE.txt web/dist/FONT-LICENSE.txt
cp vendor/MINIAUDIO-LICENSE web/dist/MINIAUDIO-LICENSE.txt

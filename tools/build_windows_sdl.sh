#!/bin/sh
# Reproducible dependency versions for the Windows cell-rendered fallback.
# Requires Linux, curl, tar, make and the MinGW-w64 x86_64 C/C++ toolchain.
set -eu
repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir=${TV_WINDOWS_BUILD_DIR:-"$repo_dir/.build-windows"}
mkdir -p "$build_dir/sources" "$build_dir/prefix"
build_dir=$(CDPATH= cd -- "$build_dir" && pwd)
fetch() {
    archive=$1 url=$2 digest=$3
    if [ ! -f "$build_dir/sources/$archive" ]; then
        curl --fail --location --retry 3 "$url" -o "$build_dir/sources/$archive.part"
        mv "$build_dir/sources/$archive.part" "$build_dir/sources/$archive"
    fi
    printf '%s  %s\n' "$digest" "$build_dir/sources/$archive" | sha256sum --check --status
}
fetch SDL2-devel-2.32.10-mingw.tar.gz \
    https://www.libsdl.org/release/SDL2-devel-2.32.10-mingw.tar.gz \
    83a5d74012311edc3c0d40ea6faecbe57ad692aa033fa5dc273cc937e3938ff2
fetch freetype-2.14.1.tar.xz \
    https://download.savannah.gnu.org/releases/freetype/freetype-2.14.1.tar.xz \
    32427e8c471ac095853212a37aef816c60b42052d4d9e48230bab3bdf2936ccc
if [ ! -d "$build_dir/SDL2-2.32.10" ]; then
    tar -xf "$build_dir/sources/SDL2-devel-2.32.10-mingw.tar.gz" -C "$build_dir"
fi
if [ ! -f "$build_dir/prefix/lib/libfreetype.a" ]; then
    tar -xf "$build_dir/sources/freetype-2.14.1.tar.xz" -C "$build_dir"
    (
        cd "$build_dir/freetype-2.14.1"
        ./configure --host=x86_64-w64-mingw32 --prefix="$build_dir/prefix" \
            --disable-shared --enable-static --without-zlib --without-bzip2 \
            --without-png --without-harfbuzz --without-brotli
        make -j"${JOBS:-2}"
        make install
    )
fi
sdl_dir="$build_dir/SDL2-2.32.10/x86_64-w64-mingw32"
cd "$repo_dir"
make windows terminal_velocity_sdl.exe \
    SDL_FLAGS="-I\"$sdl_dir/include\" -I\"$build_dir/prefix/include/freetype2\" \"$sdl_dir/lib/libSDL2.dll.a\" \"$build_dir/prefix/lib/libfreetype.a\" -static -lwinpthread"
cp "$sdl_dir/bin/SDL2.dll" "$repo_dir/SDL2.dll"
printf '%s\n' 'Built terminal_velocity.exe, terminal_velocity_sdl.exe and SDL2.dll.' \
    'Distribute them with play.cmd, play-window.cmd, assets/ and THIRD_PARTY.md.'

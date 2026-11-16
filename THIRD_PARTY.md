# Bundled rendering libraries and fonts

The portable Windows windowed build includes SDL 2.32.10 (zlib license) and links FreeType 2.14.1 under the FreeType License.

Portions of this software are copyright © 2025 The FreeType Project (www.freetype.org). All rights reserved.

- SDL source/development archive: https://www.libsdl.org/release/SDL2-devel-2.32.10-mingw.tar.gz
- FreeType source: https://download.savannah.gnu.org/releases/freetype/freetype-2.14.1.tar.xz
- License texts: `assets/licenses/SDL2.txt`, `assets/licenses/FreeType.txt`.
- `assets/cell-font.ttf`: DejaVu Sans Mono; `assets/font.ttf`: DejaVu Sans. Both use `assets/FONT-LICENSE.txt`.

FreeType is built without optional PNG, Brotli, HarfBuzz, bzip2 or zlib dependencies. Sources are unmodified. Linux uses system SDL/FreeType.

Downloaded archive SHA-256:

- `SDL2-devel-2.32.10-mingw.tar.gz`: `83a5d74012311edc3c0d40ea6faecbe57ad692aa033fa5dc273cc937e3938ff2`
- `freetype-2.14.1.tar.xz`: `32427e8c471ac095853212a37aef816c60b42052d4d9e48230bab3bdf2936ccc`

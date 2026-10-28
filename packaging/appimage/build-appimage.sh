#!/bin/sh
# Build the Steam Deck AppImage from an existing Linux build.
#
# Usage: ./build-appimage.sh [BUILD_DIR] [OUTPUT_APPIMAGE]
# Default BUILD_DIR is ../../build-linux and OUTPUT is
# ../../dist/steamdeck/ROTD-SteamDeck-x86_64.AppImage.
#
# The AppImage contains the recompiled player, the ROM preparation tool and
# SDL2.  It deliberately does NOT contain game or system ROMs.
set -eu

HERE="$(cd "$(dirname "$0")" && pwd)"
PROJECT="$(cd "$HERE/../.." && pwd)"
BUILD_DIR="${1:-$PROJECT/build-linux}"
OUT="${2:-$PROJECT/dist/steamdeck/ROTD-SteamDeck-x86_64.AppImage}"

APPIMAGETOOL="${APPIMAGETOOL:-$HOME/appimage-tools/appimagetool-x86_64.AppImage}"
if [ ! -x "$APPIMAGETOOL" ]; then
    APPIMAGETOOL="$(command -v appimagetool || true)"
fi
if [ -z "$APPIMAGETOOL" ] || [ ! -x "$APPIMAGETOOL" ]; then
    echo "build-appimage: appimagetool not found." >&2
    echo "Set APPIMAGETOOL=/path/to/appimagetool-x86_64.AppImage." >&2
    exit 1
fi
if [ ! -x "$BUILD_DIR/rotd" ]; then
    echo "build-appimage: $BUILD_DIR/rotd is missing; build the Linux target first." >&2
    exit 1
fi

WORK="$PROJECT/build-appimage"
APPDIR="$WORK/ROTD.AppDir"
rm -rf "$WORK"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/lib" \
         "$APPDIR/usr/share/applications" \
         "$APPDIR/usr/share/doc/rotd" \
         "$APPDIR/usr/share/icons/hicolor/256x256/apps"

install -m 0755 "$BUILD_DIR/rotd" "$APPDIR/usr/bin/rotd"
install -m 0755 "$BUILD_DIR/romprep" "$APPDIR/usr/bin/romprep" 2>/dev/null || true
install -m 0755 "$HERE/AppRun" "$APPDIR/AppRun"
install -m 0644 "$HERE/rotd.desktop" "$APPDIR/rotd.desktop"
install -m 0644 "$HERE/rotd.desktop" "$APPDIR/usr/share/applications/rotd.desktop"
install -m 0644 "$HERE/rotd.png" "$APPDIR/rotd.png"
install -m 0644 "$HERE/rotd.png" "$APPDIR/usr/share/icons/hicolor/256x256/apps/rotd.png"
ln -sf rotd.png "$APPDIR/.DirIcon"

# Licence texts must travel with the binary.
install -m 0644 "$PROJECT/LICENSE" "$APPDIR/usr/share/doc/rotd/LICENSE"
install -m 0644 "$PROJECT/THIRD_PARTY_NOTICES.md" "$APPDIR/usr/share/doc/rotd/THIRD_PARTY_NOTICES.md"
mkdir -p "$APPDIR/usr/share/doc/rotd/LICENSES"
cp -R "$PROJECT/LICENSES/." "$APPDIR/usr/share/doc/rotd/LICENSES/"

# Bundle the SDL2 the executable was linked against.  Resolve the soname
# through ldd so it works whether the build used a system SDL2 or a local one.
SDL_LIB="$(ldd "$BUILD_DIR/rotd" | awk '/libSDL2-2\.0\.so\.0/ {print $3; exit}')"
if [ -n "$SDL_LIB" ] && [ -f "$SDL_LIB" ]; then
    REAL="$(readlink -f "$SDL_LIB")"
    cp -L "$REAL" "$APPDIR/usr/lib/libSDL2-2.0.so.0"
    echo "bundled SDL2: $REAL"
else
    echo "warning: libSDL2-2.0.so.0 not found by ldd; relying on the host SDL2" >&2
fi

# A short note travels with the binary inside the image.
cat > "$APPDIR/README.txt" <<'EOF'
Rage of the Dragons - native Neo Geo recompilation (Steam Deck x86_64)

This AppImage contains the recompiled player, SDL2 and the ROM preparation
tool.  It contains no game or system ROM data.

Before playing, prepare your own legally obtained MAME sets:

    ./prepare-roms.sh /path/to/rotd.zip /path/to/neogeo.zip

or put the prepared files in ~/.local/share/ROTD/roms.

Controls: arrows/D-pad or left stick, Z/X/C/V = A/B/C/D,
Back = coin, Start = start, Esc = quit.  Steam Input is supported.
EOF

export ARCH=x86_64
export VERSION="0.1.0"
mkdir -p "$(dirname "$OUT")"
if ! "$APPIMAGETOOL" --no-appstream "$APPDIR" "$OUT"; then
    echo "appimagetool failed; retrying with --appimage-extract-and-run" >&2
    "$APPIMAGETOOL" --appimage-extract-and-run --no-appstream "$APPDIR" "$OUT"
fi
chmod +x "$OUT"
echo "built: $OUT"

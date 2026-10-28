#!/bin/sh
# Prepare Rage of the Dragons ROM files for the AppImage.
#
# Usage: ./prepare-roms.sh /path/to/rotd.zip /path/to/neogeo.zip [OUT_DIR]
# Default OUT_DIR: ~/.local/share/ROTD/roms
#
# The zips must be your own legally obtained MAME sets.  The prepared output
# is ROM-derived data; keep it private and do not redistribute it.
set -eu

if [ "$#" -lt 2 ]; then
    echo "usage: $0 rotd.zip neogeo.zip [OUT_DIR]" >&2
    exit 2
fi

HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="${3:-$HOME/.local/share/ROTD/roms}"

ROMPREP=""
for candidate in "$HERE/romprep" "$HERE/../build-linux/romprep" "$HERE/../build-windows/Release/romprep.exe"; do
    if [ -x "$candidate" ]; then ROMPREP="$candidate"; break; fi
done
if [ -z "$ROMPREP" ]; then
    echo "prepare-roms: romprep not found next to this script." >&2
    exit 1
fi

mkdir -p "$OUT"
"$ROMPREP" --game-zip "$1" --bios-zip "$2" --out "$OUT"

cat <<EOF

Prepared ROM files are in:
  $OUT

Run the AppImage with:
  ROTD_ROM_PATH="$OUT" ./ROTD-SteamDeck-x86_64.AppImage

Controls: arrows/D-pad or left stick; Z/X/C/V = A/B/C/D; Back = coin;
Start = start; Esc = quit.  Set NEOGEORECOMP_FULLSCREEN=0 for a window.
EOF

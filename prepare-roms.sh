#!/bin/sh
# Convenience wrapper for the Linux/Steam Deck ROM preparation script.
set -eu
HERE="$(cd "$(dirname "$0")" && pwd)"
exec "$HERE/packaging/steamdeck/prepare-roms.sh" "$@"

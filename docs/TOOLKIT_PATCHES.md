# Changes to the vendored neogeorecomp toolkit

The project vendors neogeorecomp v0.2.0 (commit
`af96b2b4b39b42b392b17bcaa6a74a5f27088202`) so a checkout builds without
submodules.  Upstream is MIT; the MIT notice is kept in
`neogeorecomp/LICENSE` and `LICENSES/neogeorecomp-MIT.txt`.

Only one generic runtime change was made:

## `src/window_sdl.c`: opt-in fullscreen desktop + nearest-neighbour hint

**Why:** Steam Deck Gaming Mode and a 1280x800 display need a fullscreen
window, and the game should not be stretched.  The upstream window is a
resizable 3x window (960x672) with no fullscreen switch.

**Change:**

- Read `NEOGEORECOMP_FULLSCREEN`; when set to a non-zero value, create the
  window with `SDL_WINDOW_FULLSCREEN_DESKTOP`.
- Set `SDL_HINT_RENDER_SCALE_QUALITY` to `"0"` so the 320x224 image uses
  nearest-neighbour scaling.
- `SDL_RenderSetLogicalSize(320, 224)` was already upstream and keeps the
  original aspect ratio, letterboxing on 16:10/16:9 displays.

The change is generic (no game addresses or title checks).  It is opt-in, so
the normal windowed build is unchanged.

No recompiler, CPU, bus, video, audio or input code was modified.

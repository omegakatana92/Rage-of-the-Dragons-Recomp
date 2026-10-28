# Changes to the vendored neogeorecomp toolkit

The project vendors neogeorecomp v0.2.0 (commit
`af96b2b4b39b42b392b17bcaa6a74a5f27088202`) so a checkout builds without
submodules.  Upstream is MIT; the MIT notice is kept in
`neogeorecomp/LICENSE` and `LICENSES/neogeorecomp-MIT.txt`.

The changes below are generic board-model fixes, not title-specific patches.
They were exercised against ROTD's 5,000-frame and 12,000-frame verified runs
(0 mismatches, same frame hashes as before the changes).

## `src/window_sdl.c`: opt-in fullscreen desktop + nearest-neighbour hint

**Why:** Steam Deck Gaming Mode and a 1280x800 display need a fullscreen
window, and the game should not be stretched.

**Change:** `NEOGEORECOMP_FULLSCREEN` selects `SDL_WINDOW_FULLSCREEN_DESKTOP`;
`SDL_HINT_RENDER_SCALE_QUALITY` is `"0"`.  `SDL_RenderSetLogicalSize(320,224)`
was already upstream and preserves the original aspect ratio.

## `src/bus.c`: MAME-compatible P-ROM bank validation

**Why:** MAME rejects a bank whose 1 MB window would start at or beyond the end
of the P ROM and selects bank 0 instead; the toolkit wrapped with a modulo,
which could execute the wrong half of a 3 MB or 5 MB program.

**Change:** `bank_write()` applies MAME's `(bank + 1) * 0x100000 >=
prom_size` check before updating the bank window.

## `src/bus.c` / `src/io.c` / `src/neogeo.c`: watchdog reset

**Why:** Several MVS titles use the watchdog reset during backup-RAM and
protection initialization.  The toolkit only ignored the $300000 kick.

**Change:** writes to `$300000` reload a 1,622,015-cycle countdown (3,244,030
24 MHz clocks, matching MAME).  Expiry performs a board reset without rewinding
frame time or clearing battery-backed RAM.  ROTD continues to pass `--verify`
unchanged.

## `src/video.c`: safe padding for non-power-of-two C-ROM sets

**Why:** Sprite codes are masked to the next power of two because the address
lines above the fitted ROMs are not connected.  The decoded buffer was
allocated for only the fitted tiles, so a non-power-of-two set such as KOF'97's
40 MB C data could read past the end of the heap.

**Change:** allocate the padded `pow2 * 256` byte buffer with `calloc`, leaving
missing tiles zero-filled.  ROTD's 64 MB set is already a power of two, so its
decoded data is unchanged.

No CPU, recompiler or audio synthesis code was modified.

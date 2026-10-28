# Rage of the Dragons: native static recompilation

**Rage of the Dragons (Evoga / Playmore, 2002) running as a native PC program.**
The game's Motorola 68000 program is translated ahead of time into C by
[neogeorecomp](https://github.com/sp00nznet/neogeorecomp) and runs against a
model of the MVS board: LSPC video, palette, I/O, the real MVS BIOS, the Z80
sound CPU and the YM2610.

Two builds are produced from the same source:

- Windows x64 (`rotd.exe`)
- Linux x86_64 AppImage for Steam Deck (`ROTD-SteamDeck-x86_64.AppImage`)

> **Source-only mirror.** This GitHub repository intentionally omits CMake
> files and the vendored neogeorecomp toolkit at the owner's request. It
> contains no ROMs, BIOS files or ROM-derived binaries. See `BUILDING.md` for
> the dependency and build outline.

> **No game data is included.** You need your own legally obtained MAME sets
> `rotd` and `neogeo`. `tools/romprep` verifies and decrypts your dump locally.
> ROM-derived output (prepared ROMs, generated C, built executables) must not
> be committed or redistributed.

## Status

**v0.1.0, alpha.** The game boots through the real MVS BIOS, runs its attract
mode, takes a coin, reaches the title, How to Play, character select, a match
and a rematch on recompiled code.  The measured soak below ran essentially
entirely on native code with no verification mismatches.

| Check | Result |
|---|---|
| ROM set validated | Yes: P1/C1-C8/M1/V1-V2 CRCs match the MAME `rotd` set |
| BIOS | `sp-s2.sp1`, `sfix.sfix`, `sm1.sm1`, `000-lo.lo` |
| Interpreter boot | Yes, with decrypted C/M/V and extracted S data |
| Native recompilation | 182,056 instructions, 6,523 routines, 11,111 dispatch entries |
| `--verify` soak | 13,267,403 blocks checked, **0 mismatches** |
| Native execution in that soak | 13,267,403 native blocks, **190 interpreted instructions**, 0 uncovered entry points |
| Graphics | BIOS, Evoga logo, title, How to Play, select, match, rematch, attract |
| Audio | YM2610/Z80 path produces non-silent stereo WAV; windowed audio device opens |
| Cross-platform | Native Windows and Linux frames are byte-identical (frame 4499 SHA-256 `8209cd53...`) |
| Windows x64 | Built and tested (headless gameplay + windowed launch) |
| Linux x86_64 | Built and tested (headless gameplay + windowed and WSLg fullscreen) |
| Steam Deck AppImage | Built and tested in WSL2 Ubuntu 22.04; **not tested on Deck hardware** |

The remaining 190 interpreted instructions in the soak come from a small
routine the game copies into work RAM at `$10FC00`; the static recompiler
cannot see RAM-generated code, so Musashi handles it.  That is a correctness
fallback, not an emulator wrapper around the game.

## Hardware notes: this is an encrypted NEO-CMC50 set

`rotd` is a **NEO-CMC50 (90G06CF7050) / NEO-PCM2** cartridge:

- The P ROM is plain.  It is a single 8 MB image: 1 MB fixed bank at
  `$000000` plus seven 1 MB banks behind `$200000`.
- The C ROMs are eight 8 MB bitplane files encrypted with the CMC50 graphics
  scheme (key `0x3f`).
- The CHA board has **no S ROM**.  The 128 KB fixed-layer data lives at the
  end of the C data and is extracted after C decryption.
- M1 is encrypted with the CMC50 M1 scheme (key derived from the first 64 KB).
- V1/V2 use the NEO-PCM2 V-ROM address swap (16-byte blocks).

`tools/romprep` implements these decryptions generically and verifies the
result against known CRCs:

| Output | CRC32 | Meaning |
|---|---|---|
| `264-s1.s1` | `c155d4ff` | extracted fixed layer |
| `264-c1.c1` ... `264-c8.c8` | `ec9d18c0` ... `82b1ba22` | decrypted C planes |
| `264-v1.v1` / `264-v2.v2` | `2c49f3fa` / `967279da` | decrypted V banks |
| `264-m1.m1` | `c5d36af9` (128 KB) | decrypted sound driver |

The decryption code and tables are derived from MAME's BSD-3-Clause
`prot_cmc.cpp` and `prot_pcm2.cpp`; see `THIRD_PARTY_NOTICES.md`.

## Quick start

### 1. Prepare your own ROMs

You need `rotd.zip` and `neogeo.zip` from your own MAME set.

```text
# Linux / Steam Deck
./prepare-roms.sh /path/to/rotd.zip /path/to/neogeo.zip
# default output: ~/.local/share/ROTD/roms

# Windows
prepare-roms.bat C:\path\to\rotd.zip C:\path\to\neogeo.zip
# default output: .\roms
```

`romprep` prints every file's CRC and refuses a set that is not the verified
`rotd` dump.  The output directory is self-contained: game files plus BIOS.

### 2. Play

```text
# Windows
rotd.exe --rom-path roms

# Linux (source build)
./build-linux/rotd --rom-path build-linux/roms

# AppImage
ROTD_ROM_PATH="$HOME/.local/share/ROTD/roms" ./ROTD-SteamDeck-x86_64.AppImage
```

The AppImage also looks for `roms` next to the AppImage and in
`~/.local/share/ROTD/roms`, so the environment variable is optional.

### Controls

| Action | Keyboard | Xbox-compatible pad |
|---|---|---|
| Move | Arrow keys | D-pad or left stick |
| A / B / C / D | Z / X / C / V | A / B / X / Y |
| Start | 1 (P1), 2 (P2) | Start |
| Coin | 5 (P1), 6 (P2) | Back / View |
| Select | 3 | Left shoulder |
| Service | 9 | - |
| Test switch | F2 | - |
| Quit | Esc | Steam overlay / close |

Steam Input: use a standard gamepad template.  The toolkit maps SDL's
GameController south/east/west/north to Neo Geo A/B/C/D, so Steam's default
Xbox layout works without a custom template.  Coin is on Back/View.

Fullscreen is enabled by `NEOGEORECOMP_FULLSCREEN=1`; the AppRun sets it by
default.  The renderer uses `SDL_RenderSetLogicalSize(320, 224)`, so 16:10
Steam Deck output is letterboxed, never stretched.

## Build from source

### Prerequisites

- CMake 3.21+
- C and C++ compiler: Visual Studio 2022/2026 with Desktop C++, or GCC/Clang
- SDL2 development files
- Git and a `rotd.zip` + `neogeo.zip`

The toolkit is not vendored in this source-only mirror. Clone
neogeorecomp v0.2.0 next to this tree. The commands below assume the local
CMake build files, which are intentionally omitted from this repository at the
owner's request; see `BUILDING.md`.

### Windows (MSVC, x64)

```powershell
cmake -S . -B build-windows -G "Visual Studio 18 2026" -A x64 `
  -DSDL2_DIR="C:/path/to/SDL2-2.32.6/cmake" `
  -DROTD_GAME_ZIP="C:/path/to/rotd.zip" `
  -DROTD_BIOS_ZIP="C:/path/to/neogeo.zip"
cmake --build build-windows --config Release
build-windows\Release\rotd.exe --rom-path build-windows\roms
```

The build first runs `romprep` into `build-windows/roms`, then regenerates and
compiles the recompiled C into `build-windows/generated`.

### Linux (GCC, x86_64)

```sh
sudo apt install build-essential cmake libsdl2-dev
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release \
  -DROTD_GAME_ZIP="$PWD/rotd.zip" -DROTD_BIOS_ZIP="$PWD/neogeo.zip"
cmake --build build-linux -j"$(nproc)"
./build-linux/rotd --rom-path build-linux/roms
```

For the AppImage I built SDL2 2.32.6 from source and passed
`-DSDL2_DIR=/path/to/sdl2-install/lib/cmake/SDL2`; the packaged AppImage
bundles that SDL2 so controller support does not depend on the host's version.

### AppImage

```sh
sh packaging/appimage/build-appimage.sh build-linux \
  dist/steamdeck/ROTD-SteamDeck-x86_64.AppImage
```

The AppDir contains `usr/bin/rotd`, `usr/bin/romprep`, `usr/lib/libSDL2-2.0.so.0`,
the `.desktop` entry and icon.  ROMs are deliberately outside the image.

### Interpreter-only build

Omit `ROTD_GAME_ZIP` and `ROTD_ROM_DIR`; the player builds without generated
code and runs everything in Musashi.  This is useful for runtime work:

```sh
cmake -S . -B build-interp -DCMAKE_BUILD_TYPE=Release
cmake --build build-interp -j
./build-interp/rotd --headless --interp --rom-path /path/to/prepared-roms --frames 3000
```

## Verification and profiling

```sh
# Scripted coin/start/match with the correctness oracle
./build-linux/rotd --headless --verify --rom-path build-linux/roms \
  --frames 5000 --input tests/coin_start.txt

# Longer soak; writes any runtime-only entry points it finds
./build-linux/rotd --headless --verify --rom-path build-linux/roms \
  --frames 24000 --input tests/play_long.txt --dump-misses build-linux/entries_soak.txt
```

Measured on this machine (Linux GCC build):

```text
[verify] 13267403 blocks checked, 0 mismatches
[exec] native blocks 13267403, interpreted instructions 190, 0 uncovered entry points
```

The Windows Release build produced the same 0-mismatch result for the 5,000
frame script (2,610,360 blocks, 76 interpreted instructions).

`--dump-misses` found no new static entry points in the long soak, so the
profile pass does not change the build for the tested paths.  The 190
interpreted instructions are the work-RAM routine at `$10FC00`.

## Project layout

```text
rotd/
  CMakeLists.txt
  README.md
  LICENSE                       PolyForm Noncommercial 1.0.0
  THIRD_PARTY_NOTICES.md
  LICENSES/                     full third-party licence texts
  neogeorecomp/                 vendored toolkit v0.2.0 (MIT)
  src/
    game.c, game.h              verified ng_game_t ROM layout
    main.c                      player (ng_main)
    recomp_main.c               generator (ng_recomp_main)
  tools/romprep/                CRC check + CMC50/PCM2 decryption (BSD-3)
    miniz/                      zip reader (MIT)
  tests/                        headless input scripts
  packaging/
    appimage/                   AppRun, desktop entry, icon, build script
    steamdeck/                  prepare-roms.sh
    windows/                    prepare-roms.bat
  build-windows/                generated, not for distribution
  build-linux/                  generated, not for distribution
  dist/
    windows/ROTD-Windows-x64/
    steamdeck/
```

## Known limitations

- **Steam Deck hardware not tested.** The AppImage was built and launched in
  WSL2 Ubuntu 22.04 (Xvfb 1280x800 and WSLg 1600x900), including fullscreen
  desktop mode.  A real Deck run in Gaming Mode is still required before
  claiming Deck validation.  The AppImage bundles SDL2 but not glibc or audio
  libraries; SteamOS 3 should provide them.
- **Work-RAM code is interpreted.** The game copies a short routine to
  `$10FC00`; 190 instructions in the long soak ran in Musashi.  This is exact
  but not native.
- **Only the first 2 MB of the 8 MB P ROM is statically analyzed.** The
  recompiler models the fixed 1 MB and the first banked MB.  Later banks are
  available to the runtime but code reached there would fall back to Musashi.
  The tested soak reached no such entry point, but later stages and every
  character pairing are not exhaustively covered.
- **Interpreter/native frame drift.** `--verify` checks CPU state, not event
  scheduling.  Title and character-select frames are byte-identical between
  `--interp` and the recompiled build, but in-match/attract frames can drift
  by a small amount (451 pixels out of 71,680 at frame 4199; 1,671 at frame
  4499) because the event timer can fall on a different block boundary.  The
  native build is deterministic and byte-identical between Windows and Linux.
- **Not a full playthrough.** The scripted tests reach title, How to Play,
  character select, a match, a rematch and attract mode.  Ending, continues,
  two-player play and all stages are untested by script.
- **Audio correctness is not a listening test.** The Z80 driver and YM2610
  produce non-silent stereo output at the board's rate, and the WAV is
  generated through the real audio path; musical/SFX fidelity was not
  auditioned against hardware.
- **No remapping/settings menu.** The toolkit's fixed keyboard and gamepad
  layout is used.
- **MVS BIOS only.** The build targets the MVS `sp-s2.sp1` path, not AES.

## Other Neo Geo sets / sound status

The generic runtime was also used to investigate `garou`, `kof96`, `kof97`,
`kof2000` and `mslug3`.  During that work I added MAME-compatible P-ROM bank
validation, a watchdog reset, and safe padding for non-power-of-two C-ROM
sets.  Those sets still stop at SNK's red "original SNK boards" warning before
sound initialization, so **their sound errors are not confirmed fixed** and
they are not part of this repository's supported builds.  ROTD remains the
verified target.

## Documentation

- `docs/VALIDATION.md`: exact ROM CRCs, commands, measured blocks, frame hashes
- `docs/TOOLKIT_PATCHES.md`: the one generic runtime change and why
- `THIRD_PARTY_NOTICES.md` and `LICENSES/`: upstream and dependency licences

## Licensing

- Original project code (game host, `romprep.c`, packaging, docs) is
  **PolyForm Noncommercial 1.0.0**.  Commercial use requires a separate
  licence.  See `LICENSE`.
- The vendored neogeorecomp toolkit is **MIT** and is not relicensed.
- Musashi, superzazu/z80 and miniz are **MIT**; ymfm is **BSD-3-Clause**;
  SDL2 is **zlib**; the CMC50/PCM2 decryption is **BSD-3-Clause** (MAME).
- Full texts: `LICENSES/` and `THIRD_PARTY_NOTICES.md`.
- **No ROMs or BIOS files are included or downloaded.**  *Rage of the
  Dragons* remains the property of its rights holders; this project is not
  affiliated with SNK, Evoga or Playmore.

## Credits

- [sp00nznet/neogeorecomp](https://github.com/sp00nznet/neogeorecomp) for the
  recompiler and MVS board model.
- MAME's Neo Geo driver and protection devices, used as the hardware and
  decryption reference.
- Musashi, superzazu/z80, ymfm, miniz and SDL2.

# Validation log

Date: 2026-10-27

Toolkit: neogeorecomp v0.2.0, commit
`af96b2b4b39b42b392b17bcaa6a74a5f27088202`.

## Input set

`rotd.zip` and `neogeo.zip` supplied by the user; originals untouched.

| File | Size | CRC32 | Check |
|---|---:|---|---|
| `264-p1.p1` | 8,388,608 | `b8cc969d` | matches MAME `rotd` |
| `264-c1.c1` | 8,388,608 | `4f148fee` | matches |
| `264-c2.c2` | 8,388,608 | `7cf5ff72` | matches |
| `264-c3.c3` | 8,388,608 | `64d84c98` | matches |
| `264-c4.c4` | 8,388,608 | `2f394a95` | matches |
| `264-c5.c5` | 8,388,608 | `6b99b978` | matches |
| `264-c6.c6` | 8,388,608 | `847d5c7d` | matches |
| `264-c7.c7` | 8,388,608 | `231d681e` | matches |
| `264-c8.c8` | 8,388,608 | `c5edb5c4` | matches |
| `264-m1.m1` | 131,072 | `4dbd7b43` | matches |
| `264-v1.v1` | 8,388,608 | `fa005812` | matches |
| `264-v2.v2` | 8,388,608 | `c3dc8bf0` | matches |

The set has no S ROM.  The decrypted C data ends with the fixed-layer image.

## ROM preparation

```text
romprep: Rage of the Dragons (NEO-CMC50 key 0x3f, NEO-PCM2 V block 16)
  264-p1.p1          8388608 bytes  crc32 b8cc969d
  264-s1.s1           131072 bytes  crc32 c155d4ff
  264-m1.m1           131072 bytes  crc32 c5d36af9
  264-v1.v1          8388608 bytes  crc32 2c49f3fa
  264-v2.v2          8388608 bytes  crc32 967279da
  264-c1.c1          8388608 bytes  crc32 ec9d18c0
  264-c2.c2          8388608 bytes  crc32 b1069066
  264-c3.c3          8388608 bytes  crc32 7e636d49
  264-c4.c4          8388608 bytes  crc32 76892fda
  264-c5.c5          8388608 bytes  crc32 469061bc
  264-c6.c6          8388608 bytes  crc32 2200220a
  264-c7.c7          8388608 bytes  crc32 edda4baf
  264-c8.c8          8388608 bytes  crc32 82b1ba22
  sp-s2.sp1           131072 bytes  crc32 9036d879
  sfix.sfix           131072 bytes  crc32 c2ea0cfd
  sm1.sm1             131072 bytes  crc32 94416d67
  000-lo.lo           131072 bytes  crc32 5a86cff2
```

These decrypted CRCs match the known `rotd` decrypted file CRCs
(`c155d4ff` for S1, `ec9d18c0`..`82b1ba22` for C1..C8, `2c49f3fa`/`967279da`
for V1/V2, `c5d36af9` for the 128 KB M1).  This is the ROM-level proof that
the CMC50/PCM2 implementation is right.

## Recompilation

```text
[m68krecomp] pointer scan: 5764 seeds
[m68krecomp] rotd: 182056 instructions emitted across routines
[m68krecomp] 6523 routines, 11111 dispatch entries
```

The static analysis covers the fixed 1 MB and the first 1 MB banked window
(2 MB of the 8 MB P ROM).  The runtime's banked region and Musashi fallback
remain available for later banks.

## Interpreted boot

Frame 600: Evoga logo.  Frame 1800: title / INSERT COIN.  Frame 3000: Rage of
the Dragons logo.  All from `--headless --interp` with the prepared set.

## Verified native run

Linux, 5,000 frames, `tests/coin_start.txt`:

```text
[verify] 2610360 blocks checked, 0 mismatches
[exec] native blocks 2610360, interpreted instructions 76, 0 uncovered entry points
```

Windows Release, same script:

```text
[verify] 2610360 blocks checked, 0 mismatches
[exec] native blocks 2610360, interpreted instructions 76, 0 uncovered entry points
```

Long soak, Linux, 24,000 frames, `tests/play_long.txt`:

```text
[verify] 13267403 blocks checked, 0 mismatches
[exec] native blocks 13267403, interpreted instructions 190, 0 uncovered entry points
[exec] 0 new entry points; entries_soak.txt now lists 0
```

The 190 interpreted instructions come from `$10FC00` in work RAM (a routine
the game copies there).  `NG_PROFILE=1` showed:

```text
[profile] 53 interpreted from $10FC00  move.b  D0, ($3,A1)
[profile] 23 interpreted from $10FC3C  bne     $10fc4c
```

No new statically discoverable entry points were found.  `--verify` compares
every native block against Musashi; the interpreted instructions are the
fallback itself and are not separately re-checked by the oracle.

## Native/platform determinism

At frame 1799 (title), native and `--interp` screenshots are byte-identical:

```text
6b2b908b956ed9c60ccb4383320bafcabf28fa3009cbb5961c120802d73bbad5  interp1799.png
6b2b908b956ed9c60ccb4383320bafcabf28fa3009cbb5961c120802d73bbad5  native1799.png
```

At frame 4499 (in match), native Windows and native Linux are byte-identical:

```text
8209cd53a920631dfb39eeb84a0171abe9bf6152105adcb488a0ead112334c78  win4500.png
8209cd53a920631dfb39eeb84a0171abe9bf6152105adcb488a0ead112334c78  n4500.png
```

Native vs `--interp` drifts in-game: 451 differing pixels at frame 4199,
1,671 at frame 4499, 0 at title/select frames 3000-3900.  Both modes are
individually deterministic across repeated runs.  This is event-scheduling
drift, not a CPU mismatch; `--verify` (which compares registers, flags and
writes per native block) still reports 0 mismatches.  The native build is the
authoritative output.

## Graphics/gameplay checkpoints

All screenshots are real output from the recompiled build:

- Evoga logo (frame 600)
- title / INSERT COIN (frame 1800)
- How to Play (frame 2500)
- character select (frame 3300)
- Billy vs. Cassandra match (frame 4200)
- active match with effects (frame 8000)
- rematch with different characters (frame 16000)
- attract/intro bridge scene (frame 23000)

## Performance

Headless Linux run, 6,000 frames (about 101 seconds of game time), no
verification:

- wall clock 17.61 s
- user 16.20 s, system 0.59 s
- maximum resident set 226,444 KB

The windowed loop paces to 59.185606 Hz with `SDL_Delay`; rendering is a
320x224 software framebuffer uploaded to one SDL texture.

## Audio

4,000-frame run with `--wav`:

- stereo, 55,555 Hz, 3,754,563 frames
- peak 15,892, RMS 2,849 (non-silent)
- windowed Windows run did not report "no sound", so SDL opened an audio device
- `--interp` WAV was the same length within 3 sample frames (rounding)

This verifies the Z80/YM2610 path produces output; it is not a listening test.

## Windows package

`dist/windows/ROTD-Windows-x64/`:

- `rotd.exe` headless 5,000-frame verify: 0 mismatches
- `rotd.exe` windowed 240-frame launch: exit code 0
- dependencies present: SDL2.dll, vcruntime140.dll, vcruntime140_1.dll,
  msvcp140.dll
- `prepare-roms.bat` reproduced all 17 prepared files from the zips

## Linux package and AppImage

`dist/steamdeck/ROTD-SteamDeck-x86_64.AppImage`:

- direct FUSE execution works (`./ROTD-SteamDeck-x86_64.AppImage --headless ...`)
- `--appimage-extract-and-run` works
- 5,000-frame verify: 2,610,360 blocks, 0 mismatches
- AppRun ROM lookup tested with `ROTD_ROM_PATH` and with a `roms` directory
  next to the AppImage
- `prepare-roms.sh` reproduced all 17 prepared files
- windowed SDL window: 960x672
- fullscreen desktop: 1600x900 on WSLg (which provides a window manager)
- Xvfb 1280x800 screen: window opened at 960x672 because Xvfb has no window
  manager; this was a resolution smoke test, not a fullscreen test
- `SDL_RenderSetLogicalSize(320,224)` keeps the game aspect ratio

**Not tested on actual Steam Deck hardware.**  The AppImage was built and
launched in WSL2 Ubuntu 22.04 with SDL2 2.32.6.  SteamOS 3 validation in
Desktop Mode and Gaming Mode remains an explicit open test.

## Machine

- Windows: Visual Studio Community 2026 18.10.1, MSVC 19.51, CMake 4.3.3
- Linux: WSL2 Ubuntu 22.04, GCC 11.4, CMake 4.4.4, SDL2 2.32.6 (built from
  source for the AppImage)
- Display tests: WSLg 1600x900, Xvfb 1280x800x24, SDL software renderer

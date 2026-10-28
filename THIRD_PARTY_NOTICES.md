# Third-party notices

This project is a game host for [sp00nznet/neogeorecomp](https://github.com/sp00nznet/neogeorecomp).
It vendors the v0.2.0 toolkit (commit `af96b2b4b39b42b392b17bcaa6a74a5f27088202`)
under `neogeorecomp/` and keeps that upstream MIT licence.

The project's original code - the game description, player and generator
entry points, `tools/romprep/romprep.c`, packaging scripts and documentation -
is offered under **PolyForm Noncommercial 1.0.0** (see `LICENSE`).  That is
the licence the project owner confirmed for this project.  It does not replace
or relicense any upstream component.

## Components

| Component | Location | Licence | Copyright |
|---|---|---|---|
| neogeorecomp v0.2.0 | `neogeorecomp/` | MIT | Copyright (c) 2026 sp00nznet |
| Musashi 68000 interpreter | `neogeorecomp/ext/musashi` | MIT | Copyright (c) 1998-2001 Karl Stenerud |
| superzazu/z80 | `neogeorecomp/ext/z80` | MIT | Copyright (c) 2019 Nicolas Allemand |
| ymfm (YM2610 synthesis) | `neogeorecomp/ext/ymfm` | BSD-3-Clause | Copyright (c) 2021 Aaron Giles |
| miniz | `tools/romprep/miniz` | MIT | Copyright (c) 2013-2014 RAD Game Tools and Valve Software; Copyright (c) 2010-2014 Rich Geldreich and Tenacious Software LLC |
| SDL2 | runtime dependency (Windows DLL / bundled Linux library) | zlib | Copyright (C) 1997-2025 Sam Lantinga |
| NEO-CMC50 / NEO-PCM2 decryption | `tools/romprep/neocrypt.c`, `neocrypt_tables.h` | BSD-3-Clause | Copyright (c) S. Smith, David Haywood, Fabio Priuli; MAME contributors |

The full texts are in `LICENSES/`:

- `LICENSES/neogeorecomp-MIT.txt`
- `LICENSES/Musashi.txt`
- `LICENSES/z80.txt`
- `LICENSES/ymfm.txt`
- `LICENSES/miniz.txt`
- `LICENSES/SDL2.txt`
- `LICENSES/MAME-BSD-3-Clause.txt`

`tools/romprep/neocrypt.c` and `neocrypt_tables.h` are a standalone
reimplementation of the NEO-CMC50 C/M decryption and NEO-PCM2 V decryption
used by MAME's `src/devices/bus/neogeo/prot_cmc.cpp` and `prot_pcm2.cpp`.
They are build-time tools and are not linked into the game executable.  They
are licensed BSD-3-Clause, not PolyForm.

## Microsoft Visual C++ runtime

The Windows package may include `vcruntime140.dll`, `vcruntime140_1.dll` and
`msvcp140.dll` from the Visual Studio redistributable.  Those files are
Microsoft redistributable components and remain subject to the Microsoft
Visual Studio licence; they are not covered by this project's licence.  They
are included only so the executable runs on a machine without the Visual C++
redistributable installed.

## Game data

No game ROMs, system ROMs, BIOS files, decrypted ROM data or recompiled code
generated from them are part of the source tree.  `tools/romprep` reads the
user's own MAME sets at build/prepare time; the resulting files are
ROM-derived and must not be committed or redistributed.  The same applies to
the generated C in `build-*/generated/` and to executables built with it.

*Rage of the Dragons* is a trademark and copyright of its respective owners.
This project is not affiliated with or endorsed by SNK, Evoga or Playmore.
Users must supply their own legally obtained ROMs.

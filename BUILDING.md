# Building this source-only tree

This GitHub repository is a **source-only mirror**.  At the owner's request it
contains no CMake files, no game ROMs, no BIOS files and no prebuilt game
binaries.

The runtime dependency is **neogeorecomp v0.2.0**, commit
`af96b2b4b39b42b392b17bcaa6a74a5f27088202`:

<https://github.com/sp00nznet/neogeorecomp>

Clone it next to this repository:

```sh
git clone --recurse-submodules https://github.com/sp00nznet/neogeorecomp
```

The local project used the following build shape (the CMake files themselves
are intentionally not in this repository):

1. `add_subdirectory(neogeorecomp)`.
2. Build `tools/romprep/romprep.c`, `neocrypt.c` and the vendored miniz
   sources into a host `romprep` tool.
3. Run `romprep --game-zip rotd.zip --bios-zip neogeo.zip --out build/roms`.
4. Build `src/recomp_main.c` against `m68krecomp_core` and run it with
   `--rom-path build/roms --bios-path build/roms`.
5. Compile `src/main.c`, `src/game.c` and the generated C into the player,
   linking `neogeorecomp`.

`docs/VALIDATION.md` records the exact tested commands and results from the
local build.  You must supply your own legally obtained `rotd.zip` and
`neogeo.zip`; this repository does not download or include them.

## Why no CMake files?

The repository owner asked for a source-only upload without CMake files.
Please keep that constraint when contributing: do not add `CMakeLists.txt`,
`*.cmake`, `build/` directories, ROMs, BIOS files or generated code.

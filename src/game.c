/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
/*
 * game.c - the Rage of the Dragons (NGM-264) ROM set.
 *
 * The set is a NEO-CMC50 (90G06CF7050) / NEO-PCM2 cartridge.  Its P ROM is
 * plain; the C, M and V ROMs are encrypted and have no separate S ROM.  The
 * board-visible files this project loads are prepared from the user's own
 * dump by tools/romprep, which decrypts C/M/V and extracts the 128 KB fixed
 * layer from the end of the C data.  Nothing in this file is derived from
 * the ROM contents; it only describes their layout.
 *
 * MAME set "rotd":
 *   P ROM  one 8 MB image, byte-swapped in the dump
 *   C ROM  eight 8 MB bitplane files (c1/c2, c3/c4, c5/c6, c7/c8)
 *   S ROM  none; romprep writes the extracted 264-s1.s1
 *   M1     one 128 KB encrypted sound driver
 *   V ROM  two 8 MB encrypted sample banks
 */
#include "game.h"

const ng_game_t k_rotd = {
    .name  = "rotd",
    .title = "Rage of the Dragons",

    /* 1 MB fixed bank at $000000 plus seven 1 MB banks behind $200000. */
    .prom_size = 0x800000,
    .prom = {
        {"264-p1.p1", 0x000000, 0x000000, 0x800000, 1},
    },

    /* Four C-ROM pairs; the runtime interleaves each pair into the
     * 64 MB sprite region. */
    .crom_size = 0x4000000,
    .crom_pairs = {
        {"264-c1.c1", "264-c2.c2"},
        {"264-c3.c3", "264-c4.c4"},
        {"264-c5.c5", "264-c6.c6"},
        {"264-c7.c7", "264-c8.c8"},
    },

    /* Extracted from the decrypted C region by tools/romprep. */
    .srom = "264-s1.s1",
    .m1   = "264-m1.m1",
    .vrom = {"264-v1.v1", "264-v2.v2"},
};

/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
/*
 * main.c - the Rage of the Dragons player.
 *
 * Hardware, video, audio and input live in neogeorecomp.  This file only
 * hands the runtime the game layout and, when ROMs were available at build
 * time, the table of recompiled 68000 routines generated from the user's own
 * P ROM.
 */
#include "game.h"

#ifdef ROTD_HAVE_RECOMP
extern const ng_func_entry_t ng_recomp_table[];
extern const size_t ng_recomp_count;
#endif

int main(int argc, char **argv) {
#ifdef ROTD_HAVE_RECOMP
    return ng_main(argc, argv, &k_rotd, ng_recomp_table, ng_recomp_count);
#else
    return ng_main(argc, argv, &k_rotd, NULL, 0);
#endif
}

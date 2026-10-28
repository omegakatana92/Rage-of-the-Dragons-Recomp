/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
/*
 * recomp_main.c - the build-time generator.
 *
 * It loads the prepared P ROM exactly as the player does and writes the
 * recompiled C into the build tree.  ROM-derived output stays in the build
 * directory and is never part of the source tree.
 */
#include <neogeorecomp/recomp.h>
#include "game.h"

int main(int argc, char **argv) { return ng_recomp_main(argc, argv, &k_rotd); }

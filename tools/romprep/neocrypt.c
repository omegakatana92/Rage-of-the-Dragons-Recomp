/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * neocrypt.c - generic NEO-CMC50 / NEO-PCM2 cartridge decryption.
 *
 * The algorithms and the CMC50 tables are derived from MAME's
 * src/devices/bus/neogeo/prot_cmc.cpp and prot_pcm2.cpp, both BSD-3-Clause,
 * Copyright (c) S. Smith, David Haywood, Fabio Priuli.  See
 * THIRD_PARTY_NOTICES.md.  This file is a standalone reimplementation for
 * build-time ROM preparation; it is not linked into the game executable.
 */
#include "neocrypt.h"
#include "neocrypt_tables.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- bit helpers ------------------------------------------------------ */

static uint16_t bitswap16(uint16_t v, const int *p) {
    uint16_t r = 0;
    for (int i = 0; i < 16; i++) {
        r = (uint16_t)(r | (((v >> p[i]) & 1u) << (15 - i)));
    }
    return r;
}

static int m1_address_scramble(int address, uint16_t key) {
    static const int p1[8][16] = {
        {15,14,10,7,1,2,3,8,0,12,11,13,6,9,5,4},
        {7,1,8,11,15,9,2,3,5,13,4,14,10,0,6,12},
        {8,6,14,3,10,7,15,1,4,0,2,5,13,11,12,9},
        {2,8,15,9,3,4,11,7,13,6,0,10,1,12,14,5},
        {1,13,6,15,14,3,8,10,9,4,7,12,5,2,0,11},
        {11,15,3,4,7,0,9,2,6,14,12,1,8,5,10,13},
        {10,5,13,8,6,15,1,14,11,9,3,0,12,7,4,2},
        {9,3,7,0,2,12,4,11,14,10,5,8,15,13,1,6},
    };
    static const int key_order[16] = {12,0,2,4,8,15,7,13,10,1,3,6,11,9,14,5};
    static const int final_order[16] = {7,15,14,6,5,13,12,4,11,3,10,2,9,1,8,0};

    int block = (address >> 16) & 7;
    int aux = address & 0xffff;

    aux ^= bitswap16(key, key_order);
    /* MAME passes p1[block][15] first (output bit 15), so reverse the row
     * for bitswap16(), which takes output bit 15 first as well but is fed
     * the natural-order table here. */
    int prow[16];
    for (int i = 0; i < 16; i++) prow[i] = p1[block][15 - i];
    aux = bitswap16((uint16_t)aux, prow);
    aux ^= m1_address_0_7_xor[(aux >> 8) & 0xff];
    aux ^= m1_address_8_15_xor[aux & 0xff] << 8;
    aux = bitswap16((uint16_t)aux, final_order);
    return (block << 16) | aux;
}

/* ---- CRC-32 ----------------------------------------------------------- */

uint32_t ng_crc32(const void *data, size_t len) {
    static uint32_t table[256];
    static int init;
    if (!init) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        init = 1;
    }
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) crc = table[(crc ^ p[i]) & 0xff] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

/* ---- CMC50 graphics --------------------------------------------------- */

static void cmc_decrypt(uint8_t *r0, uint8_t *r1,
                        uint8_t c0, uint8_t c1,
                        const uint8_t *table0hi,
                        const uint8_t *table0lo,
                        const uint8_t *table1,
                        int base, int invert) {
    uint8_t tmp, xor0, xor1;

    tmp = table1[(base & 0xff) ^ kof2000_address_0_7_xor[(base >> 8) & 0xff]];
    xor0 = (uint8_t)((table0hi[(base >> 8) & 0xff] & 0xfe) | (tmp & 0x01));
    xor1 = (uint8_t)((tmp & 0xfe) | (table0lo[(base >> 8) & 0xff] & 0x01));

    if (invert) {
        *r0 = (uint8_t)(c1 ^ xor0);
        *r1 = (uint8_t)(c0 ^ xor1);
    } else {
        *r0 = (uint8_t)(c0 ^ xor0);
        *r1 = (uint8_t)(c1 ^ xor1);
    }
}

int ng_cmc50_gfx_decrypt(uint8_t *rom, size_t rom_size, int extra_xor) {
    if (!rom || rom_size == 0 || (rom_size & 3) || (rom_size & (rom_size - 1))) {
        fprintf(stderr, "cmc50: unsupported C ROM size %zu\n", rom_size);
        return 1;
    }

    uint8_t *buf = (uint8_t *)malloc(rom_size);
    if (!buf) return 1;

    /* Data xor. */
    for (size_t rpos = 0; rpos < rom_size / 4; rpos++) {
        cmc_decrypt(&buf[4 * rpos + 0], &buf[4 * rpos + 3],
                    rom[4 * rpos + 0], rom[4 * rpos + 3],
                    kof2000_type0_t03, kof2000_type0_t12, kof2000_type1_t03,
                    (int)rpos, (int)((rpos >> 8) & 1));
        cmc_decrypt(&buf[4 * rpos + 1], &buf[4 * rpos + 2],
                    rom[4 * rpos + 1], rom[4 * rpos + 2],
                    kof2000_type0_t12, kof2000_type0_t03, kof2000_type1_t12,
                    (int)rpos,
                    (int)((((rpos >> 16) ^ kof2000_address_16_23_xor2[(rpos >> 8) & 0xff])) & 1));
    }

    /* Address xor. */
    for (size_t rpos = 0; rpos < rom_size / 4; rpos++) {
        uint32_t baser = (uint32_t)rpos;
        baser ^= (uint32_t)extra_xor;
        baser ^= (uint32_t)kof2000_address_8_15_xor1[(baser >> 16) & 0xff] << 8;
        baser ^= (uint32_t)kof2000_address_8_15_xor2[baser & 0xff] << 8;
        baser ^= (uint32_t)kof2000_address_16_23_xor1[baser & 0xff] << 16;
        baser ^= (uint32_t)kof2000_address_16_23_xor2[(baser >> 8) & 0xff] << 16;
        baser ^= (uint32_t)kof2000_address_0_7_xor[(baser >> 8) & 0xff];
        baser &= (uint32_t)(rom_size / 4 - 1);

        memcpy(&rom[4 * rpos], &buf[4 * baser], 4);
    }

    free(buf);
    return 0;
}

int ng_cmc50_sfix_decrypt(const uint8_t *rom, size_t rom_size,
                          uint8_t *fixed, size_t fixed_size) {
    if (!rom || !fixed || fixed_size > rom_size) return 1;
    const uint8_t *src = rom + rom_size - fixed_size;
    for (size_t i = 0; i < fixed_size; i++) {
        fixed[i] = src[(i & ~(size_t)0x1f) + ((i & 7) << 2) +
                       ((~i & 8) >> 2) + ((i & 0x10) >> 4)];
    }
    return 0;
}

/* ---- CMC50 M1 --------------------------------------------------------- */

int ng_cmc50_m1_decrypt(uint8_t *rom, size_t rom_size) {
    if (!rom || rom_size < 0x20000) {
        fprintf(stderr, "cmc50: M1 image too small (%zu bytes)\n", rom_size);
        return 1;
    }

    uint16_t key = 0;
    for (size_t i = 0; i < 0x10000; i++) key = (uint16_t)(key + rom[i]);

    uint8_t *buf = (uint8_t *)malloc(rom_size);
    if (!buf) return 1;

    for (size_t i = 0; i < rom_size; i++) {
        buf[i] = rom[m1_address_scramble((int)i, key)];
    }
    memcpy(rom, buf, rom_size);
    free(buf);
    return 0;
}

/* ---- NEO-PCM2 V ROMs -------------------------------------------------- */

int ng_pcm2_v_decrypt(uint8_t *rom, size_t rom_size, int value) {
    /* MAME's pcm2_prot_device::decrypt() swaps address lines inside blocks
     * of `value` bytes: word j is read from word j ^ (value / 4).  For the
     * ROTD board value == 16, which exchanges the two 8-byte halves of every
     * 16-byte block. */
    if (!rom || value <= 0 || (value & 1) || (size_t)value > rom_size) return 1;
    if (value != 16) {
        /* Keep the general word-swap form for other boards. */
        uint16_t *r = (uint16_t *)rom;
        size_t words = rom_size / 2;
        size_t block = (size_t)value / 2;
        size_t half = (size_t)value / 4;
        uint16_t *tmp = (uint16_t *)malloc((size_t)value);
        if (!tmp) return 1;
        for (size_t i = 0; i + block <= words; i += block) {
            memcpy(tmp, &r[i], (size_t)value);
            for (size_t j = 0; j < block; j++) r[i + j] = tmp[j ^ half];
        }
        free(tmp);
        return 0;
    }

    for (size_t i = 0; i + 16 <= rom_size; i += 16) {
        uint8_t t[8];
        memcpy(t, rom + i, 8);
        memcpy(rom + i, rom + i + 8, 8);
        memcpy(rom + i + 8, t, 8);
    }
    return 0;
}

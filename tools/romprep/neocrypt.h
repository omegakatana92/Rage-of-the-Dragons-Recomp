/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * neocrypt.h - generic NEO-CMC50 / NEO-PCM2 cartridge decryption.
 *
 * The routines here implement the decryption the cartridge hardware performs
 * on the C, M and V ROMs of NEO-CMC50 (90G06CF7050) boards.  The P ROM of the
 * supported set is not encrypted.  They are parameterised by the documented
 * per-board keys, so the same code serves every set using this chip.
 */
#ifndef ROTD_NEOCRYPT_H
#define ROTD_NEOCRYPT_H

#include <stddef.h>
#include <stdint.h>

/* CMC50 graphics decryption.  rom is the interleaved C region
 * (c1,c2,c3,c4... bytes interleaved as the board sees them), rom_size is its
 * size in bytes.  extra_xor is the board's GFX key (ROTD: 0x3f). */
int ng_cmc50_gfx_decrypt(uint8_t *rom, size_t rom_size, int extra_xor);

/* Extract and descramble the 128 KB fixed-layer data stored at the end of the
 * decrypted C region. */
int ng_cmc50_sfix_decrypt(const uint8_t *rom, size_t rom_size,
                          uint8_t *fixed, size_t fixed_size);

/* CMC50 M1 decryption.  rom holds the encrypted M1 image; only the first
 * 64 KB is used to derive the key and only the first 128 KB is decrypted. */
int ng_cmc50_m1_decrypt(uint8_t *rom, size_t rom_size);

/* NEO-PCM2 V ROM address-line swap.  value is the byte block size used by
 * the board (ROTD: 16). */
int ng_pcm2_v_decrypt(uint8_t *rom, size_t rom_size, int value);

/* Helper used by the tools: CRC-32 (IEEE) over a buffer. */
uint32_t ng_crc32(const void *data, size_t len);

#endif

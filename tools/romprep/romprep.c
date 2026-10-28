/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
/*
 * romprep.c - prepare the Rage of the Dragons ROM set for neogeorecomp.
 *
 * The retail set is a NEO-CMC50 / NEO-PCM2 cartridge.  Its P ROM is plain,
 * but the C, M and V ROMs are encrypted and the CHA board has no separate
 * S ROM: the fixed-layer data is stored at the end of the C data.
 *
 * This tool reads the user's own MAME zips (or an already extracted
 * directory), verifies the known CRCs, decrypts C/M/V, extracts the S image,
 * and writes the board-visible files the runtime expects:
 *
 *   264-p1.p1   plain program ROM (byte-swapped dump, runtime swaps it)
 *   264-s1.s1   fixed layer extracted from the decrypted C data
 *   264-m1.m1   decrypted sound driver
 *   264-v1.v1   decrypted sample bank 0
 *   264-v2.v2   decrypted sample bank 1
 *   264-c1..c8  decrypted sprite bitplanes
 *   sp-s2.sp1   MVS Asia/Europe system ROM
 *   sfix.sfix   system fixed layer
 *   sm1.sm1     system sound driver
 *   000-lo.lo   sprite shrink table
 *
 * The output is ROM-derived data.  It is for the user's own use and must not
 * be committed or redistributed.
 *
 * Usage:
 *   romprep --game-zip rotd.zip --bios-zip neogeo.zip --out DIR
 *   romprep --src DIR --out DIR                (already extracted files)
 */
#include "neocrypt.h"
#include "miniz/miniz.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(p) mkdir(p, 0777)
#endif

#define P_SIZE  0x800000u
#define C_FILE  0x800000u
#define C_TOTAL 0x4000000u
#define S_SIZE  0x20000u
#define M_SIZE  0x20000u
#define V_FILE  0x800000u
#define V_TOTAL 0x1000000u

#define CRC_P1    0xb8cc969du
#define CRC_M1    0x4dbd7b43u
#define CRC_V1    0xfa005812u
#define CRC_V2    0xc3dc8bf0u
#define CRC_C1    0x4f148feeu
#define CRC_C2    0x7cf5ff72u
#define CRC_C3    0x64d84c98u
#define CRC_C4    0x2f394a95u
#define CRC_C5    0x6b99b978u
#define CRC_C6    0x847d5c7du
#define CRC_C7    0x231d681eu
#define CRC_C8    0xc5edb5c4u
#define CRC_BIOS  0x9036d879u
#define CRC_SFIX  0xc2ea0cfdu
#define CRC_SM1   0x94416d67u
#define CRC_LO    0x5a86cff2u
#define CRC_S     0xc155d4ffu

static const char *const k_cnames[8] = {
    "264-c1.c1", "264-c2.c2", "264-c3.c3", "264-c4.c4",
    "264-c5.c5", "264-c6.c6", "264-c7.c7", "264-c8.c8",
};
static const uint32_t k_ccrc[8] = {
    CRC_C1, CRC_C2, CRC_C3, CRC_C4, CRC_C5, CRC_C6, CRC_C7, CRC_C8,
};

static int make_dirs(const char *path) {
    char tmp[1024];
    size_t n = strlen(path);
    if (n == 0 || n >= sizeof(tmp)) return 1;
    memcpy(tmp, path, n + 1);
    for (size_t i = 1; i < n; i++) {
        if (tmp[i] == '/' || tmp[i] == '\\') {
            char c = tmp[i];
            tmp[i] = '\0';
            if (MKDIR(tmp) != 0 && errno != EEXIST) { /* parent may be a drive */ }
            tmp[i] = c;
        }
    }
    if (MKDIR(tmp) != 0 && errno != EEXIST) return 1;
    return 0;
}

static uint8_t *read_plain(const char *dir, const char *name, size_t want) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "romprep: cannot open %s: %s\n", path, strerror(errno)); return NULL; }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long n = ftell(f);
    rewind(f);
    if (n < 0 || (size_t)n != want) {
        fprintf(stderr, "romprep: %s is %ld bytes, expected %zu\n", path, n, want);
        fclose(f);
        return NULL;
    }
    uint8_t *buf = (uint8_t *)malloc(want);
    if (!buf || fread(buf, 1, want, f) != want) {
        fprintf(stderr, "romprep: cannot read %s\n", path);
        fclose(f);
        free(buf);
        return NULL;
    }
    fclose(f);
    return buf;
}

typedef struct {
    mz_zip_archive zip;
    int open;
} zip_t;

static int zip_open(zip_t *z, const char *path) {
    memset(z, 0, sizeof(*z));
    if (!mz_zip_reader_init_file(&z->zip, path, 0)) {
        fprintf(stderr, "romprep: cannot open zip %s\n", path);
        return 0;
    }
    z->open = 1;
    return 1;
}

static void zip_close(zip_t *z) {
    if (z->open) mz_zip_reader_end(&z->zip);
    z->open = 0;
}

static uint8_t *read_zip(zip_t *z, const char *name, size_t want) {
    if (!z->open) return NULL;
    int idx = mz_zip_reader_locate_file(&z->zip, name, NULL, 0);
    if (idx < 0) {
        /* Accept archives whose entries live under a folder, e.g.
         * "rotd/264-p1.p1". */
        const size_t nlen = strlen(name);
        mz_uint count = mz_zip_reader_get_num_files(&z->zip);
        for (mz_uint i = 0; i < count; i++) {
            mz_zip_archive_file_stat st;
            if (!mz_zip_reader_file_stat(&z->zip, i, &st)) continue;
            size_t slen = strlen(st.m_filename);
            if (slen >= nlen && !strcmp(st.m_filename + slen - nlen, name) &&
                (slen == nlen || st.m_filename[slen - nlen - 1] == '/' ||
                 st.m_filename[slen - nlen - 1] == '\\')) {
                idx = (int)i;
                break;
            }
        }
    }
    if (idx < 0) {
        fprintf(stderr, "romprep: %s is not in the archive\n", name);
        return NULL;
    }
    size_t n = 0;
    void *p = mz_zip_reader_extract_to_heap(&z->zip, (mz_uint)idx, &n, 0);
    if (!p) {
        fprintf(stderr, "romprep: cannot extract %s\n", name);
        return NULL;
    }
    if (n != want) {
        fprintf(stderr, "romprep: %s is %zu bytes, expected %zu\n", name, n, want);
        mz_free(p);
        return NULL;
    }
    return (uint8_t *)p;
}

typedef struct {
    zip_t *zip;
    const char *dir;
} source_t;

static uint8_t *load_file(source_t *s, const char *name, size_t want) {
    return s->zip ? read_zip(s->zip, name, want) : read_plain(s->dir, name, want);
}

static void release(uint8_t *p, source_t *s) {
    if (!p) return;
    if (s->zip) mz_free(p);
    else free(p);
}

static int check_crc(const char *name, const uint8_t *p, size_t n, uint32_t expect) {
    uint32_t got = ng_crc32(p, n);
    if (got != expect) {
        fprintf(stderr, "romprep: %s has CRC32 %08x, expected %08x (wrong set or damaged file)\n",
                name, got, expect);
        return 0;
    }
    return 1;
}

static int write_out(const char *dir, const char *name, const void *data, size_t n) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "romprep: cannot create %s: %s\n", path, strerror(errno));
        return 0;
    }
    if (fwrite(data, 1, n, f) != n) { fclose(f); return 0; }
    return fclose(f) == 0;
}

static void report(const char *name, const void *data, size_t n) {
    printf("  %-16s %9zu bytes  crc32 %08x\n", name, n, ng_crc32(data, n));
}

int main(int argc, char **argv) {
    const char *game_zip = NULL, *bios_zip = NULL, *src_dir = NULL, *out = "roms";
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--game-zip") && i + 1 < argc) game_zip = argv[++i];
        else if (!strcmp(argv[i], "--bios-zip") && i + 1 < argc) bios_zip = argv[++i];
        else if (!strcmp(argv[i], "--src") && i + 1 < argc) src_dir = argv[++i];
        else if (!strcmp(argv[i], "--out") && i + 1 < argc) out = argv[++i];
        else {
            fprintf(stderr,
                    "usage: %s --game-zip rotd.zip [--bios-zip neogeo.zip] --out DIR\n"
                    "       %s --src DIR --out DIR\n", argv[0], argv[0]);
            return 2;
        }
    }
    if (!game_zip && !src_dir) {
        fprintf(stderr, "romprep: supply --game-zip or --src\n");
        return 2;
    }

    zip_t gz = {0}, bz = {0};
    source_t gs = {0}, bs = {0};
    uint8_t *prom = NULL, *m1 = NULL, *v1 = NULL, *v2 = NULL, *crom = NULL;
    uint8_t *csrc[8] = {0};
    uint8_t *bios = NULL, *sfix = NULL, *sm1 = NULL, *lo = NULL;
    uint8_t srom[S_SIZE];
    int ok = 0;

    if (game_zip) {
        if (!zip_open(&gz, game_zip)) goto cleanup;
        gs.zip = &gz;
    } else {
        gs.dir = src_dir;
    }
    if (bios_zip) {
        if (!zip_open(&bz, bios_zip)) goto cleanup;
        bs.zip = &bz;
    } else if (game_zip) {
        bs.zip = &gz;
    } else {
        bs.dir = src_dir;
    }

    printf("romprep: Rage of the Dragons (NEO-CMC50 key 0x3f, NEO-PCM2 V block 16)\n");

    prom = load_file(&gs, "264-p1.p1", P_SIZE);
    m1 = load_file(&gs, "264-m1.m1", M_SIZE);
    v1 = load_file(&gs, "264-v1.v1", V_FILE);
    v2 = load_file(&gs, "264-v2.v2", V_FILE);
    if (!prom || !m1 || !v1 || !v2) goto cleanup;

    if (!check_crc("264-p1.p1", prom, P_SIZE, CRC_P1) ||
        !check_crc("264-m1.m1", m1, M_SIZE, CRC_M1) ||
        !check_crc("264-v1.v1", v1, V_FILE, CRC_V1) ||
        !check_crc("264-v2.v2", v2, V_FILE, CRC_V2)) goto cleanup;

    crom = (uint8_t *)malloc(C_TOTAL);
    if (!crom) { fprintf(stderr, "romprep: out of memory\n"); goto cleanup; }
    for (int p = 0; p < 4; p++) {
        csrc[2 * p] = load_file(&gs, k_cnames[2 * p], C_FILE);
        csrc[2 * p + 1] = load_file(&gs, k_cnames[2 * p + 1], C_FILE);
        if (!csrc[2 * p] || !csrc[2 * p + 1]) goto cleanup;
        if (!check_crc(k_cnames[2 * p], csrc[2 * p], C_FILE, k_ccrc[2 * p]) ||
            !check_crc(k_cnames[2 * p + 1], csrc[2 * p + 1], C_FILE, k_ccrc[2 * p + 1])) goto cleanup;
        for (size_t i = 0; i < C_FILE; i++) {
            crom[(size_t)p * C_FILE * 2 + 2 * i] = csrc[2 * p][i];
            crom[(size_t)p * C_FILE * 2 + 2 * i + 1] = csrc[2 * p + 1][i];
        }
    }

    if (ng_cmc50_gfx_decrypt(crom, C_TOTAL, 0x3f)) goto cleanup;
    if (ng_cmc50_sfix_decrypt(crom, C_TOTAL, srom, sizeof(srom))) goto cleanup;
    if (!check_crc("decrypted S", srom, sizeof(srom), CRC_S)) goto cleanup;
    if (ng_cmc50_m1_decrypt(m1, M_SIZE)) goto cleanup;
    if (ng_pcm2_v_decrypt(v1, V_FILE, 16) || ng_pcm2_v_decrypt(v2, V_FILE, 16)) goto cleanup;

    for (int p = 0; p < 4; p++) {
        for (size_t i = 0; i < C_FILE; i++) {
            csrc[2 * p][i] = crom[(size_t)p * C_FILE * 2 + 2 * i];
            csrc[2 * p + 1][i] = crom[(size_t)p * C_FILE * 2 + 2 * i + 1];
        }
    }

    bios = load_file(&bs, "sp-s2.sp1", 0x20000);
    sfix = load_file(&bs, "sfix.sfix", 0x20000);
    sm1 = load_file(&bs, "sm1.sm1", 0x20000);
    lo = load_file(&bs, "000-lo.lo", 0x20000);
    if (!bios || !sfix || !sm1 || !lo) goto cleanup;
    if (!check_crc("sp-s2.sp1", bios, 0x20000, CRC_BIOS) ||
        !check_crc("sfix.sfix", sfix, 0x20000, CRC_SFIX) ||
        !check_crc("sm1.sm1", sm1, 0x20000, CRC_SM1) ||
        !check_crc("000-lo.lo", lo, 0x20000, CRC_LO)) goto cleanup;

    if (make_dirs(out) != 0) {
        fprintf(stderr, "romprep: cannot create output directory %s\n", out);
        goto cleanup;
    }

    ok = 1;
    ok &= write_out(out, "264-p1.p1", prom, P_SIZE);
    ok &= write_out(out, "264-s1.s1", srom, S_SIZE);
    ok &= write_out(out, "264-m1.m1", m1, M_SIZE);
    ok &= write_out(out, "264-v1.v1", v1, V_FILE);
    ok &= write_out(out, "264-v2.v2", v2, V_FILE);
    for (int i = 0; i < 8; i++) ok &= write_out(out, k_cnames[i], csrc[i], C_FILE);
    ok &= write_out(out, "sp-s2.sp1", bios, 0x20000);
    ok &= write_out(out, "sfix.sfix", sfix, 0x20000);
    ok &= write_out(out, "sm1.sm1", sm1, 0x20000);
    ok &= write_out(out, "000-lo.lo", lo, 0x20000);
    if (!ok) {
        fprintf(stderr, "romprep: failed writing output to %s\n", out);
        goto cleanup;
    }

    printf("romprep: wrote %s\n", out);
    report("264-p1.p1", prom, P_SIZE);
    report("264-s1.s1", srom, S_SIZE);
    report("264-m1.m1", m1, M_SIZE);
    report("264-v1.v1", v1, V_FILE);
    report("264-v2.v2", v2, V_FILE);
    for (int i = 0; i < 8; i++) report(k_cnames[i], csrc[i], C_FILE);
    report("sp-s2.sp1", bios, 0x20000);
    report("sfix.sfix", sfix, 0x20000);
    report("sm1.sm1", sm1, 0x20000);
    report("000-lo.lo", lo, 0x20000);

cleanup:
    release(prom, &gs); release(m1, &gs); release(v1, &gs); release(v2, &gs);
    for (int i = 0; i < 8; i++) release(csrc[i], &gs);
    release(bios, &bs); release(sfix, &bs); release(sm1, &bs); release(lo, &bs);
    free(crom);
    zip_close(&gz);
    zip_close(&bz);
    return ok ? 0 : 1;
}

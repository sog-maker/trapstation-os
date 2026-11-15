/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

#define OTA_AREA 0x000E0000u
#define OTA_AREA_LEN 0x5000u
#define OTA_RES (OTA_AREA + OTA_AREA_LEN - 0x100u)
#define OTA_FL_LEN 0x93000u

#define OTA_LDR_HCRC 0xEBAAu
#define OTA_LDR_DCRC 0x5881u
#define OTA_LDR_LEN 0x4DE1u
#define OTA_FRAME_MAX 640u

static int ota_wire_send(const uint8_t *p, uint32_t n);
static int ota_frame_get(const uint8_t **p, uint32_t *n);
static void ota_frame_done(void);
static uint32_t ota_now_ms(void);
static void ota_idle(void);
static int ota_erase(uint32_t off);
static int ota_prog(uint32_t off, const void *p, uint32_t n);
static int ota_fread(uint32_t off, void *p, uint32_t n);
static void ota_show(uint32_t step, int32_t code);
static void ota_commit(const uint8_t *parm);

static uint32_t ota_rd16(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8; }
static uint32_t ota_rd24(const uint8_t *p) { return ota_rd16(p) | (uint32_t)p[2] << 16; }
static uint32_t ota_rd32(const uint8_t *p) { return ota_rd24(p) | (uint32_t)p[3] << 24; }
static void ota_wr16(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void ota_wr32(uint8_t *p, uint32_t v) { ota_wr16(p, v); ota_wr16(p + 2, v >> 16); }

static uint32_t ota_crc16(const uint8_t *p, uint32_t n, uint32_t c)
{
    uint32_t i;
    while (n--) {
        c ^= (uint32_t)*p++ << 8;
        for (i = 0; i < 8u; i++)
            c = c & 0x8000u ? (c << 1) ^ 0x1021u : c << 1;
        c &= 0xFFFFu;
    }
    return c;
}

static void ota_jl_enc(uint8_t *p, uint32_t n)
{
    uint32_t k = 0xFFFFu;
    while (n--) {
        *p++ ^= (uint8_t)k;
        k = ((k << 1) ^ (k & 0x8000u ? 0x1021u : 0u)) & 0xFFFFu;
    }
}

static uint32_t ota_pack7(const uint8_t *in, uint32_t n, uint8_t *out)
{
    uint32_t acc = 0, nb = 0, o = 0;
    while (n--) {
        acc |= (uint32_t)*in++ << nb;
        nb += 8u;
        while (nb >= 7u) {
            out[o++] = (uint8_t)(acc & 0x7Fu);
            acc >>= 7;
            nb -= 7u;
        }
    }
    if (nb)
        out[o++] = (uint8_t)(acc & 0x7Fu);
    return o;
}

static uint32_t ota_unpack7(const uint8_t *in, uint32_t n, uint8_t *out, uint32_t max)
{
    uint32_t acc = 0, nb = 0, o = 0;
    while (n--) {
        acc |= (uint32_t)(*in++ & 0x7Fu) << nb;
        nb += 7u;
        if (nb >= 8u) {
            if (o < max)
                out[o] = (uint8_t)acc;
            o++;
            acc >>= 8;
            nb -= 8u;
        }
    }
    return o;
}

static uint8_t ota_msg[8 + 32], ota_wire[2 + 48];
static uint8_t ota_dec[OTA_FRAME_MAX] __attribute__((section(".pool")));

static int ota_send_msg(uint32_t cmd, const uint8_t *body, uint32_t n)
{
    uint32_t i, s = 0, w;
    ota_msg[0] = 0;
    ota_msg[1] = 0x59;
    ota_msg[2] = (uint8_t)cmd;
    ota_msg[3] = (uint8_t)n;
    ota_msg[4] = 0;
    ota_msg[5] = 0;
    for (i = 0; i < n; i++) {
        ota_msg[6 + i] = body[i];
        s += body[i];
    }
    ota_msg[6 + n] = (uint8_t)~s;
    ota_wire[0] = 0xF0;
    w = 1u + ota_pack7(ota_msg, 7u + n, ota_wire + 1);
    ota_wire[w++] = 0xF7;
    return ota_wire_send(ota_wire, w);
}

static void ota_reply_identity(void)
{
    static const char ID[] = FELUCCA_ID;
    uint8_t id[27];
    uint32_t i;
    for (i = 0; i < sizeof id; i++)
        id[i] = i < sizeof ID - 1u ? (uint8_t)ID[i] : 0;
    ota_send_msg(0x11, id, sizeof id);
}

static uint32_t ota_take(void)
{
    const uint8_t *p;
    uint32_t n, d, s = 0, i;
    if (!ota_frame_get(&p, &n))
        return 0;
    d = ota_unpack7(p, n, ota_dec, sizeof ota_dec);
    ota_frame_done();
    if (d < 7u || d > sizeof ota_dec || ota_dec[0] != 0 || ota_dec[1] != 0x59 || ota_rd24(ota_dec + 3) + 7u != d)
        return 0;
    for (i = 6; i + 1u < d; i++)
        s += ota_dec[i];
    if ((uint8_t)~s != ota_dec[d - 1u])
        return 0;
    return d;
}

#if FELUCCA_SMP_USB
static void smp_session(uint32_t d);
#endif
#if FELUCCA_MIRROR
static void mir_cmd(const uint8_t *body, uint32_t n);
#endif
#if FELUCCA_BK_USB
static void bk_session(uint32_t d);
#endif
static void ota_service(void)
{
    uint32_t d = ota_take();
    if (d == 7u && ota_dec[2] == 0x11)
        ota_reply_identity();
#if FELUCCA_MIRROR
    else if (d >= 7u && ota_dec[2] == 0x70)
        mir_cmd(ota_dec + 6, d - 7u);
#endif
#if FELUCCA_SMP_USB
    else if (d == 7u && ota_dec[2] == 0x60)
        smp_session(d);
#endif
#if FELUCCA_BK_USB
    else if (d == 8u && ota_dec[2] == 0x50)
        bk_session(d);
#endif
}

static uint32_t ota_deadline;

static int ota_read(uint32_t addr, uint8_t *dst, uint32_t len)
{
    uint8_t q[8];
    uint32_t tries, t, d, i;
    if (ota_deadline && (int32_t)(ota_now_ms() - ota_deadline) > 0)
        return -1;
    q[0] = 0;
    ota_wr32(q + 1, addr);
    q[5] = (uint8_t)len;
    q[6] = (uint8_t)(len >> 8);
    q[7] = 0;
    for (tries = 0; tries < 3u; tries++) {
        if (ota_send_msg(0x30, q, 8))
            return -1;
        for (t = ota_now_ms(); ota_now_ms() - t < 5000u;) {
            ota_idle();
            d = ota_take();
            if (!d)
                continue;
            if (d == 7u && ota_dec[2] == 0x11) {
                ota_reply_identity();
                continue;
            }
            if (d == len + 15u && ota_dec[2] == 0x30 && ota_rd24(ota_dec + 3) == len + 8u &&
                ota_rd32(ota_dec + 7) == addr && ota_rd24(ota_dec + 11) == len) {
                for (i = 0; i < len; i++)
                    dst[i] = ota_dec[14 + i];
                return 0;
            }
        }
    }
    return -1;
}

static int ota_memeq(const uint8_t *a, const uint8_t *b, uint32_t n)
{
    while (n--)
        if (*a++ != *b++)
            return 0;
    return 1;
}

static int ota_stage_head(uint32_t len)
{
    static const char NAME[16] = "LOADER.BIN";
    uint8_t h[32];
    uint32_t i;
    for (i = 0; i < 32u; i++)
        h[i] = 0;
    ota_wr16(h + 2, 0xFFFFu);
    ota_wr32(h + 4, OTA_AREA);
    ota_wr32(h + 8, len + 32u);
    h[12] = 0x41u;
    h[13] = 0xFFu;
    ota_wr16(h + 14, 0xFFFFu);
    for (i = 0; i < 16u; i++)
        h[16 + i] = (uint8_t)NAME[i];
    ota_wr16(h, ota_crc16(h + 2, 30, 0));
    return ota_prog(OTA_AREA, h, 32);
}

static int ota_ufw(uint8_t *hdr, uint32_t *fl_off, uint32_t *fl_len, uint32_t *ota_off, uint32_t *ota_len)
{
    uint32_t i, nent;
    *fl_off = *fl_len = *ota_off = *ota_len = 0;
    if (ota_read(0, hdr, 512) || ota_read(512, hdr + 512, 512))
        return -1;
    ota_jl_enc(hdr, 0x40);
    nent = ota_rd16(hdr + 8);
    if (ota_crc16(hdr + 2, 0x3E, 0) != ota_rd16(hdr) || nent == 0 || nent > 11u ||
        ota_crc16(hdr + 0x40, nent * 0x50u, 0) != ota_rd16(hdr + 2))
        return -2;
    for (i = 0; i < nent; i++) {
        uint8_t *e = hdr + 0x40 + i * 0x50u;
        ota_jl_enc(e, 0x50);
        if (ota_rd16(e) == 0) {
            *fl_off = ota_rd32(e + 8);
            *fl_len = ota_rd32(e + 12);
        }
        if (ota_rd16(e) == 100) {
            *ota_off = ota_rd32(e + 8);
            *ota_len = ota_rd32(e + 12);
        }
    }
    return 0;
}

#define OTA_STAGED_RC(rc) ((rc) <= -8 && (rc) >= -13)
static int ota_stage(void)
{
    static uint8_t hdr[0x400] __attribute__((section(".pool"))), b[512] __attribute__((section(".pool"))), mine[512] __attribute__((section(".pool")));
    uint32_t i, n, ota_off, ota_len, fl_off, fl_len, c, len, own, official;
    int rc;

    ota_show(1, 0);
    if ((rc = ota_ufw(hdr, &fl_off, &fl_len, &ota_off, &ota_len)) != 0)
        return rc;
    if (!fl_off || !ota_off)
        return -3;
    if (fl_len != OTA_FL_LEN)
        return -15;

    if (ota_read(ota_off, b, 512))
        return -6;
    len = ota_rd32(b + 8);
    if (ota_crc16(b + 2, 30, 0) != ota_rd16(b) || ota_rd32(b + 4) != 0x20u || b[12] != 0x41u ||
        ota_len != len + 0x20u || len > OTA_AREA_LEN - 0x200u || ota_crc16(b + 0x22, 30, 0) != ota_rd16(b + 0x20) ||
        ota_rd32(b + 0x20 + 8) != 0x01C0A800u)
        return -7;
    official = ota_rd16(b) == OTA_LDR_HCRC && ota_rd16(b + 2) == OTA_LDR_DCRC && len == OTA_LDR_LEN;
    for (i = 0x48, own = 0; i + 16u <= 0x48u + 64u && !own; i++)
        own = ota_memeq(b + i, (const uint8_t *)"FELUCCA-LOADER-1", 16) && ota_rd32(b + 0x20 + 4) <= 0x14000u;
    if (!official && !own)
        return -7;
    if (official) {
        ota_show(2, 0);
        for (i = 0; i < 0x4000u; i += 512u) {
            if (ota_read(fl_off + i, b, 512) || ota_fread(i, mine, 512))
                return -4;
            if (!ota_memeq(b, mine, 512))
                return -5;
        }
    }

    ota_show(3, 0);
    if (ota_read(ota_off, b, 512))
        return -6;
    c = ota_rd16(b + 2);
    for (i = 0; i < OTA_AREA_LEN; i += 0x1000u)
        if (ota_erase(OTA_AREA + i))
            return -8;
    {
        uint32_t crc = 0;
        for (i = 0; i < len; i += n) {
            n = len - i > 512u ? 512u : len - i;
            if (ota_read(ota_off + 0x20u + i, b, n))
                return -9;
            crc = ota_crc16(b, n, crc);
            if (ota_prog(OTA_AREA + 32u + i, b, n))
                return -10;
        }
        if (crc != c)
            return -11;
        for (i = 0, crc = 0; i < len; i += n) {
            n = len - i > 512u ? 512u : len - i;
            if (ota_fread(OTA_AREA + 32u + i, b, n))
                return -11;
            crc = ota_crc16(b, n, crc);
        }
        if (crc != c)
            return -11;
    }
    if (ota_stage_head(len))
        return -12;

    ota_show(4, 0);
    for (i = 0; i < 4u; i++)
        if (!ota_read(0xE0000000u, b, 8) && ota_memeq(b, (const uint8_t *)"success", 8))
            return 0;
    return -13;
}

static void ota_unstage(void)
{
    uint32_t i;
    for (i = 0; i < OTA_AREA_LEN; i += 0x1000u)
        ota_erase(OTA_AREA + i);
}

static int ota_session(void)
{
    static uint8_t parm[112];
    uint8_t back[112];
    uint32_t i;
    int rc;
    ota_deadline = ota_now_ms() + 120000u;
    rc = ota_stage();
    if (rc) {
        if (OTA_STAGED_RC(rc))
            ota_unstage();
        ota_show(9, rc);
        return rc;
    }

    for (i = 0; i < sizeof parm; i++)
        parm[i] = 0;
    ota_wr16(parm + 2, 0x5A0Du);
    ota_wr16(parm + 4, 0x5A01u);
    ota_wr16(parm + 6, 0x5441u);
    for (i = 0; i < 12u; i++)
        parm[8 + i] = (uint8_t)"ota-FM-1_015"[i];
    ota_wr32(parm + 72, OTA_AREA);
    ota_wr16(parm, ota_crc16(parm + 2, 78, 0));
#if FELUCCA_OTA_DRYRUN
    ota_unstage();
    ota_show(9, 1);
    return 1;
#endif
#ifndef FELUCCA_OTA_RAMONLY
#define FELUCCA_OTA_RAMONLY 0

#endif
    if (!FELUCCA_OTA_RAMONLY && (ota_prog(OTA_RES, parm, sizeof parm) || ota_fread(OTA_RES, back, sizeof back) ||
        !ota_memeq(back, parm, sizeof parm))) {
        ota_unstage();
        ota_show(9, -14);
        return -14;
    }
    ota_show(5, 0);
    ota_commit(parm);
    return 0;
}

static int ota_boot_cleanup(void)
{
    uint8_t a[4], r[8];
    uint32_t i, used = 0;
    if (ota_fread(OTA_AREA, a, 4) || ota_fread(OTA_RES, r, 8))
        return -1;
    for (i = 0; i < 4u; i++)
        used |= a[i] != 0xFFu;
    for (i = 0; i < 8u; i++)
        used |= r[i] != 0xFFu;
    if (used)
        ota_unstage();
    return (int)used;
}

/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Blair (blairysbeatlab), BBL JAM BOX */

#define MFX_CSLOPE 32
#define MFX_CB 1400u
static int16_t mfx_cb[2][MFX_CB] __attribute__((section(".pool")));
static struct {
    int32_t dj_a, idx;
    uint8_t hp;
    int32_t z[4];
    int32_t cm, ci, cd, lp[2];

    uint32_t w;
    uint8_t dirty;
} mfx;
static uint16_t mfx_g[64];
static const uint16_t MFX_T2[13] = {32768, 30929, 29193, 27554, 26008, 24548, 23170, 21870, 20643, 19484, 18390, 17358, 16384};

static int32_t mfx_delay(int32_t t)
{
    int32_t s = t * 12, semi = s >> 4, fr = s & 15, o = semi / 12, i = semi - o * 12;
    int32_t a = MFX_T2[i], b = MFX_T2[i + 1];
    int32_t d = (int32_t)(((int64_t)(1349 * 256) * (a + (((b - a) * fr) >> 4))) >> 15) >> o;
    return d < 4 * 256 ? 4 * 256 : d;
}

static int32_t mfx_soft(int32_t y)
{
    int32_t a = y < 0 ? -y : y, e;
    if (a <= 16384)
        return y;
    e = a - 16384;
    a = 16384 + (int32_t)(((int64_t)e * 16383) / (e + 16384));
    return y < 0 ? -a : a;
}

static uint8_t cb_sel = NTRK, cb_at = NTRK, cb_fade;
static uint8_t cb_keys;
static void cb_latch(void)
{
    if (cb_sel == cb_at)
        cb_fade = 0;
    else if (mfx.cm || mfx.dirty)
        cb_fade = 1;
    else {
        cb_at = cb_sel < NTRK ? cb_sel : NTRK;
        cb_fade = 0;
    }
}
static int cb_live(void) { return mfx.cm || mfx.dirty || (song.g[G_SYNC] && !cb_fade); }

static void cb_run(int32_t *bl, int32_t *br, uint32_t n)
{
    int32_t ct = cb_fade ? 0 : song.g[G_SYNC] * 258, td, gain, fbk, damp;
    uint32_t i, nc = br ? 2u : 1u, ch;
    if (!ct && !mfx.cm && !mfx.dirty)
        return;
    td = mfx_delay(song.g[G_CRATE]);
    fbk = song.g[G_CDEPTH] * 251;
    gain = 32768 - ((fbk * 22938) >> 15);
    damp = 32768 - song.g[G_INFO] * 248;
    for (i = 0; i < n; i++) {
        mfx.cm += clamp(ct - mfx.cm, -MFX_CSLOPE, MFX_CSLOPE);
        mfx.ci += clamp((ct ? 32768 : 0) - mfx.ci, -MFX_CSLOPE, MFX_CSLOPE);
        mfx.cd += clamp(td - mfx.cd, -48, 48);
        if (mfx.cm) {
            int32_t rd = (int32_t)(mfx.w << 8) - mfx.cd, m = mfx.cm, *io[2];
            uint32_t p0, p1, fq;
            io[0] = &bl[i];
            io[1] = br ? &br[i] : 0;
            if (rd < 0)
                rd += (int32_t)(MFX_CB << 8);
            p0 = (uint32_t)(rd >> 8) % MFX_CB;
            fq = (uint32_t)rd & 255u;
            p1 = (p0 + 1u) % MFX_CB;
            for (ch = 0; ch < nc; ch++) {
                int32_t x = clamp(*io[ch] >> 2, -32767, 32767), y, wet;
                int32_t d = mfx_cb[ch][p0] + (((int32_t)mfx_cb[ch][p1] - mfx_cb[ch][p0]) * (int32_t)fq >> 8);
                mfx.lp[ch] += (int32_t)(((int64_t)(d - mfx.lp[ch]) * damp) >> 15);
                int32_t fb = (int32_t)(((int64_t)mfx.lp[ch] * fbk) >> 15);
                y = mfx_soft(((x * mfx.ci) >> 15) + fb);
                mfx_cb[ch][mfx.w] = (int16_t)y;
                y = mfx.ci == 32768 ? y : mfx_soft(x + fb);
                wet = (int32_t)(((int64_t)y * gain) >> 15);
                *io[ch] = pf_mix(*io[ch], wet << 2, m);
            }
            mfx.w = (mfx.w + 1u) % MFX_CB;
            mfx.dirty = 1;
        }
    }
    if (!mfx.cm && !ct && mfx.dirty) {
        for (i = 0; i < MFX_CB; i++)
            mfx_cb[0][i] = mfx_cb[1][i] = 0;
        mfx.lp[0] = mfx.lp[1] = 0;
        mfx.dirty = 0;
    }
}

static void mfx_chunk(int32_t *bl, int32_t *br, uint32_t n)
{
    int32_t dj = song.g[G_MIDI];
    int32_t djt = (dj > 1 || dj < -1) ? 32768 : 0;
    int16_t c[3];
    int32_t k = 16384, ds = 0;
    uint32_t i;
    if (!djt && !mfx.dj_a) {
        if (cb_at == NTRK)
            cb_run(bl, br, n);
        return;
    }
    if (!mfx_g[0])
        for (i = 0; i < 64u; i++)
            mfx_g[i] = (uint16_t)(((uint32_t)PF_SVF[i][1] << 14) / (uint32_t)PF_SVF[i][0]);

    if (!mfx.dj_a && djt)
        mfx.hp = dj > 0;
    if (mfx.hp != (dj > 0) && dj != 0 && mfx.dj_a)
        djt = 0;
    {
        int32_t a = dj < 0 ? -dj : dj, tgt, fr, g, den, a1, a2, a3;
        a = a > 64 ? 64 : a;
        tgt = mfx.hp ? (4 << 8) + ((a * (50 - 4) * 256) >> 6) : (63 << 8) - ((a * (63 - 12) * 256) >> 6);
        if (!mfx.dj_a)
            mfx.idx = mfx.hp ? 4 << 8 : 63 << 8;
        if (!djt)
            tgt = mfx.idx;
        mfx.idx += clamp(tgt - mfx.idx, -48, 48);
        {
            int32_t num = mfx.hp ? mfx.idx - (4 << 8) : (63 << 8) - mfx.idx, den = mfx.hp ? (50 - 4) * 256 : (63 - 12) * 256;
            a = (clamp(num, 0, den) * 64 * 256) / den;
        }
        i = (uint32_t)(mfx.idx >> 8);
        fr = mfx.idx & 255;
        g = mfx_g[i] + (((int32_t)mfx_g[i < 63u ? i + 1u : 63u] - mfx_g[i]) * fr >> 8);
        k = 16384 - ((a * (16384 - 5200)) >> 14);
        den = 16384 + (int32_t)(((int64_t)g * (g + k)) >> 14);
        a1 = (int32_t)((1u << 28) / (uint32_t)den);
        a2 = (g * a1) >> 14;
        a3 = (g * a2) >> 14;
        c[0] = (int16_t)a1; c[1] = (int16_t)a2; c[2] = (int16_t)a3;
    }
    (void)ds;
    for (i = 0; i < n; i++) {
        int32_t l = bl[i], r = br[i];
        mfx.dj_a += clamp(djt - mfx.dj_a, -MFX_CSLOPE, MFX_CSLOPE);
        if (mfx.dj_a) {
            int32_t xl = clamp(l >> 3, -16383, 16383), xr = clamp(r >> 3, -16383, 16383), fl, fr2;
            int32_t *z = mfx.z, v3, v1, v2;
            v3 = xl - z[1]; v1 = (c[0] * z[0] + c[1] * v3) >> 14; v2 = z[1] + ((c[1] * z[0] + c[2] * v3) >> 14);
            z[0] = clamp(2 * v1 - z[0], -65535, 65535); z[1] = clamp(2 * v2 - z[1], -65535, 65535);
            fl = mfx.hp ? xl - ((k * v1) >> 14) - v2 : v2;
            z += 2;
            v3 = xr - z[1]; v1 = (c[0] * z[0] + c[1] * v3) >> 14; v2 = z[1] + ((c[1] * z[0] + c[2] * v3) >> 14);
            z[0] = clamp(2 * v1 - z[0], -65535, 65535); z[1] = clamp(2 * v2 - z[1], -65535, 65535);
            fr2 = mfx.hp ? xr - ((k * v1) >> 14) - v2 : v2;
            l = pf_mix(l, fl << 3, mfx.dj_a);
            r = pf_mix(r, fr2 << 3, mfx.dj_a);
        } else {
            mfx.z[0] = mfx.z[1] = mfx.z[2] = mfx.z[3] = 0;
        }
        bl[i] = l;
        br[i] = r;
    }
    if (cb_at == NTRK)
        cb_run(bl, br, n);
}
#define MFX_CHUNK 32u

static __attribute__((noinline)) void mfx_block(int32_t *bl, int32_t *br, uint32_t n)
{
    uint32_t o;
    for (o = 0; o < n; o += MFX_CHUNK)
        mfx_chunk(bl + o, br + o, n - o < MFX_CHUNK ? n - o : MFX_CHUNK);
}

static int8_t dj_trk[NTRK];
static uint8_t dj_sel = NTRK;
static uint8_t sc_keys;
static uint8_t fxl_mute;
static volatile int16_t sc_pend = -1;

static uint8_t dj_keys;
struct dj_part_s { int32_t a, idx, z[2]; uint8_t hp; };
static struct dj_part_s djp[NTRK];
static void mfx_g_init(void)
{
    uint32_t i;
    if (!mfx_g[0])
        for (i = 0; i < 64u; i++)
            mfx_g[i] = (uint16_t)(((uint32_t)PF_SVF[i][1] << 14) / (uint32_t)PF_SVF[i][0]);
}
static int dj_on(uint32_t k) { return dj_trk[k % NTRK] != 0 || djp[k % NTRK].a != 0; }
static void dj_part(uint32_t k, int32_t *b, uint32_t n)
{
    int32_t dj = dj_trk[k % NTRK], djt = (dj > 1 || dj < -1) ? 32768 : 0, kq, c0, c1, c2;
    uint32_t i;
    struct dj_part_s *f = &djp[k % NTRK];
    if (!djt && !f->a)
        return;
    if (!f->a && djt)
        f->hp = dj > 0;
    if (f->hp != (dj > 0) && dj != 0 && f->a)
        djt = 0;
    {
        int32_t a = dj < 0 ? -dj : dj, tgt, fr, g, den, a1;
        a = a > 64 ? 64 : a;
        tgt = f->hp ? (4 << 8) + ((a * (50 - 4) * 256) >> 6) : (63 << 8) - ((a * (63 - 12) * 256) >> 6);
        if (!f->a)
            f->idx = f->hp ? 4 << 8 : 63 << 8;
        if (!djt)
            tgt = f->idx;
        f->idx += clamp(tgt - f->idx, -48, 48);
        {
            int32_t num = f->hp ? f->idx - (4 << 8) : (63 << 8) - f->idx, dn = f->hp ? (50 - 4) * 256 : (63 - 12) * 256;
            a = (clamp(num, 0, dn) * 64 * 256) / dn;
        }
        i = (uint32_t)(f->idx >> 8);
        fr = f->idx & 255;
        g = mfx_g[i] + (((int32_t)mfx_g[i < 63u ? i + 1u : 63u] - mfx_g[i]) * fr >> 8);
        kq = 16384 - ((a * (16384 - 5200)) >> 14);
        den = 16384 + (int32_t)(((int64_t)g * (g + kq)) >> 14);
        a1 = (int32_t)((1u << 28) / (uint32_t)den);
        c0 = a1;
        c1 = (g * a1) >> 14;
        c2 = (g * c1) >> 14;
    }
    for (i = 0; i < n; i++) {
        f->a += clamp(djt - f->a, -MFX_CSLOPE, MFX_CSLOPE);
        if (f->a) {
            int32_t x = clamp(b[i] >> 3, -16383, 16383), v3, v1, v2, y;
            v3 = x - f->z[1];
            v1 = (c0 * f->z[0] + c1 * v3) >> 14;
            v2 = f->z[1] + ((c1 * f->z[0] + c2 * v3) >> 14);
            f->z[0] = clamp(2 * v1 - f->z[0], -65535, 65535);
            f->z[1] = clamp(2 * v2 - f->z[1], -65535, 65535);
            y = f->hp ? x - ((kq * v1) >> 14) - v2 : v2;
            b[i] = pf_mix(b[i], y << 3, f->a);
        } else
            f->z[0] = f->z[1] = 0;
    }
}

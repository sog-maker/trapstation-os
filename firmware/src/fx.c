/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

#define DLY_LEN 65536u
static int16_t dly_buf[DLY_LEN] __attribute__((section(".pool")));
static const uint16_t REV_COMB[4] = {1116, 1188, 1277, 1356};
static const uint16_t REV_AP[2] = {556, 441};
static int16_t rev_comb[1116 + 1188 + 1277 + 1356] __attribute__((section(".pool")));
static union {
    int16_t ap[556 + 441];
    int32_t sp[(556 + 441) / 2];
} rev_u __attribute__((section(".pool")));
#define rev_ap (rev_u.ap)
static struct {
    uint32_t dly_w;
    int32_t dly_cur;
    int32_t dly_lp, dly_lp2, dly_hp;
    uint32_t dly_ph;
    uint16_t comb_i[4], ap_i[2];
    int32_t comb_lp[4];
    uint8_t rtype;
    uint16_t sp_w;
    int32_t sp_lp, sp_hp, sp_he, sp_size;

    uint32_t sp_ph;
} fx;

#define SP_LEN 4096u
#define SP_MASK (SP_LEN - 1u)
#define SP_N 10u
#define SP_A 2867

_Static_assert(sizeof rev_comb / 2u >= SP_LEN && sizeof rev_u.sp / 4u >= 4u * (SP_N + 1u), "SPRING in ROOM's buffers");

#define HL_NL 8u
#define HL_NAP 2u
#define HL_NC (CTL / 2u)
#define HL_MOD 8
#define HL_LPM 0x69u
#define HL_KD(d) (32767 - (d) * 180)
#define HL_LOUD 32
#define HL_OUT 4
static const uint16_t HL_LEN[HL_NL] = {467, 509, 547, 593, 631, 673, 709, 743};
static const uint16_t HL_AP[HL_NAP] = {131, 307};
static const int16_t HL_APG[HL_NAP] = {11469, 10240};
_Static_assert(467 + 509 + 547 + 593 + 631 + 673 + 709 + 743 + HL_NL <= sizeof rev_comb / 2u &&
               131 + 307 <= sizeof rev_u.ap / 2u, "HALL in ROOM's buffers");
_Static_assert(CTL % 2u == 0u && HL_NC + 2u * HL_MOD + 2u < 467u && HL_NC <= 131u, "HALL: one wrap a block at most");
static struct {
    uint16_t i[HL_NL], d[HL_NAP];
    int32_t lp[HL_NL], le[HL_NL];
    int32_t x, m, s, hp, he;

    int16_t g[HL_NL], sz;
    uint32_t ph;
    uint8_t side;
    int32_t sd[CTL];
    int32_t bx[HL_NC], l[HL_NC], r[HL_NC];
    int16_t y[HL_NL][HL_NC];
} hl __attribute__((section(".pool")));
#define rev_side (hl.sd)

#define LIM_T 18000
static int32_t lim_env = LIM_T;

static volatile uint8_t fx_usb_fixed;
#define MASTER_FULL 4096

static inline uint32_t master_of_pot(uint32_t k10) { return (k10 * k10) >> 8; }
static __attribute__((noinline)) void usb_fixed_dac(int32_t *out, uint32_t n)
{
    uint32_t i;
    int32_t m = (int32_t)song.master_q12;
    for (i = 0; i < 2u * n; i++)
        out[i] = (out[i] * m) >> 12;
}
static volatile uint8_t fx_lowcut;

static int32_t lc_l1, lc_l2, lc_r1, lc_r2, dc_l, dc_r, dce_l, dce_r;

static inline int32_t dc_block(int32_t x, int32_t *dc, int32_t *err)
{
    int32_t e = (x << 6) - *dc + *err, d = e >> 12;
    *err = e - (d << 12);
    *dc += d;
    return x - ((*dc + 32) >> 6);
}

static int32_t lce[4];
static inline int32_t lowcut1(int32_t x, int32_t *lc, int32_t *err, uint32_t sh)
{
    int32_t e = x - *lc + *err, d = e >> sh;
    *err = e - (d << sh);
    *lc += d;
    return x - *lc;
}

static int32_t sb_lp1, sb_lp2, sb_lp3, sb_lp4, sb_env, sb_h1, sb_h2, sb_hl;
static inline int32_t spk_bass(int32_t m)
{
    int32_t a, t, u;
    sb_lp1 += ((m - sb_lp1) * 692) >> 15;
    sb_lp2 += ((sb_lp1 - sb_lp2) * 692) >> 15;
    sb_lp3 += ((sb_lp2 - sb_lp3) * 692) >> 15;
    sb_lp4 += ((sb_lp3 - sb_lp4) * 692) >> 15;
    a = sb_lp4 < 0 ? -sb_lp4 : sb_lp4;
    if (a > sb_env)
        sb_env += (a - sb_env) >> 2;
    else if (sb_env > 0)
        sb_env -= (sb_env >> 11) + 1;
    t = clamp(sb_lp4 * 8, -sb_env, sb_env);
    sb_h1 += (t - sb_h1) >> 5;
    u = t - sb_h1;
    sb_h2 += (u - sb_h2) >> 5;
    u -= sb_h2;
    sb_hl += (u - sb_hl) >> 3;
    return sb_hl + (sb_hl >> 1);
}

static int master_rest(void)
{
    return !(dc_l | dc_r) && lim_env <= LIM_T &&
           (!fx_lowcut || !(lc_l1 | lc_l2 | lc_r1 | lc_r2 | sb_lp1 | sb_lp2 | sb_lp3 | sb_lp4 | sb_env | sb_h1 | sb_h2 | sb_hl));
}
static inline void master_out(int32_t *l, int32_t *r)
{
    int32_t al, ar, a;
    *l = dc_block(*l, &dc_l, &dce_l);
    *r = dc_block(*r, &dc_r, &dce_r);
    if (fx_lowcut) {
        uint32_t sh = fx_lowcut == 2u ? 5u : 6u;
        int32_t b = fx_lowcut == 2u ? spk_bass((*l + *r) >> 1) : 0;
        *l = lowcut1(*l, &lc_l1, &lce[0], sh);
        *l = lowcut1(*l, &lc_l2, &lce[1], sh) + b;
        *r = lowcut1(*r, &lc_r1, &lce[2], sh);
        *r = lowcut1(*r, &lc_r2, &lce[3], sh) + b;
    }
    al = *l < 0 ? -*l : *l;
    ar = *r < 0 ? -*r : *r;
    a = al > ar ? al : ar;
    if (a > lim_env)
        lim_env += (a - lim_env) >> 2;
    else if (lim_env > LIM_T)
        lim_env -= ((lim_env - LIM_T) >> 12) + 1;
    if (lim_env > LIM_T) {
        int32_t g = (int32_t)(((uint32_t)LIM_T << 15) / (uint32_t)lim_env);
        *l = ((*l >> 4) * g) >> 11;
        *r = ((*r >> 4) * g) >> 11;
    }
    *l = softclip(*l);
    *r = softclip(*r);
}

static const uint8_t DIV_DEN[6] = {1, 2, 4, 8, 3, 6};
static uint32_t midi_beat_samples;
static uint32_t beat_samples(void)
{
    uint32_t bpm = song.g[G_BPM] >= 40 ? (uint32_t)song.g[G_BPM] : 120u;
    return song.g[G_CLOCK] && midi_beat_samples ? midi_beat_samples : (uint32_t)FS * 60u / bpm;
}
static uint32_t div_samples(uint32_t div)
{
    uint32_t quarter = beat_samples();
    return div < 6u ? quarter / DIV_DEN[div] : div < 10u ? quarter << (div - 5u) : div == 10u ? quarter / 16u : quarter / DIV_DEN[div % 6u];
}

#include "repeat.c"
#include "trap_tape.c"
#include "mfx.c"

#include "click.c"

static const uint8_t DIV_DOT[3] = {2, 3, 1};

static const uint32_t EXP2_Q15[17] = {32768, 34219, 35734, 37316, 38968, 40693, 42495, 44376, 46341, 48393, 50535, 52773, 55109, 57549, 60097, 62757, 65536};
static int32_t delay_q8(void)
{
    uint32_t e = (uint32_t)song.g[G_DTIME] * 3189u, oct = e >> 16, fr = (e >> 12) & 15u, sub = e & 4095u;
    uint32_t m = EXP2_Q15[fr] + (((uint32_t)EXP2_Q15[fr + 1u] - EXP2_Q15[fr]) * sub >> 12);
    uint32_t s = ((880u << oct) * m) >> 15;
    return (int32_t)(s >= DLY_LEN - 2u ? DLY_LEN - 2u : s) << 8;
}
static uint32_t delay_samples(void)
{
    uint32_t d = (uint32_t)song.g[G_DTIME];
    uint32_t s = d - 10u < 3u ? (beat_samples() * 3u) >> DIV_DOT[d - 10u] : div_samples(d);
    return s < 16u ? 16u : s >= DLY_LEN ? DLY_LEN - 1u : s;
}

#define QV_GAIN 22

#define QV_R 4u
#define QV_NC (CTL / QV_R)
enum { QV_D0, QV_D1, QV_D2, QV_D3, QV_MAL, QV_DL1, QV_APL, QV_DL2, QV_MAR, QV_DR1, QV_APR, QV_DR2, QV_NL };
static const uint16_t QV_LEN[QV_NL] = {37, 27, 95, 70, 169 + 8, 1122, 454, 937, 229 + 8, 1063, 669, 797};
static const uint8_t QV_IN_AP[QV_NL] = {0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0};
_Static_assert(37 + 27 + 95 + 70 + 1122 + 937 + 1063 + 669 + 797 <= sizeof rev_comb / 2u &&
               169 + 8 + 454 + 229 + 8 <= sizeof rev_u.ap / 2u, "QUAD in ROOM's buffers");
static struct {
    uint16_t w[QV_NL];
    int32_t bw, dl, dr, le, re;
    int32_t pl, pr, acc[2];
    int32_t fi[2], fo[4];
    uint32_t ph;
    int32_t frz;
} qv __attribute__((section(".pool")));

static uint8_t qv_near;
static inline int32_t qmul(int32_t a, int32_t b)
{
    int64_t p = (int64_t)a * b;
    if (qv_near)
        return (int32_t)((p + 16384) >> 15);
    return (int32_t)(p >= 0 ? p >> 15 : -((-p) >> 15));
}
static int16_t *qv_buf(uint32_t k)
{
    uint32_t j, oc = 0, oa = 0;
    for (j = 0; j < k; j++) {
        if (QV_IN_AP[j])
            oa += QV_LEN[j];
        else
            oc += QV_LEN[j];
    }
    return QV_IN_AP[k] ? rev_ap + oa : rev_comb + oc;
}
static int16_t *qv_b[QV_NL];
static const uint16_t QV_DEC[127] = {
    1997, 2154, 2317, 2489, 2668, 2854, 3048, 3249, 3458, 3674, 3897, 4127, 4364, 4607, 4857, 5113,
    5376, 5644, 5918, 6197, 6481, 6770, 7064, 7362, 7664, 7970, 8279, 8592, 8907, 9225, 9545, 9868,
    10192, 10517, 10844, 11172, 11501, 11830, 12159, 12488, 12816, 13144, 13472, 13798, 14123, 14447, 14769, 15090,
    15408, 15724, 16038, 16350, 16659, 16965, 17269, 17569, 17867, 18161, 18452, 18739, 19024, 19304, 19581, 19855,
    20124, 20390, 20653, 20911, 21165, 21416, 21663, 21905, 22144, 22379, 22610, 22837, 23060, 23280, 23495, 23706,
    23914, 24118, 24318, 24514, 24706, 24895, 25080, 25261, 25439, 25613, 25783, 25950, 26114, 26274, 26430, 26584,
    26734, 26881, 27025, 27165, 27303, 27437, 27569, 27697, 27823, 27946, 28066, 28183, 28298, 28410, 28519, 28626,
    28731, 28833, 28932, 29029, 29124, 29217, 29307, 29395, 29481, 29565, 29647, 29727, 29806, 29882, 29956};
static int32_t qv_tap(uint32_t k, uint32_t back)
{
    uint32_t L = QV_LEN[k];
    return qv_b[k][(qv.w[k] + L - back) % L];
}
static void qv_put(uint32_t k, int32_t v)
{
    qv_b[k][qv.w[k]] = (int16_t)clamp(v, -32767, 32767);
    if (++qv.w[k] >= QV_LEN[k])
        qv.w[k] = 0;
}
static int32_t qv_ap(uint32_t k, int32_t x, int32_t g)
{
    int32_t d = qv_tap(k, QV_LEN[k]), v = x - qmul(d, g);
    qv_put(k, v);
    return d + qmul(v, g);
}
static int32_t qv_mod(uint32_t k, int32_t x, int32_t g, int32_t ofs)
{
    uint32_t L = QV_LEN[k], D = ((L - 8u) << 8) + (uint32_t)(ofs + (4 << 8)), di = D >> 8, fr = D & 255u;
    int32_t a = qv_tap(k, di), b = qv_tap(k, di + 1u), d = a + (((b - a) * (int32_t)fr) >> 8), v = x - qmul(d, g);
    qv_put(k, v);
    return d + qmul(v, g);
}
static uint32_t qv_pk;
static __attribute__((noinline)) void rev_quad(const int32_t *rev_in, int32_t *out, uint32_t n)
{
    uint32_t i, j, k;
    int32_t sz = song.g[G_RSIZE], freeze = sz >= 127, dec = QV_DEC[clamp(sz, 0, 126)];
    int32_t dmp = 32767 - song.g[G_RDAMP] * 220, pl = qv.pl, pr = qv.pr;
    int32_t gr = clamp(song.g[G_RTYPE], 0, 127), q = gr * gr / 127;
    int32_t kin = 20700 + ((32767 - 20700) * q) / 127, kout = 22000 + ((32767 - 22000) * q) / 127;
    int32_t hold = q > 64 ? (q - 64) * 520 : 0, bits = 16 - (q * 8) / 127;
    (void)n;
    qv_near = qv.frz != 0 || freeze;
    qv_pk = 0;
    if (!qv_b[0])
        for (k = 0; k < QV_NL; k++)
            qv_b[k] = qv_buf(k);
    for (i = 0; i < QV_NC; i++) {
        int32_t x = 0, a, b, l, r, tri, gin, dk, dp;
        for (j = 0; j < QV_R; j++) {
            int32_t xi = qmul(clamp(rev_in[QV_R * i + j], -262143, 262143), 2580);
            qv.fi[0] += qmul(xi - qv.fi[0], kin);
            qv.fi[1] += qmul(qv.fi[0] - qv.fi[1], kin);
            x += qv.fi[1];
        }
        x >>= 2;
        qv.frz += clamp((freeze ? 32768 : 0) - qv.frz, -512, 512);
        gin = 32768 - qv.frz;
        dk = dec + (int32_t)(((int64_t)(32768 - dec) * qv.frz) >> 15);
        dp = dmp + (int32_t)(((int64_t)(32767 - dmp) * qv.frz) >> 15);
        x = (int32_t)(((int64_t)x * gin) >> 15);
        qv.bw += qmul(x - qv.bw, 20000);
        a = qv_ap(QV_D0, qv.bw, 24576);
        a = qv_ap(QV_D1, a, 24576);
        a = qv_ap(QV_D2, a, 20480);
        a = qv_ap(QV_D3, a, 20480);
        qv.ph += 272700u;
        tri = (int32_t)(qv.ph >> 23);
        tri = (tri < 256 ? tri : 511 - tri) * 8 - (4 << 8);
        tri = (int32_t)(((int64_t)tri * gin) >> 15);

        l = qv_mod(QV_MAL, a + qmul(qv.re, dk), -22938, tri);
        b = qv_tap(QV_DL1, QV_LEN[QV_DL1]);
        qv_put(QV_DL1, l);
        qv.dl += qmul(b - qv.dl, dp);
        b = qv_ap(QV_APL, qmul(qv.dl, dk), 16384);
        l = qv_tap(QV_DL2, QV_LEN[QV_DL2]);
        qv_put(QV_DL2, b);
        r = qv_mod(QV_MAR, a + qmul(qv.le, dk), -22938, -tri);
        b = qv_tap(QV_DR1, QV_LEN[QV_DR1]);
        qv_put(QV_DR1, r);
        qv.dr += qmul(b - qv.dr, dp);
        b = qv_ap(QV_APR, qmul(qv.dr, dk), 16384);
        r = qv_tap(QV_DR2, QV_LEN[QV_DR2]);
        qv_put(QV_DR2, b);
        qv.le = l;
        qv.re = r;

        l = qv_tap(QV_DL1, 67) + qv_tap(QV_DL1, 749) - qv_tap(QV_APL, 400) + qv_tap(QV_DL2, 503) - qv_tap(QV_DR1, 501) -
            qv_tap(QV_APR, 47) - qv_tap(QV_DR2, 269);
        r = qv_tap(QV_DR1, 89) + qv_tap(QV_DR1, 914) - qv_tap(QV_APR, 309) + qv_tap(QV_DR2, 674) - qv_tap(QV_DL1, 532) -
            qv_tap(QV_APL, 84) - qv_tap(QV_DL2, 30);
        l = qmul(l, 19661);
        r = qmul(r, 19661);
        qv_pk |= (uint32_t)(l < 0 ? -l : l) | (uint32_t)(r < 0 ? -r : r);
        if (bits < 16) {
            int32_t h = 1 << (15 - bits);
            l = (l + h) & ~(2 * h - 1);
            r = (r + h) & ~(2 * h - 1);
        }
        for (j = 0; j < QV_R; j++) {
            int32_t lo = pl + (((l - pl) * (int32_t)(j + 1u)) >> 2), ro = pr + (((r - pr) * (int32_t)(j + 1u)) >> 2);
            lo += qmul(pl - lo, hold);
            ro += qmul(pr - ro, hold);
            qv.fo[0] += qmul(lo - qv.fo[0], kout);
            qv.fo[1] += qmul(qv.fo[0] - qv.fo[1], kout);
            qv.fo[2] += qmul(ro - qv.fo[2], kout);
            qv.fo[3] += qmul(qv.fo[2] - qv.fo[3], kout);
            lo = qv.fo[1];
            ro = qv.fo[3];
            out[QV_R * i + j] += ((lo + ro) >> 1) * QV_GAIN;
            rev_side[QV_R * i + j] = ((lo - ro) >> 1) * QV_GAIN;
        }
        pl = l;
        pr = r;
    }
    qv.pl = pl;
    qv.pr = pr;
    hl.side = 1;
}

#if FELUCCA_CPU2
/* the dry mix and the sends, as each core adds its parts into them (after Jangada: mx[1] is the second core's,
 * mix_block), and a part's own block (b; mx[0].b also the fade of a reverb model change, after the parts) */
typedef struct { int32_t l[CTL], r[CTL], d[CTL], v[CTL], b[CTL]; } mixsum_t;
static mixsum_t mx[2];
#define mix_l (mx[0].l)
#define mix_r (mx[0].r)
#define send_d (mx[0].d)
#define send_r (mx[0].v)
#define part_buf (mx[0].b)
#define SM_DECL , mixsum_t *sm
#define SM_ARG(x) , (x)
#define SM(f) (sm->f)
#else

static int32_t part_buf[CTL];
#define SM_DECL
#define SM_ARG(x)
#define SM(f) SM1_##f
#define SM1_l mix_l
#define SM1_r mix_r
#define SM1_d send_d
#define SM1_v send_r
#define SM1_b part_buf
#endif

#define GAIN_SLEW 256u
static void gain_ramp(int32_t *buf, uint32_t n, int32_t *cur, int32_t to)
{
    int32_t g = *cur, st, mx = (int32_t)(GAIN_SLEW * n);
    uint32_t i;
    if (g == to && to >= 32767)
        return;
    to = clamp(to, g - mx, g + mx);
    st = (to - g) / (int32_t)(n ? n : 1u);
    for (i = 0; i < n; i++) {
        g = i + 1u == n ? to : g + st;
        buf[i] = (int32_t)(((int64_t)buf[i] * g) >> 15);
    }
    *cur = to;
}

static void fx_panic(void)
{
    memset(dly_buf, 0, sizeof dly_buf);
    memset(rev_comb, 0, sizeof rev_comb);
    memset(&rev_u, 0, sizeof rev_u);
    memset(mfx_cb, 0, sizeof mfx_cb);
    tape_reset();
}
static int32_t fxs_cur[4] = {32767, 32767, 32767, 32767}, amp_cur = 32767, amp_cr = 32767;

#define QV_REST_N 44100u
#define DLY_REST 64
#define FX_REST_PAD 4096u
static uint32_t qv_rest, dly_rest;
static int fx_quiet(const int32_t *x, uint32_t n)
{
    uint32_t i, o = 0;
    for (i = 0; i < n; i++)
        o |= (uint32_t)x[i];
    return o == 0u;
}
static void fx_buses(const int32_t *dly_in, const int32_t *rev_in, int32_t *wet,
                     uint32_t n)
{
    uint32_t i, dl = delay_samples(), dn = n;
    int32_t dpk = 0;
    int32_t fb = song.g[G_DFDBK] * 260, col = 2000 + song.g[G_DCOLOR] * 240, hpk = 700;
    int32_t dtarget = delay_q8();
    {
        int32_t tn = song.g[G_DCOLOR] - 64;
        col = tn < 0 ? 14000 + tn * 180 : 14000 + tn * 127;
        hpk = tn > 0 ? 700 + tn * 130 : 700;
    }
#define SIMPLE_DT(dl) (fx.dly_cur)
    int32_t dmix = song.g[G_DMIX] * 258;
    int32_t rt;
    if (!fx_quiet(dly_in, n))
        dly_rest = 0;
    else if (dly_rest > (uint32_t)(fx.dly_cur >> 8) + FX_REST_PAD) {
        fx.dly_cur = dtarget;
        fx.dly_ph += 77900u * n;
        fx.dly_w += n;
        for (i = 0; i < n; i++)
            wet[i] = 0;
        dn = 0;
    }
    for (i = 0; i < dn; i++) {
        int32_t y = 0, x;

        {
            int32_t dm, pos, f, x0, x1, lp;
            uint32_t ri;
            {
                int32_t d = dtarget - fx.dly_cur, step = d >> 12;
                if (!fx.dly_cur)
                    d = step = dtarget, fx.dly_cur = 0;
                else if (d > 0 && !step)
                    step = 1;
                step = step > 128 ? 128 : step < -128 ? -128 : step;
                fx.dly_cur += step;
                dl = (uint32_t)fx.dly_cur >> 8;
            }
            dm = (int32_t)(dl >> 2) < 20 ? (int32_t)(dl >> 2) : 20;
            fx.dly_ph += 77900u;
            pos = (int32_t)(SIMPLE_DT(dl)) + ((osc_sine(fx.dly_ph) * dm) >> 7);
            ri = (uint32_t)pos >> 8;
            f = pos & 255;
            x0 = dly_buf[(fx.dly_w - ri) & (DLY_LEN - 1u)];
            x1 = dly_buf[(fx.dly_w - ri - 1u) & (DLY_LEN - 1u)];
            x = x0 + (((x1 - x0) * f) >> 8);
            fx.dly_lp += mulq15(x - fx.dly_lp, col);
            fx.dly_lp2 += mulq15(fx.dly_lp - fx.dly_lp2, col);
            fx.dly_hp += mulq15(fx.dly_lp2 - fx.dly_hp, hpk);
            lp = fx.dly_lp2 - fx.dly_hp;
            dly_buf[fx.dly_w & (DLY_LEN - 1u)] =
                (int16_t)clamp((dly_in[i] >> 1) + softclip(mulq15(lp, fb) * 5 >> 2), -32768, 32767);
            x = lp;
            dpk |= lp + DLY_REST;
        }
        fx.dly_w++;
        y += mulq15(x << 1, dmix);
        wet[i] = y;
    }
    if (dn)
        dly_rest = (dpk & ~127) || !fx_quiet(dly_in, n) ? 0u : dly_rest + n;
    hl.side = 0;
    (void)rt;
    gain_ramp(wet, n, &fxs_cur[2], fxs_gain[2]);
    if (!fx_quiet(rev_in, n) || qv_near || song.g[G_RSIZE] >= 127)
        qv_rest = 0;
    else if (qv_rest > QV_REST_N) {
        uint32_t k;
        for (k = 0; k < QV_NL; k++)
            qv.w[k] = (uint16_t)((qv.w[k] + n / QV_R) % QV_LEN[k]);
        qv.ph += 272700u * (n / QV_R);
        return;
    }
    if (fxs_gain[3] >= 32767 && fxs_cur[3] >= 32767) {
        rev_quad(rev_in, wet, n);
    } else {
        static int32_t rw[CTL];
        int32_t c = fxs_cur[3];
        for (i = 0; i < n; i++)
            rw[i] = 0;
        rev_quad(rev_in, rw, n);
        gain_ramp(rev_side, n, &c, fxs_gain[3]);
        gain_ramp(rw, n, &fxs_cur[3], fxs_gain[3]);
        for (i = 0; i < n; i++)
            wet[i] += rw[i];
    }
    qv_rest = qv_pk || !fx_quiet(rev_in, n) ? 0u : qv_rest + n;
}

static __attribute__((noinline)) void rev_side_mix(int32_t *l, int32_t *r, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        l[i] += rev_side[i];
        r[i] -= rev_side[i];
    }
}

static void events_block(uint32_t n);
#if FELUCCA_CPU2
static int32_t wet[CTL];
#else
static int32_t send_d[CTL], send_r[CTL], wet[CTL], mix_l[CTL], mix_r[CTL];
#endif

static __attribute__((noinline)) void mix_spread(track_t *t, int32_t *b, uint32_t n SM_DECL)
{
    int32_t *d = SPREAD_BUF(t);
    uint32_t i;
    int32_t lvl = LEVEL_Q12[t->p[P_LEVEL] & 127], pan = t->p[P_PAN], s = (t->p[P_SPRD] + 1) >> 1;
    int32_t pa = clamp(pan - s, -64, 63), pb = clamp(pan + s, -64, 63);
    int32_t gla = 4096 - (pa > 0 ? pa * 64 : 0), gra = 4096 + (pa < 0 ? pa * 64 : 0);
    int32_t glb = 4096 - (pb > 0 ? pb * 64 : 0), grb = 4096 + (pb < 0 ? pb * 64 : 0);
    int32_t glm = (gla + glb) >> 1, grm = (gra + grb) >> 1, gld = (gla - glb) >> 1, grd = (gra - grb) >> 1;
    int32_t dl = t->p[P_DLY] * 258, r = t->p[P_REV] * 258, pk = t->peak;
    int32_t xmax = dl;
    xmax = 0x7FFFFFFF / ((xmax > r ? xmax : r) | 1);
    for (i = 0; i < n; i++)
        d[i] = (d[i] << 1) - b[i];
    for (i = 0; i < n; i++) {
        int32_t x = mulq12r(b[i], lvl), a = x < 0 ? -x : x, y = mulq12r(d[i], lvl);
        int32_t xs = clamp(x, -xmax, xmax);
        if (a > pk)
            pk = a;
        if (dl)
            SM(d)[i] += mulq15(xs, dl);
        if (r)
            SM(v)[i] += mulq15(xs, r);
        SM(l)[i] += ((x * glm) >> 12) + ((y * gld) >> 12);
        SM(r)[i] += ((x * grm) >> 12) + ((y * grd) >> 12);
    }
    t->peak = pk;
}

/* a part's cost for the second core's split (mix_split, after Jangada's): its voices x its engine's cost per
 * voice (ENG_VCOST: the host's instructions per sample, the CPU at 8 notes held less the idle mix, over the
 * engine's voices), and the DIST, INSERT and sends while it sounds or its tail runs. ENGINES[] order; DIGITAL as FM6
 * when built, nothing when retired. An estimate: it only steers the split, the sound does not depend on it */
#if FELUCCA_CPU2
static uint16_t part_cost[NTRK];
static const uint16_t ENG_VCOST[NENGINES] = {181, 181, 181, 181, 181};
#endif

static int32_t mix_was[NTRK][5];
static __attribute__((noinline)) void mix_glide(track_t *t, const int32_t *b, uint32_t n, const int32_t *to SM_DECL)
{
    int32_t *w = mix_was[(uint32_t)(t - trk) % NTRK], c[5], st[5], pk = t->peak, sm0, xm;
    uint32_t i, k;
    sm0 = w[3] > w[4] ? w[3] : w[4];
    sm0 = to[3] > sm0 ? to[3] : sm0;
    sm0 = to[4] > sm0 ? to[4] : sm0;
    xm = 0x7FFFFFFF / (sm0 | 1);
    for (k = 0; k < 5u; k++) {
        c[k] = w[k] << 8;
        st[k] = ((to[k] - w[k]) << 8) / (int32_t)n;
        w[k] = to[k];
    }
    for (i = 0; i < n; i++) {
        int32_t x, a, xs, dv, rv;
        for (k = 0; k < 5u; k++)
            c[k] += st[k];
        dv = c[3] >> 8;
        rv = c[4] >> 8;
        x = mulq12r(b[i], c[0] >> 8);
        a = x < 0 ? -x : x;
        xs = clamp(x, -xm, xm);
        if (a > pk)
            pk = a;
        if (dv)
            SM(d)[i] += mulq15(xs, dv);
        if (rv)
            SM(v)[i] += mulq15(xs, rv);
        SM(l)[i] += (x * (c[1] >> 8)) >> 12;
        SM(r)[i] += (x * (c[2] >> 8)) >> 12;
    }
    t->peak = pk;
}

static int cb_tail(uint32_t k, int32_t *b, uint32_t n)
{
    uint32_t i;
    if (cb_at != k || !cb_live())
        return 0;
    for (i = 0; i < n; i++)
        b[i] = 0;
    return 1;
}
static void mix_part(track_t *t, uint32_t n SM_DECL)
{
    int32_t *b = SM(b);
    uint32_t i;
#if FELUCCA_CPU2
    uint32_t nr;
#endif
    mod_begin(t);
    T_SPREAD(t) = t->p[P_SPRD] > 0 && (trk_vmode(t) == V_POLY || trk_vmode(t) == V_UNISON)
                  && !dj_on((uint32_t)(t - trk)) && !t->p[P_MUTE];
#if FELUCCA_CPU2
    nr = track_render(t, b, n);
    part_cost[(uint32_t)(t - trk) % NTRK] = (uint16_t)(nr * ENG_VCOST[t->engine % NENGINES] + (nr ? 60u : 0u));
    if (nr || cb_tail((uint32_t)(t - trk), b, n))
#else
    if (track_render(t, b, n) || cb_tail((uint32_t)(t - trk), b, n))
#endif
        ;
    else {
        int32_t lv = t->p[P_MUTE] ? 0 : LEVEL_Q12[t->p[P_LEVEL] & 127], pa = t->p[P_PAN], *w = mix_was[(uint32_t)(t - trk) % NTRK];
        w[0] = lv;
        w[1] = 4096 - (pa > 0 ? pa * 64 : 0);
        w[2] = 4096 + (pa < 0 ? pa * 64 : 0);
        w[3] = t->p[P_DLY] * 258;
        w[4] = t->p[P_REV] * 258;
        if (MOD(t).on)
            mod_end(t);
        return;
    }
    dj_part((uint32_t)(t - trk), b, n);
    if (cb_at == (uint32_t)(t - trk))
        cb_run(b, 0, n);
    if (T_SPREAD(t))
        mix_spread(t, b, n SM_ARG(sm));
    else {
    }
    if (!T_SPREAD(t)) {

        int32_t lvl = t->p[P_MUTE] ? 0 : LEVEL_Q12[t->p[P_LEVEL] & 127], pan = t->p[P_PAN];
        int32_t gl = 4096 - (pan > 0 ? pan * 64 : 0), gr = 4096 + (pan < 0 ? pan * 64 : 0);
        int32_t d = t->p[P_DLY] * 258, r = t->p[P_REV] * 258, pk = t->peak;
        int32_t xmax = d;
        const int32_t *was = mix_was[(uint32_t)(t - trk) % NTRK];
        xmax = 0x7FFFFFFF / ((xmax > r ? xmax : r) | 1);
        if (was[0] != lvl || was[1] != gl || was[2] != gr || was[3] != d || was[4] != r) {
            int32_t to[5];
            to[0] = lvl;
            to[1] = gl;
            to[2] = gr;
            to[3] = d;
            to[4] = r;
            mix_glide(t, b, n, to SM_ARG(sm));
            pk = t->peak;
            n = 0;
        }
        for (i = 0; i < n; i++) {
            int32_t x = mulq12r(b[i], lvl), a = x < 0 ? -x : x;
            int32_t xs = clamp(x, -xmax, xmax);
            if (a > pk)
                pk = a;
            if (d)
                SM(d)[i] += mulq15(xs, d);
            if (r)
                SM(v)[i] += mulq15(xs, r);
            SM(l)[i] += (x * gl) >> 12;
            SM(r)[i] += (x * gr) >> 12;
        }
        t->peak = pk;
    }
    if (MOD(t).on)
        mod_end(t);
}

#define SCOPE_N 512u
static int16_t scope_buf[SCOPE_N];
static uint32_t scope_w;
static volatile uint8_t scope_mix;
static __attribute__((noinline)) void scope_take_mix(uint32_t n)
{
    uint32_t i;
    for (i = 1; i < n; i += 2u)
        scope_buf[scope_w++ & (SCOPE_N - 1u)] = (int16_t)clamp(mix_l[i] + wet[i], -32767, 32767);
}

static __attribute__((noinline)) void perf_master(int32_t *out, uint32_t n)
{
    uint32_t i;
    int32_t mg = fx_usb_fixed ? MASTER_FULL : (int32_t)song.master_q12;
    for (i = 0; i < n; i++) {
        mix_l[i] = mulq12r(mix_l[i] + wet[i], mg);
        mix_r[i] = mulq12r(mix_r[i] + wet[i], mg);
    }
    perf_block(mix_l, mix_r, n);
    mfx_block(mix_l, mix_r, n);
    tape_block(mix_l,mix_r,n,mg*8,mg*8);
    for (i = 0; i < n; i++) {
        int32_t l = mix_l[i], r = mix_r[i];
        master_out(&l, &r);
        out[2u * i] = l;
        out[2u * i + 1u] = r;
    }
}

/* ---- the second core (hal/fm1_cpu1.h; after Jangada's, github.com/zednaked/jangada, GPL-3.0) renders some of
 * the parts of each block into its own sums (mx[1]) while this core renders the others; then this core adds them in
 * and goes on (the LFOs' blocks, the FX layer, the buses, the master) as with one core. The sums are integers, so the
 * order they add up in changes nothing: the output is bit for bit one core's. A part's render touches its own state
 * only (its track_t with its SPREAD, ENV SYNC and LFO phase, its voices, its matrix: mod.c MOD, its SPREAD buffer,
 * its engine's memory) and what nobody writes meanwhile (song, the tables); what parts share is done before the
 * split (events_block: the notes, voice stealing; the FX layer's perf_begin) or after it (the LFOs' S&H draw from the
 * shared generator, part by part: voice.c track_lfo_end), checked under ThreadSanitizer too.
 * Without a second core (the host, the browser, MENU > 2ND CORE OFF, one that did not answer or stopped) C1_RUN takes
 * nothing and this core renders them all */
#if FELUCCA_CPU2
#ifndef C1_RUN
#define C1_ON() 0
#define C1_RUN(fn, arg) 0
#define C1_WAIT() 0
#endif
#ifndef C1_SPLIT
#define C1_SPLIT() mix_split()
#endif
static uint32_t c1_blocks;

static void c1_parts(uint32_t arg)
{
    uint32_t i, n = arg >> 8;
    mixsum_t *sm = &mx[1];
    for (i = 0; i < n; i++)
        sm->l[i] = sm->r[i] = sm->d[i] = sm->v[i] = 0;
    for (i = 0; i < NPART; i++)
        if (arg >> i & 1u)
            mix_part(&trk[i], n, sm);
}

#define C1_HAND 40u
static uint32_t mix_split(void)
{
    uint32_t m, i, best = 0, all = 0, lo, c[NPART];
    for (i = 0; i < NPART; i++) {

        uint32_t k, nv = 0, now;
        for (k = 0; k < NVOICE; k++)
            nv += trk[i].v[k].active != 0;
        now = nv * ENG_VCOST[eng_idx(trk[i].eng_req)] + (nv ? 60u : 0u);
        c[i] = now > part_cost[i] ? now : part_cost[i];
        all += c[i];
    }
    lo = all;
    for (m = 1; m < (1u << NPART) - 1u; m++) {
        uint32_t a = 0, w;
        for (i = 0; i < NPART; i++)
            if (m >> i & 1u)
                a += c[i];
        w = (a > all - a ? a : all - a) + C1_HAND;
        if (w < lo) {
            lo = w;
            best = m;
        }
    }
    return best;
}
#endif

static void amp_block(uint32_t n)
{
    int32_t v = song.g[G_AMP], to = v >= 127 ? 32767 : (v * v * 32767) / (127 * 127), p = song.g[G_PAN];
    gain_ramp(mix_l, n, &amp_cur, p > 0 ? to * (64 - p) / 64 : to);
    gain_ramp(mix_r, n, &amp_cr, p < 0 ? to * (64 + p) / 64 : to);
}

static void mix_block(int32_t *out, uint32_t n)
{
    uint32_t i;
    int perf;
    int32_t mg = fx_usb_fixed ? MASTER_FULL : (int32_t)song.master_q12;
    for (i = 0; i < n; i++)
        send_d[i] = send_r[i] = mix_l[i] = mix_r[i] = 0;
    mfx_g_init();
    cb_latch();
    events_block(n);
    perf = perf_begin(n);
#if FELUCCA_CPU2
    uint32_t m = C1_ON() ? C1_SPLIT() & ((1u << NPART) - 1u) : 0u;
    if (m && !C1_RUN(c1_parts, m | n << 8))
        m = 0;
    c1_blocks += m != 0;
    for (i = 0; i < NPART; i++)
        if (!(m >> i & 1u))
            mix_part(&trk[i], n, &mx[0]);
    if (m) {
        const mixsum_t *sm = &mx[1];
        if (C1_WAIT())
            c1_parts(m | n << 8);
        for (i = 0; i < n; i++) {
            mix_l[i] += sm->l[i];
            mix_r[i] += sm->r[i];
            send_d[i] += sm->d[i];
            send_r[i] += sm->v[i];
        }
    }
    for (i = 0; i < NPART; i++)
        track_lfo_end(&trk[i]);
#else
    for (i = 0; i < NPART; i++)
        mix_part(&trk[i], n);
#endif
    mod_g_begin(n);
    gain_ramp(send_d, n, &fxs_cur[0], fxs_gain[0]);
    gain_ramp(send_r, n, &fxs_cur[1], fxs_gain[1]);
    fx_buses(send_d, send_r, wet, n);
    if (hl.side)
        rev_side_mix(mix_l, mix_r, n);
    if (scope_mix)
        scope_take_mix(n);
    if (perf) {
        perf_master(out, n);
        mod_g_end();
        return;
    }
    for (i = 0; i < n; i++) {
        mix_l[i] = mulq12r(mix_l[i] + wet[i], mg);
        mix_r[i] = mulq12r(mix_r[i] + wet[i], mg);
    }
    mfx_block(mix_l, mix_r, n);
    amp_block(n);
    tape_block(mix_l,mix_r,n,(mg*amp_cur)>>12,(mg*amp_cr)>>12);
    mod_g_end();
    if (fx_quiet(mix_l, n) && fx_quiet(mix_r, n) && master_rest()) {
        for (i = 0; i < 2u * n; i++)
            out[i] = 0;
        return;
    }
    for (i = 0; i < n; i++) {
        int32_t l = mix_l[i], r = mix_r[i];
        master_out(&l, &r);
        out[2u * i] = l;
        out[2u * i + 1u] = r;
    }
    return;
    for (i = 0; i < n; i++) {
        int32_t l = mulq12r(mix_l[i] + wet[i], mg);
        int32_t r = mulq12r(mix_r[i] + wet[i], mg);
        master_out(&l, &r);
        out[2u * i] = l;
        out[2u * i + 1u] = r;
    }
}

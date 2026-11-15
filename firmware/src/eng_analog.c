/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Blair (blairysbeatlab), BBL JAM BOX */

#define CRUSH_RATE 53
#define CRUSH_BITS 11

enum { AN_KICK, AN_SNARE, AN_TOM, AN_RIM, AN_CLAP, AN_COW, AN_HAT, AN_CYM, AN_BASS, AN_N };
static const char *const N_AMACH[AN_N] = {"KICK", "SNARE", "TOM", "RIM", "CLAP", "COWB", "HAT", "CYM", "BASS"};

static const char *const AN_LABEL[AN_N][8] = {
    {"TUNE", "DECAY", "CLICK", "TONE", "SWEEP", "S.TIM", "DRIVE", "LEVEL"},
    {"TUNE", "TONE", "SNAP", "DECAY", "BODY", "B.DEC", "DRIVE", "LEVEL"},
    {"TUNE", "DECAY", "SWEEP", "CLICK", "TONE", "NOISE", "DRIVE", "LEVEL"},
    {"TUNE", "DECAY", "TONE", "CLICK", "RATIO", "Q", "DRIVE", "LEVEL"},
    {"TUNE", "DECAY", "SPRD", "TONE", "BURST", "ROOM", "DRIVE", "LEVEL"},
    {"TUNE", "DECAY", "TONE", "CLICK", "DETUN", "Q", "DRIVE", "LEVEL"},
    {"TUNE", "DECAY", "TONE", "METAL", "HPF", "SNAP", "DRIVE", "LEVEL"},
    {"TUNE", "DECAY", "TONE", "METAL", "SHIM", "BODY", "DRIVE", "LEVEL"},
    {"TUNE", "DECAY", "CLICK", "TONE", "SWEEP", "S.TIM", "DRIVE", "LEVEL"},
};
static const uint8_t AN_DEF[AN_N][8] = {
    {55, 70, 45, 80, 50, 40, 15, 110}, {64, 70, 80, 60, 80, 60, 20, 105}, {50, 64, 20, 30, 90, 20, 10, 105},
    {64, 40, 70, 50, 64, 60, 15, 100}, {64, 60, 50, 50, 64, 40, 15, 100}, {64, 50, 64, 40, 46, 50, 10, 95},
    {64, 25, 64, 80, 60, 50, 10, 90}, {64, 80, 70, 80, 80, 40, 10, 85},
    {64, 100, 3, 45, 10, 15, 40, 100},
};

static const uint16_t AN_TRIM[AN_N] = {4096, 4096, 4096, 5120, 12800, 5120, 36000, 18400, 4096};
#define AN_NOTE 60

static const preset_t ANALOG_PRESETS[AN_N] = {
#define AP(n, a, b, c, d, e, f, g, m) {n, {a, b, c, d, e, f, g, m}, {0, 100, 0, 40}, 0, 0, FX(0, 0, 0, 0), PAT(0)}
    AP("KICK", 55, 70, 45, 80, 50, 40, 15, 0), AP("SNARE", 64, 70, 80, 60, 80, 60, 20, 1),
    AP("TOM", 50, 64, 20, 30, 90, 20, 10, 2), AP("RIM", 64, 40, 70, 50, 64, 60, 15, 3),
    AP("CLAP", 64, 60, 50, 50, 64, 40, 15, 4), AP("COWBELL", 64, 50, 64, 40, 46, 50, 10, 5),
    AP("HAT", 64, 25, 64, 80, 60, 50, 10, 6), AP("CYMBAL", 64, 80, 70, 80, 80, 40, 10, 7),
    AP("BASS", 64, 100, 3, 45, 10, 15, 40, AN_BASS),
#undef AP
};

#define AQ(x) ((int64_t)((x) * 65536.0 + 0.5))
#define AN_FLOOR 26844u
#define AN_DECLICK 64u

static int64_t an_exp2(int64_t x)
{
    int64_t i = x >> 16, f = (x & 0xFFFF) << 14, p;
    p = 16377;
    p = 165394 + ((p * f) >> 30);
    p = 1431680 + ((p * f) >> 30);
    p = 10327387 + ((p * f) >> 30);
    p = 59597083 + ((p * f) >> 30);
    p = 257941248 + ((p * f) >> 30);
    p = 744261118 + ((p * f) >> 30);
    p = 1073741824 + ((p * f) >> 30);
    if (i >= 0)
        return i > 30 ? (int64_t)1 << 46 : (p << i) >> 14;
    return i < -40 ? 0 : (p >> -i) >> 14;
}

static int64_t an_log2(int64_t v)
{
    int64_t r = 0, m;
    int32_t n = 0, b;
    if (v <= 0)
        return -((int64_t)40 << 16);
    while (v >= ((int64_t)2 << 16)) { v >>= 1; n++; }
    while (v < ((int64_t)1 << 16)) { v <<= 1; n--; }
    m = v << 14;
    for (b = 15; b >= 0; b--) {
        m = (m * m) >> 30;
        if (m >= ((int64_t)2 << 30)) {
            m >>= 1;
            r |= (int64_t)1 << b;
        }
    }
    return ((int64_t)n << 16) + r;
}

static int64_t an_div(int64_t n, int64_t d)
{
    uint64_t un = (uint64_t)(n < 0 ? -n : n), ud = (uint64_t)(d < 0 ? -d : d), q = 0;
    int32_t s = 0;
    if (!ud)
        return 0;
    while (ud < un && !(ud >> 62)) {
        ud <<= 1;
        s++;
    }
    for (; s >= 0; s--) {
        q <<= 1;
        if (un >= ud) {
            un -= ud;
            q |= 1u;
        }
        ud >>= 1;
    }
    return (n < 0) != (d < 0) ? -(int64_t)q : (int64_t)q;
}
static int64_t an_pow(int64_t b, int64_t e) { return b <= 0 ? 0 : an_exp2((an_log2(b) * e) >> 16); }
static int64_t an_mul(int64_t a, int64_t b) { return (a * b) >> 16; }
static int64_t an_x(int32_t p) { return (int64_t)((clamp(p, 0, 127) << 16) / 127); }
static uint32_t an_samples(int64_t sec) { int64_t n = (sec * FS) >> 16; return n < 1 ? 1u : (uint32_t)n; }
static uint32_t an_inc(int64_t hz)
{
    hz = clamp((int32_t)(hz >> 4), 0, (int32_t)(AQ(21000.0) >> 4));
    return (uint32_t)(((uint64_t)hz << 4) * 1558265u >> 20);
}

static uint32_t an_dec(int64_t a, uint32_t n)
{
    int64_t u = an_div(a * 1488522236, (int64_t)n << 16), d, t;
    if (u > ((int64_t)3 << 29))
        u = (int64_t)3 << 29;
    t = u; d = u;
    t = (t * u) >> 31; d -= t >> 1;
    t = (t * u) >> 31; d += an_div(t, 6);
    t = (t * u) >> 31; d -= an_div(t, 24);
    t = (t * u) >> 31; d += an_div(t, 120);
    t = (t * u) >> 31; d -= an_div(t, 720);
    return d <= 0 ? 1u : (uint32_t)(d << 1);
}

typedef struct {
    uint32_t v;
    int32_t step;
    uint32_t na, n1, pre;
    uint32_t d, d2;
} an_env_t;

static void an_env(an_env_t *e, int64_t pk, int64_t dec, int64_t at)
{
    int64_t p28 = pk << 12;
    e->pre = 0;
    e->n1 = 0;
    if (p28 <= (int64_t)AN_FLOOR) {
        e->v = 0;
        return;
    }
    e->v = AN_FLOOR;
    e->na = an_samples(at);
    e->step = (int32_t)an_div(p28 - (int64_t)AN_FLOOR, e->na);
    e->d = e->d2 = an_dec(an_log2(pk) + 870824, an_samples(dec));
}

static inline uint32_t an_env_tick(an_env_t *e)
{
    uint32_t v = e->v;
    if (!v)
        return 0;
    if (e->pre) {
        e->pre--;
        return 0;
    }
    if (e->na) {
        e->na--;
        e->v = v + (uint32_t)e->step;
        return v;
    }
    if (e->n1 && !--e->n1)
        e->d = e->d2;
    e->v = v - (uint32_t)(((uint64_t)v * e->d) >> 32);
    if (e->v < AN_FLOOR)
        e->v = 0;
    return v;
}

typedef struct { uint32_t f, d, n; } an_swp_t;
static void an_sweep(an_swp_t *s, int64_t r, int64_t sec)
{
    s->n = an_samples(sec);
    s->f = (uint32_t)(clamp((int32_t)(r >> 4), 1 << 12, 7 << 12) << 16);
    s->d = an_dec(an_log2(r), s->n);
}
static inline uint32_t an_sweep_tick(an_swp_t *s)
{
    uint32_t f = s->f;
    if (s->n) {
        s->n--;
        s->f = s->n ? f - (uint32_t)(((uint64_t)f * s->d) >> 32) : 1u << 28;
    }
    return f;
}

typedef struct { int32_t bl, bh, bb, c1, c2, x1, x2, y1, y2; } an_svf_t;
#define AN_QDB05 61870
#define AN_QDB06 61162
#define AN_QDB07 60462
static int32_t an_kq(int64_t q) { return (int32_t)an_div((int64_t)1 << 32, q < 1024 ? 1024 : q); }
static void an_svf_set(an_svf_t *f, int64_t hz, int32_t k)
{
    uint32_t inc = an_inc(hz);
    int64_t th = (int64_t)(((uint64_t)inc * 3373259426u) >> 32), g, g2, kg, r;
    if (th < ((int64_t)1 << 28)) {
        int64_t t2 = (th * th) >> 30;
        g = th + an_div((th * t2) >> 30, 3) + an_div(((((th * t2) >> 30) * t2) >> 30) * 2, 15);
    } else {
        int64_t sn = sine_i(inc >> 1), c = sine_i((inc >> 1) + 0x40000000u);
        g = c > 0 ? an_div(sn << 30, c) : (int64_t)60 << 30;
    }
    g2 = (g * g) >> 30;
    kg = ((int64_t)k * g) >> 16;
    r = an_div((int64_t)1 << 59, ((int64_t)1 << 30) + kg + g2);
    memset(f, 0, sizeof *f);
    f->bl = (int32_t)((g2 * r) >> 30);
    f->bh = (int32_t)r;
    f->bb = (int32_t)((kg * r) >> 30);
    f->c1 = (int32_t)((2 * (((int64_t)1 << 30) - g2) * r) >> 30);
    f->c2 = (int32_t)(-(((((int64_t)1 << 30) - kg + g2) * r) >> 30));
}
static inline int32_t an_bq(an_svf_t *f, int32_t b, int32_t xs)
{
    int32_t y = (int32_t)((((int64_t)b * xs << 8) + (int64_t)f->c1 * f->y1 + (int64_t)f->c2 * f->y2) >> 29);
    f->y2 = f->y1;
    f->y1 = y;
    return y >> 8;
}
static inline int32_t an_lp(an_svf_t *f, int32_t x)
{
    int32_t xs = x + 2 * f->x1 + f->x2;
    f->x2 = f->x1;
    f->x1 = x;
    return an_bq(f, f->bl, xs);
}
static inline int32_t an_hp(an_svf_t *f, int32_t x)
{
    int32_t xs = x - 2 * f->x1 + f->x2;
    f->x2 = f->x1;
    f->x1 = x;
    return an_bq(f, f->bh, xs);
}
static inline int32_t an_bp(an_svf_t *f, int32_t x)
{
    int32_t xs = x - f->x2;
    f->x2 = f->x1;
    f->x1 = x;
    return an_bq(f, f->bb, xs);
}

static const int16_t AN_TANH[65] = {
    0, 4075, 8025, 11742, 15142, 18173, 20812, 23065, 24955, 26518, 27796, 28829, 29659, 30321, 30846, 31261, 31588,
    31845, 32047, 32205, 32328, 32425, 32500, 32559, 32605, 32641, 32669, 32690, 32707, 32720, 32731, 32739, 32745,
    32750, 32754, 32757, 32759, 32761, 32762, 32763, 32764, 32765, 32765, 32766, 32766, 32766, 32766, 32766, 32767,
    32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767, 32767};
static inline int32_t an_tanh(int32_t u)
{
    int32_t a = u < 0 ? -u : u, i = a >> 12, y;
    y = i >= 64 ? 32767 : AN_TANH[i] + (((AN_TANH[i + 1] - AN_TANH[i]) * (a & 4095)) >> 12);
    return u < 0 ? -y : y;
}

typedef struct {
    uint8_t mach, owner, nb, b;
    int32_t seed;
    int32_t dc, dcn;
    int32_t last;
    int32_t pitch16;
    uint32_t ph[6], inc[6];
    int32_t g0, g1;
    uint32_t bn, bc, bd, btail;
    an_env_t e[3];
    an_swp_t sw;
    an_svf_t f[4];
    int32_t gk;
    uint32_t gq, gqi, gi;
    uint32_t cp;
    int32_t ch;
    int32_t acc;
} an_v_t;
/* TRAPSTATION ACCENT, after X0X's 808 (upstream fm1-x0x dsp/drum808.c: 8W8's velocity line, the trigger 4 V normal ..
 * 7.3 V accented, the circuit driven harder): an accented hit (velocity 127: a step's ACC, a roll's LAST) is struck
 * harder by the track's AMT (P_ED_FX on an 808, 0..127, params.c): its level up to x1.99 (+6 dB, x0x' full velocity
 * gain), its transient (click / snap) up to x1.6 and its DRIVE up by 0.35 of the knob's range. Not accented: as BBL */
static const int8_t AN_ACC_P[AN_N] = {2, 2, 3, 3, -1, 3, 5, -1, 2};
static an_v_t an_v[NTRK][2] __attribute__((section(".pool")));
static struct { int32_t rng; } an_t[NTRK];

static inline int32_t an_noise(int32_t *s) { return (int32_t)noise32(s) >> 16; }

static an_v_t *an_of(track_t *t, voice_t *v)
{
    uint32_t tr = (uint32_t)(t - trk) % NTRK, vi = (uint32_t)(v - t->v) + 1u;
    return an_v[tr][0].owner == vi ? &an_v[tr][0] : an_v[tr][1].owner == vi ? &an_v[tr][1] : 0;
}

static int64_t an_kick_body(an_v_t *h, const int64_t *P, int64_t base, int64_t df)
{
    int64_t d;
    d = an_mul(AQ(0.05) + an_mul(an_pow(P[1], AQ(2.2)), AQ(3.0)), df);
    h->inc[0] = an_inc(base);
    an_sweep(&h->sw, AQ(1.0) + an_mul(P[4], AQ(5.0)), AQ(0.008) + an_mul(P[5], AQ(0.1)));
    an_env(&h->e[0], AQ(1.0), d, AQ(0.001));
    an_svf_set(&h->f[0], an_mul(AQ(80.0), an_pow(AQ(200.0), P[3])), AN_QDB05);
    if (P[2] > AQ(0.02)) {
        an_svf_set(&h->f[1], AQ(1500.0) + an_mul(P[3], AQ(3000.0)), an_kq(AQ(0.8)));
        an_env(&h->e[1], an_mul(P[2], AQ(1.5)), AQ(0.012), AQ(0.0005));
    }
    return d;
}
static int64_t an_kick(an_v_t *h, const int64_t *P, int64_t pf, int64_t df)
{
    return an_kick_body(h, P, an_mul(an_mul(AQ(20.0), an_pow(AQ(16.0), P[0])), pf), df);
}

static int64_t an_bass(an_v_t *h, const int64_t *P, int64_t pf, int64_t df)
{
    int64_t ratio = an_exp2((P[0] * 127 - AQ(64.0)) / 12);
    return an_kick_body(h, P, an_mul(an_mul(AQ(65.406391325), ratio), pf), df);
}
static int64_t an_snare(an_v_t *h, const int64_t *P, int64_t pf, int64_t df)
{
    int64_t tf = an_mul(an_mul(AQ(0.35), an_pow(AQ(8.0), P[0])), pf), bdur, nd;
    bdur = an_mul(AQ(0.05) + an_mul(P[5], AQ(0.2)), df);
    h->inc[0] = an_inc(an_mul(AQ(180.0), tf));
    h->inc[1] = an_inc(an_mul(AQ(330.0), tf));
    an_sweep(&h->sw, AQ(1.25), AQ(0.02));
    an_env(&h->e[0], AQ(0.3) + an_mul(AQ(0.7), P[4]), bdur, AQ(0.001));
    nd = an_mul(AQ(0.03) + an_mul(an_pow(P[3], AQ(2.5)), AQ(1.6)), df);
    an_svf_set(&h->f[0], AQ(300.0) + an_mul(an_mul(P[1], P[1]), AQ(12000.0)), AN_QDB07);
    an_env(&h->e[1], an_mul(P[2], AQ(1.6)), nd, AQ(0.001));
    return bdur > nd ? bdur : nd;
}
static int64_t an_tom(an_v_t *h, const int64_t *P, int64_t pf, int64_t df)
{
    int64_t base = an_mul(an_mul(AQ(40.0), an_pow(AQ(24.0), P[0])), pf), d;
    d = an_mul(AQ(0.04) + an_mul(an_mul(P[1], P[1]), AQ(3.0)), df);
    h->inc[0] = an_inc(base);
    an_sweep(&h->sw, AQ(1.0) + an_mul(P[2], AQ(6.0)), AQ(0.05));
    an_env(&h->e[0], AQ(1.0), d, AQ(0.001));
    an_svf_set(&h->f[0], an_mul(AQ(300.0), an_pow(AQ(30.0), P[4])), AN_QDB05);
    if (P[3] > AQ(0.02)) {
        an_svf_set(&h->f[1], AQ(1800.0), an_kq(AQ(1.0)));
        an_env(&h->e[1], an_mul(P[3], AQ(1.2)), AQ(0.01), AQ(0.0005));
    }
    if (P[5] > AQ(0.02)) {
        an_svf_set(&h->f[2], an_mul(base, AQ(2.2)), an_kq(AQ(1.2)));
        an_env(&h->e[2], an_mul(P[5], AQ(0.5)), an_mul(AQ(0.06), df), AQ(0.001));
    }
    return d;
}
static int64_t an_rim(an_v_t *h, const int64_t *P, int64_t pf, int64_t df)
{
    int64_t tf = an_mul(an_mul(AQ(0.33), an_pow(AQ(10.0), P[0])), pf), d;
    d = an_mul(AQ(0.008) + an_mul(an_mul(P[1], P[1]), AQ(0.6)), df);
    h->inc[0] = an_inc(an_mul(AQ(455.0), tf));
    h->inc[1] = an_inc(an_mul(an_mul(AQ(1667.0), tf), AQ(0.7) + an_mul(P[4], AQ(0.7))));
    h->g0 = (int32_t)((AQ(1.0) - an_mul(P[2], AQ(0.8))) >> 1);
    h->g1 = (int32_t)((AQ(0.3) + an_mul(P[2], AQ(0.7))) >> 1);
    an_svf_set(&h->f[0], an_mul(AQ(1700.0), tf), an_kq(AQ(1.5) + an_mul(P[5], AQ(12.0))));
    an_env(&h->e[0], AQ(0.9), d, AQ(0.0005));
    if (P[3] > AQ(0.02)) {
        an_svf_set(&h->f[1], AQ(2500.0), AN_QDB07);
        an_env(&h->e[1], P[3], AQ(0.004), AQ(0.0003));
    }
    return d;
}
static int64_t an_clap(an_v_t *h, const int64_t *P, int64_t pf, int64_t df)
{
    int64_t tf = an_mul(an_mul(AQ(0.35), an_pow(AQ(8.0), P[0])), pf), sp, tail;
    uint32_t nb = 2u + (uint32_t)((P[4] * 4 + 32768) >> 16);
    sp = AQ(0.003) + an_mul(an_mul(P[2], P[2]), AQ(0.08));
    tail = an_mul(AQ(0.03) + an_mul(an_pow(P[1], AQ(2.4)), AQ(1.6)), df);
    an_svf_set(&h->f[0], an_mul(AQ(1100.0), tf), an_kq(AQ(0.5) + an_mul(an_mul(P[3], P[3]), AQ(20.0))));
    h->nb = (uint8_t)nb;
    h->b = 0;
    h->bn = an_samples(sp);
    h->bc = 0;
    h->bd = an_dec(an_log2(AQ(20.0)), an_samples(an_mul(sp, AQ(0.8))));
    h->btail = an_dec(870824, an_samples(tail));
    h->e[0].v = 1u << 28;
    if (P[5] > AQ(0.02)) {
        an_svf_set(&h->f[1], AQ(1800.0), AN_QDB05);
        an_env(&h->e[1], an_mul(P[5], AQ(0.35)), an_mul(AQ(0.2) + an_mul(P[5], AQ(0.6)), df), AQ(0.01));
        h->e[1].pre = (nb - 1u) * h->bn;
    }
    return tail + sp * nb;
}
static int64_t an_cow(an_v_t *h, const int64_t *P, int64_t pf, int64_t df)
{
    int64_t tf = an_mul(an_mul(AQ(0.35), an_pow(AQ(8.0), P[0])), pf), d;
    d = an_mul(AQ(0.03) + an_mul(an_pow(P[1], AQ(2.2)), AQ(2.2)), df);
    h->inc[0] = an_inc(an_mul(AQ(540.0), tf));
    h->inc[1] = an_inc(an_mul(an_mul(AQ(540.0), tf), AQ(1.3) + an_mul(P[4], AQ(0.5))));
    an_svf_set(&h->f[0], an_mul(an_mul(AQ(800.0), tf), an_pow(AQ(6.0), P[2] - AQ(0.5))), an_kq(AQ(1.0) + an_mul(P[5], AQ(8.0))));
    an_env(&h->e[0], AQ(1.0), AQ(0.011), AQ(0.001));
    h->e[0].d = an_dec(an_log2(AQ(1.0)) - an_log2(AQ(0.3)), an_samples(AQ(0.011)));
    h->e[0].n1 = an_samples(AQ(0.011));
    h->e[0].d2 = an_dec(an_log2(AQ(0.3)) + 870824, an_samples(d));
    if (P[3] > AQ(0.02)) {
        an_svf_set(&h->f[1], AQ(3000.0), AN_QDB07);
        an_env(&h->e[1], P[3], AQ(0.004), AQ(0.0003));
    }
    return d + AQ(0.012);
}
static const int32_t AN_HR[6] = {65536, 97190, 117965, 166855, 172360, 255394};
static void an_six(an_v_t *h, int64_t tf, int64_t metal)
{
    int64_t mt = AQ(0.1) + an_mul(metal, AQ(2.2));
    uint32_t i;
    for (i = 0; i < 6u; i++)
        h->inc[i] = an_inc(an_mul(an_mul(AQ(205.3), tf), AQ(1.0) + an_mul(AN_HR[i] - AQ(1.0), mt)));
}
static int64_t an_hat(an_v_t *h, const int64_t *P, int64_t pf, int64_t df)
{
    int64_t tf = an_mul(an_mul(AQ(0.316), an_pow(AQ(10.0), P[0])), pf), d;
    d = an_mul(AQ(0.02) + an_mul(an_mul(an_mul(P[1], P[1]), P[1]), AQ(4.0)), df);
    an_six(h, tf, P[3]);
    an_svf_set(&h->f[0], AQ(2000.0) + an_mul(P[2], AQ(16000.0)), an_kq(AQ(0.9)));
    an_svf_set(&h->f[1], AQ(4000.0) + an_mul(P[4], AQ(5000.0)), AN_QDB07);
    an_env(&h->e[0], AQ(1.0), d, AQ(0.0005));
    if (P[5] > AQ(0.02)) {
        an_svf_set(&h->f[2], AQ(7000.0), AN_QDB07);
        an_env(&h->e[1], an_mul(P[5], AQ(0.35)), AQ(0.006), AQ(0.0003));
    }
    return d;
}
static int64_t an_cym(an_v_t *h, const int64_t *P, int64_t pf, int64_t df)
{
    int64_t tf = an_mul(an_mul(AQ(0.316), an_pow(AQ(10.0), P[0])), pf), d;
    d = an_mul(AQ(0.15) + an_mul(an_mul(P[1], P[1]), AQ(5.0)), df);
    an_six(h, tf, P[3]);
    an_svf_set(&h->f[0], AQ(2000.0) + an_mul(P[2], AQ(14000.0)), an_kq(AQ(0.6)));
    an_svf_set(&h->f[1], AQ(3500.0), AN_QDB07);
    an_svf_set(&h->f[2], AQ(3000.0) + an_mul(P[4], AQ(15000.0)), AN_QDB06);
    an_env(&h->e[0], AQ(0.9), d, AQ(0.002));
    if (P[5] > AQ(0.02)) {
        an_svf_set(&h->f[3], AQ(2500.0), AN_QDB06);
        an_env(&h->e[1], an_mul(P[5], AQ(0.8)), an_mul(d, AQ(1.2)), AQ(0.003));
    }
    return d;
}
static int64_t (*const AN_VOICE[AN_N])(an_v_t *, const int64_t *, int64_t, int64_t) = {
    an_kick, an_snare, an_tom, an_rim, an_clap, an_cow, an_hat, an_cym, an_bass};

static void analog_note_on(track_t *t, voice_t *v)
{
    uint32_t tr = (uint32_t)(t - trk) % NTRK, vi = (uint32_t)(v - t->v) + 1u, i;
    an_v_t *h = an_of(t, v);
    int32_t *rs = &an_t[tr].rng, dc = 0;
    int64_t P[8], pf;
    if (h) {
        dc = h->last + ((h->dc * h->dcn) >> 6);
    } else {
        an_v_t *o = &an_v[tr][0];
        h = o->owner && t->v[(o->owner - 1u) % NVOICE].active ? &an_v[tr][1] : o;
    }
    memset(h, 0, sizeof *h);
    h->owner = (uint8_t)vi;
    h->dc = dc;
    h->dcn = dc ? (int32_t)AN_DECLICK : 0;
    h->mach = (uint8_t)((uint32_t)t->p[P_E7] % AN_N);
    h->gk = -1;
    if (!*rs)
        *rs = (int32_t)(0x9E3779B9u * (tr + 1u));
    h->seed = (int32_t)(0x2545F491u ^ (vi * 0x9E3779B9u) ^ (uint32_t)noise32(rs));
    h->cp = 65536u;
    for (i = 0; i < 7u; i++)
        P[i] = an_x(t->p[P_E0 + i]);
    P[7] = an_x(t->p[P_LN0]);
    h->acc = v->vel >= 127u ? (int32_t)an_x(t->p[P_ED_FX]) : 0;
    if (h->acc && AN_ACC_P[h->mach] >= 0)
        P[AN_ACC_P[h->mach]] += (P[AN_ACC_P[h->mach]] * (h->acc * 39321LL >> 16)) >> 16;
    h->pitch16 = v->pitch16;
    pf = an_exp2(((v->pitch16 - AN_NOTE * 16) << 16) / 192);
    AN_VOICE[h->mach](h, P, pf, AQ(1.0));
    v->s[0] = (int32_t)vi;
}

static int analog_done(track_t *t, voice_t *v)
{
    const an_v_t *h = an_of(t, v);
    return !h || (!h->e[0].v && !h->e[1].v && !h->e[2].v && !h->dcn);
}

static inline uint32_t an_clap_env(an_v_t *h)
{
    uint32_t v = h->e[0].v;
    if (!v)
        return 0;
    if (h->b + 1u < h->nb) {
        if (h->bc * 5u < h->bn * 4u)
            h->e[0].v = v - (uint32_t)(((uint64_t)v * h->bd) >> 32);
        if (++h->bc >= h->bn) {
            h->bc = 0;
            h->b++;
            h->e[0].v = 1u << 28;
        }
    } else {
        h->e[0].v = v - (uint32_t)(((uint64_t)v * h->btail) >> 32);
        if (h->e[0].v < AN_FLOOR)
            h->e[0].v = 0;
    }
    return v;
}

static inline int32_t an_g(int32_t s, uint32_t e) { return (int32_t)(((int64_t)s * (int32_t)(e >> 13)) >> 15); }
static inline int32_t an_sq(uint32_t ph) { return ((int32_t)ph >> 31) | 1; }

static void analog_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    an_v_t *hp = an_of(t, v), hv, *h = &hv;
    const int16_t *p = t->p;
    uint32_t i, k, inc[6], q, qi, ci, cr;
    int32_t gain, kk, ninv, velg = v->vel * 258, trim, subg;
    int64_t g, dr, pr;
    if (!hp)
        return;
    hv = *hp;

    subg = (h->mach == AN_KICK || h->mach == AN_BASS) ? (int32_t)(((int64_t)an_x(p[P_DIST]) * 26214) >> 16) : 0;
    trim = (VOICE_FS * AN_TRIM[h->mach % AN_N]) >> 12;

    g = an_x(p[P_LN1]);
    dr = an_x(p[P_E6]);
    if (h->acc) {
        dr += ((int64_t)h->acc * 22938) >> 16;
        dr = dr > AQ(1.0) ? AQ(1.0) : dr;
        velg = (int32_t)((96 * 258LL * (65536 + (((int64_t)h->acc * 64881) >> 16))) >> 16);
    }
    kk = (int32_t)((AQ(1.0) + dr * 3) >> 4);
    ninv = (int32_t)((32767u << 14) / (uint32_t)an_tanh(kk << 3));
    gain = (int32_t)((an_x(p[P_LN0]) * AQ(1.1)) >> 17);
    cr = g > 0;
    if (cr && h->gk != (int32_t)g) {
        uint32_t st = ((uint32_t)an_exp2(g * CRUSH_BITS) / AN_TRIM[h->mach % AN_N]) << 12;
        h->gk = (int32_t)g;
        h->gq = st < 65536u ? 65536u : st;
        h->gqi = 0xFFFFFFFFu / h->gq;
        h->gi = (uint32_t)an_exp2(-((int32_t)g * CRUSH_RATE) / 10);
    }
    q = h->gq;
    qi = h->gqi;
    ci = h->gi;
    pr = an_exp2(((m->pitch16 - h->pitch16) << 16) / 192);
    for (k = 0; k < 6u; k++)
        inc[k] = (uint32_t)(((uint64_t)h->inc[k] * (uint64_t)pr) >> 16);
    for (i = 0; i < n; i++) {
        int32_t s = 0, x, y, sb = 0;
        {
            switch (h->mach) {
            case AN_BASS:
            case AN_KICK: {
                uint32_t f = an_sweep_tick(&h->sw);
                sb = an_g(sine_i(h->ph[0]), an_env_tick(&h->e[0]));
                s = an_lp(&h->f[0], sb);
                h->ph[0] += (uint32_t)(((uint64_t)inc[0] * f) >> 28);
                if (h->e[1].v)
                    s += an_g(an_bp(&h->f[1], an_noise(&h->seed)), an_env_tick(&h->e[1]));
                break;
            }
            case AN_SNARE: {
                uint32_t f = an_sweep_tick(&h->sw);
                s = an_g(sine_i(h->ph[0]) + ((sine_i(h->ph[1]) * 19661) >> 15), an_env_tick(&h->e[0]));
                h->ph[0] += (uint32_t)(((uint64_t)inc[0] * f) >> 28);
                h->ph[1] += (uint32_t)(((uint64_t)inc[1] * f) >> 28);
                if (h->e[1].v)
                    s += an_g(an_hp(&h->f[0], an_noise(&h->seed)), an_env_tick(&h->e[1]));
                break;
            }
            case AN_TOM: {
                uint32_t f = an_sweep_tick(&h->sw);
                s = an_lp(&h->f[0], an_g(sine_i(h->ph[0]), an_env_tick(&h->e[0])));
                h->ph[0] += (uint32_t)(((uint64_t)inc[0] * f) >> 28);
                if (h->e[1].v | h->e[2].v) {
                    int32_t nz = an_noise(&h->seed);
                    if (h->e[1].v)
                        s += an_g(an_bp(&h->f[1], nz), an_env_tick(&h->e[1]));
                    if (h->e[2].v)
                        s += an_g(an_bp(&h->f[2], nz), an_env_tick(&h->e[2]));
                }
                break;
            }
            case AN_RIM:
                s = an_g(an_bp(&h->f[0], an_sq(h->ph[0]) * h->g0 + an_sq(h->ph[1]) * h->g1), an_env_tick(&h->e[0]));
                h->ph[0] += inc[0];
                h->ph[1] += inc[1];
                if (h->e[1].v)
                    s += an_g(an_hp(&h->f[1], an_noise(&h->seed)), an_env_tick(&h->e[1]));
                break;
            case AN_CLAP: {
                int32_t nz = an_noise(&h->seed);
                s = an_g(an_bp(&h->f[0], nz), an_clap_env(h));
                if (h->e[1].v)
                    s += an_g(an_lp(&h->f[1], nz), an_env_tick(&h->e[1]));
                }
                break;
            case AN_COW:
                s = an_g(an_bp(&h->f[0], (an_sq(h->ph[0]) + an_sq(h->ph[1])) * 19660), an_env_tick(&h->e[0]));
                h->ph[0] += inc[0];
                h->ph[1] += inc[1];
                if (h->e[1].v)
                    s += an_g(an_hp(&h->f[1], an_noise(&h->seed)), an_env_tick(&h->e[1]));
                break;
            default: {
                int32_t six = (an_sq(h->ph[0]) + an_sq(h->ph[1]) + an_sq(h->ph[2]) + an_sq(h->ph[3]) + an_sq(h->ph[4]) +
                               an_sq(h->ph[5])) * 3277;
                for (k = 0; k < 6u; k++)
                    h->ph[k] += inc[k];
                if (h->mach == AN_HAT) {
                    s = an_g(an_hp(&h->f[1], an_bp(&h->f[0], six)), an_env_tick(&h->e[0]));
                    if (h->e[1].v)
                        s += an_g(an_hp(&h->f[2], an_noise(&h->seed)), an_env_tick(&h->e[1]));
                } else {
                    s = an_g(an_lp(&h->f[2], an_hp(&h->f[1], an_bp(&h->f[0], six))), an_env_tick(&h->e[0]));
                    if (h->e[1].v)
                        s += an_g(an_lp(&h->f[3], six), an_env_tick(&h->e[1]));
                }
                break;
            }
            }
        }

        x = clamp((int32_t)(((int64_t)s * gain) >> 15), -32767, 32767);
        y = (int32_t)(((int64_t)an_tanh((x * kk) >> 12) * ninv) >> 14);
        if (cr) {
            h->cp += ci;
            if (h->cp >= 65536u) {
                h->cp -= 65536u;
                h->ch = (int32_t)(((((int64_t)y * qi + 32768) >> 16) * q) >> 16);
            }
            y = h->ch;
        } else {
            h->cp = 65536u;
        }
        if (subg)
            y += (int32_t)(((((int64_t)sb * gain) >> 15) * subg) >> 15);
        y = (int32_t)(((int64_t)y * velg) >> 15);
        h->last = y;
        if (h->dcn) {
            y += (h->dc * h->dcn) >> 6;
            h->dcn--;
        }
        out[i] += (int32_t)(((int64_t)mulq15(y, amp_at(m, i)) * trim) >> 15);
    }
    *hp = hv;
}

static const param_desc_t *analog_desc(const track_t *t, uint32_t k)
{
    static param_desc_t d[7];
    uint32_t mach = (uint32_t)t->p[P_E7] % AN_N;
    if (k >= 7u)
        return 0;
    d[k] = (param_desc_t){AN_LABEL[mach][k], F_INT, 0, 127, AN_DEF[mach][k], 0, 0};
    return &d[k];
}

static const engine_t ENG_ANALOG = {
    .name = "ANALOG",
    .page_title = {"OSC 1", "OSC 2"},
    .edit = {{"TUNE", F_INT, 0, 127, 55, 0, 0}, {"DECAY", F_INT, 0, 127, 70, 0, 0}, {"CLICK", F_INT, 0, 127, 45, 0, 0},
             {"TONE", F_INT, 0, 127, 80, 0, 0}, {"SWEEP", F_INT, 0, 127, 50, 0, 0}, {"S.TIM", F_INT, 0, 127, 40, 0, 0},
             {"DRIVE", F_INT, 0, 127, 15, 0, 0}, {"MACH", F_ENUM, 0, AN_N - 1, 0, N_AMACH, 0}},
    .presets = ANALOG_PRESETS,
    .npresets = AN_N,
    .note_on = analog_note_on,
    .render = analog_render,
    .knob = {P_E0, P_E1, P_E2, P_E3},
    .desc = analog_desc,
    .poly = 2,
    .keep = 0,
    .oneshot = 1,
    .ownenv = 1,
    .done = analog_done,
};

/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

enum { MS_OFF, MS_LFO, MS_ENV, MS_VEL, MS_KEY, MS_RAND, MS_MODW, MS_AT, MS_EXPR, MS_SH, MS_SLEW, MS_FENV, MS_N };
enum { MD_OFF, MD_PITCH, MD_CUT, MD_SHP, MD_AMP, MD_PAN, MD_DIST, MD_CHO, MD_DLY, MD_REV, MD_RATE, MD_VIB, MD_E1,
       MD_DEPTH = MD_E1 + 8, MD_VEL, MD_TIME, MD_X0, MD_N = MD_X0 + 45 };

static const uint8_t MDX[MD_N - MD_X0] = {P_ATK, P_DEC, P_SUS, P_REL, P_ED_FLT, P_ED_PIT, P_ED_SHP, P_ED_FX, P_LD_FLT,
    P_LD_SHP, P_LD_AMP, 0x80 | G_DTIME, 0x80 | G_DFDBK, 0x80 | G_DCOLOR, 0x80 | G_RSIZE, 0x80 | G_RDAMP, 0x80 | G_RTYPE,
    P_FLT_RES, P_FE_AMT, P_FE_ATK, P_FE_DEC, P_FE_SUS, P_FE_REL, 0x80 | G_SYNC, 0x80 | G_CRATE, 0x80 | G_CDEPTH,
    0x80 | G_INFO, 0x80 | G_MIDI, P_LN0, P_LN1, P_LN2,
    P_FTYPE, P_LWAVE, P_LPHASE, P_LFADE, P_LSYNC, P_LTRIG, P_LPOL, P_LEVEL,
    P_FM3_ATK, P_FM3_DEC, P_FM4_ATK, P_FM4_DEC, P_FM3_LEVEL, P_FM4_LEVEL};
#define MD_GLOBAL(d) ((d) >= MD_X0 && (d) < MD_N && (MDX[(d) - MD_X0] & 0x80u))
_Static_assert(NELEM(N_MSRC) == MS_N && NELEM(N_MDST) == MD_N, "N_MSRC / N_MDST == MS_* / MD_*");
_Static_assert(P_M2SRC == P_M1SRC + 3 && P_M4AMT == P_M1SRC + 11 && P_M4AMT + 1 == P_FM1_ATK, "matrix slots: 3 ids each");
#define NMSLOT 4u
#define MS_VOICE(s) ((s) >= MS_ENV && (s) <= MS_RAND)
#define MS_BIPOLAR(s) ((s) == MS_LFO || (s) == MS_KEY || (s) == MS_RAND || (s) == MS_SH || (s) == MS_SLEW)
#define MD_ENGINE(d) ((d) >= MD_E1 && (d) < MD_E1 + 8)

static uint32_t mod_param(uint32_t d)
{
    static const uint8_t ID[MD_E1] = {[MD_PAN] = P_PAN, [MD_DIST] = P_DIST, [MD_CHO] = P_CHOR, [MD_DLY] = P_DLY,
                                      [MD_REV] = P_REV, [MD_RATE] = P_LRATE, [MD_VIB] = P_LD_PIT};
    return d >= MD_X0 ? MDX[(d - MD_X0) % (MD_N - MD_X0)] : MD_ENGINE(d) ? P_E0 + d - MD_E1 : ID[d];
}

typedef struct {
    uint8_t on;
    uint8_t amp;
    uint8_t nv, nk;
    int32_t pit, cut, shp, gain;
    int32_t ldep;
    uint8_t vsrc[NMSLOT], vdst[NMSLOT];
    int8_t vamt[NMSLOT];
    uint8_t kid[NMSLOT];
    int16_t keep[NMSLOT];
} modw_t;
#if FELUCCA_CPU2
static modw_t mod_w[NTRK];
#define MOD(t) (mod_w[C1_PI(t)])
#else
static modw_t mod;
#define MOD(t) mod
#endif
#define MOD_W_CLEAR() memset(&MOD(&trk[0]), 0, C1_NP * sizeof(modw_t))

static uint32_t mod_seed = 0x2545F491u;
static int16_t mod_rand(void)
{
    mod_seed ^= mod_seed << 13;
    mod_seed ^= mod_seed >> 17;
    mod_seed ^= mod_seed << 5;
    return (int16_t)(mod_seed >> 16);
}

static int32_t mod_key(uint32_t note) { return clamp(((int32_t)note - 60) * 512, -32767, 32767); }

static int32_t mod_sh(const track_t *t) { return t->lfo_rnd ? (int32_t)(t->lfo_rnd >> 16) - 32768 : 0; }

static int32_t mod_tsrc(const track_t *t, uint32_t s, int32_t lfo)
{
    switch (s) {
    case MS_SH:
        return mod_sh(t);
    case MS_SLEW:
        return t->lfo_prev + (((mod_sh(t) - t->lfo_prev) * (int32_t)(t->lfo_ph >> 17)) >> 15);
    case MS_LFO:
        return lfo;
    case MS_ENV:
        return t->m_env;
    case MS_FENV:
        return t->engine == ENGI_ANALOG ? 0 : t->m_fenv;
    case MS_VEL:
        return t->m_vel * 258;
    case MS_KEY:
        return mod_key(t->m_key);
    case MS_RAND:
        return t->m_rnd;
    case MS_MODW:
        return t->mw * 258;
    case MS_AT:
        return t->at * 258;
    default:
        return (127 - t->ex_off) * 258;
    }
}

static int32_t mod_gain(uint32_t s, int32_t x, int32_t a)
{
    int32_t u = MS_BIPOLAR(s) ? (x + 32768) >> 1 : x;
    return clamp(32767 - (a > 0 ? ((32767 - u) * a) >> 6 : (u * -a) >> 6), 0, 32767);
}

static void mod_voice_add(uint32_t s, uint32_t d, int32_t x, int32_t a, int32_t *pit, int32_t *cut, int32_t *shp,
                          int32_t *gain)
{
    if (d == MD_PITCH)
        *pit += (x * a * 3) >> 15;
    else if (d == MD_CUT)
        *cut += (x * a) >> 7;
    else if (d == MD_SHP)
        *shp += (x * a) >> 7;
    else
        *gain = mulq15(*gain, mod_gain(s, x, a));
}

static __attribute__((noinline)) int32_t mod_depth(const track_t *t, int32_t lfo)
{
    const int16_t *p = t->p;
    int32_t g = 32767;
    uint32_t k;
    for (k = 0; k < NMSLOT; k++) {
        uint32_t s = (uint32_t)p[P_M1SRC + 3u * k];
        if (p[P_M1DST + 3u * k] == MD_DEPTH && p[P_M1AMT + 3u * k] && s && s < MS_N)
            g = mulq15(g, mod_gain(s == MS_LFO && p[P_LPOL] ? MS_ENV : s, mod_tsrc(t, s, lfo), p[P_M1AMT + 3u * k]));
    }
    return g;
}

static __attribute__((noinline)) int mod_begin(track_t *t)
{
    const int16_t *p = t->p;
    int32_t lfo, off[NMSLOT];
    uint32_t k, j;
    modw_t *mw = &MOD(t);
    mw->on = 0;
    for (k = 0; k < NMSLOT; k++)
        if (p[P_M1SRC + 3u * k] && p[P_M1DST + 3u * k] && p[P_M1AMT + 3u * k] &&
            (p[P_M1DST + 3u * k] < MD_VEL || (p[P_M1DST + 3u * k] >= MD_X0 && !MD_GLOBAL(p[P_M1DST + 3u * k]))))
            break;
    if (k == NMSLOT)
        return 0;
    lfo = mulq15(t->lfo_val, t->lfo_fade);
    mw->on = 1;
    mw->amp = 0;
    mw->nv = mw->nk = 0;
    mw->pit = mw->cut = mw->shp = 0;
    mw->gain = 32767;
    mw->ldep = mod_depth(t, lfo);
    if (mw->ldep < 32767)
        lfo = mulq15(lfo, mw->ldep);
    for (; k < NMSLOT; k++) {
        uint32_t s = (uint32_t)p[P_M1SRC + 3u * k], d = (uint32_t)p[P_M1DST + 3u * k];
        int32_t a = p[P_M1AMT + 3u * k], x;
        if (!s || !d || !a || s >= MS_N || (d >= MD_DEPTH && d < MD_X0) || d >= MD_N || MD_GLOBAL(d))
            continue;
        if (d <= MD_AMP) {
            mw->amp |= d == MD_AMP;
            if (MS_VOICE(s)) {
                mw->vsrc[mw->nv] = (uint8_t)s;
                mw->vdst[mw->nv] = (uint8_t)d;
                mw->vamt[mw->nv++] = (int8_t)a;
            } else {
                mod_voice_add(s == MS_LFO && p[P_LPOL] ? MS_ENV : s, d, mod_tsrc(t, s, lfo), a, &mw->pit, &mw->cut,
                              &mw->shp, &mw->gain);
            }
            continue;
        }
        {
            uint32_t id = mod_param(d);
            const param_desc_t *pd = id >= P_E0 ? &ENGINES[t->engine]->edit[id - P_E0] : &TP[id];
            x = mod_tsrc(t, s, lfo);
            for (j = 0; j < mw->nk && mw->kid[j] != id; j++)
                ;
            if (j == mw->nk) {
                mw->kid[j] = (uint8_t)id;
                mw->keep[j] = p[id];
                off[j] = 0;
                mw->nk++;
            }
            off[j] += (((x * a) >> 6) * (pd->max - pd->min)) >> 15;
        }
    }
    for (j = 0; j < mw->nk; j++) {
        uint32_t id = mw->kid[j];
        const param_desc_t *pd = id >= P_E0 ? &ENGINES[t->engine]->edit[id - P_E0] : &TP[id];
        t->p[id] = (int16_t)clamp(mw->keep[j] + off[j], pd->min, pd->max);
    }
    return 1;
}

#define MG_N 24u
static uint8_t mg_n, mg_id[MG_N];
static int16_t mg_keep[MG_N], mg_base[MG_N];
static uint8_t mg_ov_n, mg_ov_g[MG_N];
static int16_t mg_ov_v[MG_N];
static uint32_t mg_slot(uint32_t g, int32_t *off)
{
    uint32_t j;
    for (j = 0; j < mg_n && mg_id[j] != g; j++)
        ;
    if (j == mg_n) {
        if (mg_n == MG_N)
            return MG_N;
        mg_id[j] = (uint8_t)g;
        mg_keep[j] = mg_base[j] = song.g[g];
        off[j] = 0;
        mg_n++;
    }
    return j;
}

static const uint8_t FXL_ID[3][4] = {{G_F1RATE, G_F1WAVE, G_F1DEST, G_F1AMT}, {G_F2RATE, G_F2WAVE, G_F2DEST, G_F2AMT},
                                     {G_F3RATE, G_F3WAVE, G_F3DEST, G_F3AMT}};
static const uint8_t FXD_G[NELEM(N_FXDST)] = {0xFFu, G_AMP, G_MIDI, G_SYNC, G_CRATE, G_CDEPTH, G_INFO, G_DTIME, G_DFDBK,
                                              G_DCOLOR, G_RSIZE, G_RDAMP, G_RTYPE, 0xFFu, 0xFFu, 0xFFu, 0xFFu, G_PAN};
static struct { uint32_t ph, rnd; } fxlfo[3];
static uint32_t fxlfo_seed = 0x6C8E9CF5u;
static int32_t fxs_gain[4] = {32767, 32767, 32767, 32767};

static void fxlfo_reset(void) { fxlfo[0].ph = fxlfo[1].ph = fxlfo[2].ph = 0; }
static uint32_t div_samples(uint32_t div);
static int32_t fxlfo_block(uint32_t k, uint32_t n)
{
    uint32_t cyc = div_samples(LSYNC_DIV[1u + (uint32_t)song.g[FXL_ID[k][0]] % NELEM(N_FXRATE)]), q, r, old, ph;
    cyc = cyc ? cyc : 1u;
    q = 0xFFFFFFFFu / cyc;
    r = 0xFFFFFFFFu % cyc;
    old = fxlfo[k].ph;
    ph = fxlfo[k].ph = old + q * n + (r * n + n) / cyc;
    if (ph < old || !fxlfo[k].rnd) {
        fxlfo_seed ^= fxlfo_seed << 13;
        fxlfo_seed ^= fxlfo_seed >> 17;
        fxlfo_seed ^= fxlfo_seed << 5;
        fxlfo[k].rnd = fxlfo_seed | 1u;
    }
    switch (song.g[FXL_ID[k][1]]) {
    case 1:
        return (int32_t)((ph < 0x80000000u ? ph : ~ph) >> 16);
    case 2:
        return (int32_t)(ph >> 17);
    case 3:
        return 32767 - (int32_t)(ph >> 17);
    case 4:
        return ph < 0x80000000u ? 32767 : 0;
    case 5:
        return (int32_t)(fxlfo[k].rnd >> 17);
    default:
        return (32767 + osc_sine(ph - 0x40000000u)) >> 1;
    }
}

static __attribute__((noinline)) void mod_g_begin(uint32_t nb)
{
    int32_t off[MG_N];
    uint32_t i, k, j;
    mg_n = 0;
    for (j = 0; j < mg_ov_n && j < MG_N; j++) {
        mg_id[j] = mg_ov_g[j];
        mg_keep[j] = song.g[mg_ov_g[j]];
        mg_base[j] = mg_ov_v[j];
        off[j] = 0;
        mg_n++;
    }
    fxs_gain[0] = fxs_gain[1] = fxs_gain[2] = fxs_gain[3] = 32767;
    for (k = 0; k < 3u; k++) {
        int32_t v = fxlfo_block(k, nb), a;
        uint32_t d = (uint32_t)song.g[FXL_ID[k][2]], g;
        for (j = 0; j < mg_n && mg_id[j] != FXL_ID[k][3]; j++)
            ;
        a = j < mg_n ? mg_base[j] : song.g[FXL_ID[k][3]];
        if (!d || !a || d >= NELEM(FXD_G))
            continue;
        if (d >= FXD_DSND && d < FXD_DSND + 4u) {
            int32_t *gp = &fxs_gain[(d - FXD_DSND) & 3u];
            *gp = clamp(*gp + ((v * a) >> 6), 0, 32767);
            continue;
        }
        g = FXD_G[d];
        if ((j = mg_slot(g, off)) < MG_N)
            off[j] += (((v * a) >> 6) * (GP[g].max - GP[g].min)) >> 15;
    }
    for (i = 0; i < NTRK; i++) {
        const track_t *t = &trk[i];
        const int16_t *p = t->p;
        int32_t lfo = 0, have = 0;
        for (k = 0; k < NMSLOT; k++) {
            uint32_t s = (uint32_t)p[P_M1SRC + 3u * k], d = (uint32_t)p[P_M1DST + 3u * k], g;
            int32_t a = p[P_M1AMT + 3u * k];
            if (!s || !a || s >= MS_N || !MD_GLOBAL(d))
                continue;
            if (!have) {
                int32_t dg;
                lfo = mulq15(t->lfo_val, t->lfo_fade);
                dg = mod_depth(t, lfo);
                if (dg < 32767)
                    lfo = mulq15(lfo, dg);
                have = 1;
            }
            g = MDX[d - MD_X0] & 0x7Fu;
            if ((j = mg_slot(g, off)) < MG_N)
                off[j] += (((mod_tsrc(t, s, lfo) * a) >> 6) * (GP[g].max - GP[g].min)) >> 15;
        }
    }
    for (j = 0; j < mg_n; j++) {
        uint32_t g = mg_id[j];
        int32_t mx = g == G_RSIZE && mg_base[j] < GP[g].max && off[j] ? GP[g].max - 1 : GP[g].max;
        song.g[g] = (int16_t)clamp(mg_base[j] + off[j], GP[g].min, mx);
    }
}
static void mod_g_end(void)
{
    uint32_t j;
    for (j = 0; j < mg_n; j++)
        song.g[mg_id[j]] = mg_keep[j];
    mg_n = 0;
}

static __attribute__((noinline)) void mod_end(track_t *t)
{
    uint32_t j;
    modw_t *mw = &MOD(t);
    for (j = 0; j < mw->nk; j++)
        t->p[mw->kid[j]] = mw->keep[j];
    mw->on = 0;
}

static __attribute__((noinline)) void mod_voice(track_t *t, voice_t *v, vmod_t *m, int32_t fine)
{
    const modw_t *mw = &MOD(t);
    int32_t pit = mw->pit, cut = mw->cut, shp = mw->shp, gain = mw->gain, env = m->envq15;
    uint32_t k;
    if (v == &t->v[t->m_vi])
        t->m_env = env;
    for (k = 0; k < mw->nv; k++) {
        uint32_t s = mw->vsrc[k];
        int32_t x = s == MS_ENV ? env : s == MS_VEL ? v->mvel * 258 : s == MS_KEY ? mod_key(v->note) : v->mrnd;
        mod_voice_add(s, mw->vdst[k], x, mw->vamt[k], &pit, &cut, &shp, &gain);
    }
    if (pit) {
        m->pitch16 = clamp(m->pitch16 + pit, 0, 2047);
        m->inc = pitch_inc(m->pitch16);
        if (fine)
            m->inc += (uint32_t)((int32_t)(m->inc >> 12) * fine);
    }
    m->cutoff = clamp(m->cutoff + cut, -150 * 256, 150 * 256);
    m->shape = clamp(m->shape + shp, -62 * 256, 190 * 256);
    if (mw->amp) {
        m->amp1 = mulq15(m->amp1, gain);
        v->env_out = m->amp1;
    }
}

static __attribute__((noinline)) int mod_has(const track_t *t, uint32_t d)
{
    const int16_t *p = t->p;
    uint32_t k;
    for (k = 0; k < NMSLOT; k++)
        if (p[P_M1DST + 3u * k] == (int16_t)d && p[P_M1AMT + 3u * k] && p[P_M1SRC + 3u * k] > 0 &&
            p[P_M1SRC + 3u * k] < MS_N)
            return 1;
    return 0;
}

static __attribute__((noinline)) int32_t mod_at_note(const track_t *t, uint32_t d, uint32_t note, uint32_t vel,
                                                     int32_t rnd)
{
    const int16_t *p = t->p;
    int32_t sum = 0, lfo = mulq15(t->lfo_val, t->lfo_fade), g = mod_depth(t, lfo);
    uint32_t k;
    if (g < 32767)
        lfo = mulq15(lfo, g);
    for (k = 0; k < NMSLOT; k++) {
        uint32_t s = (uint32_t)p[P_M1SRC + 3u * k];
        int32_t a = p[P_M1AMT + 3u * k], x;
        if (p[P_M1DST + 3u * k] != (int16_t)d || !a || !s || s >= MS_N)
            continue;
        x = s == MS_VEL ? (int32_t)vel * 258 : s == MS_KEY ? mod_key(note) : s == MS_RAND ? rnd : mod_tsrc(t, s, lfo);
        if (d == MD_VEL) {
            sum += (((x * a) >> 6) * 127) >> 15;
        } else {
            int32_t u = MS_BIPOLAR(s) && !(s == MS_LFO && p[P_LPOL]) ? (x + 32768) >> 1 : x;
            u = clamp(u, 0, 32767);
            sum += a > 0 ? (u * a) >> 6 : ((32767 - u) * -a) >> 6;
        }
    }
    return sum;
}

static uint32_t mod_note(track_t *t, uint32_t note, uint32_t vel)
{
    int16_t r = mod_rand();
    if (mod_has(t, MD_VEL))
        vel = (uint32_t)clamp((int32_t)vel + mod_at_note(t, MD_VEL, note, vel, r), 1, 127);
    t->m_vel = (uint8_t)vel;
    t->m_key = (uint8_t)note;
    t->m_rnd = r;
    return vel;
}

#define MOD_TIME_MAX ((uint32_t)FS * 30u / 1000u)
static __attribute__((noinline)) uint32_t mod_time(track_t *t, uint32_t note, uint32_t vel, uint32_t period)
{
    uint32_t mx = period * 7u / NUDGE_DIV, d;
    int32_t q;
    if (!mod_has(t, MD_TIME))
        return 0;
    q = clamp(mod_at_note(t, MD_TIME, note, vel, mod_rand()), 0, 32767);
    d = ((uint32_t)q * MOD_TIME_MAX) >> 15;
    return d < mx ? d : mx;
}

static __attribute__((noinline)) void mod_midi(track_t *t, uint32_t st, uint32_t d1, uint32_t d2)
{
    if (st == 0xD0u) {
        t->at = (uint8_t)d1;
    } else if (st == 0xB0u) {
        if (d1 == 1u)
            t->mw = (uint8_t)d2;
        else if (d1 == 11u)
            t->ex_off = (uint8_t)(127u - d2);
        else if (d1 == 121u)
            t->mw = t->at = t->ex_off = 0;
    }
}


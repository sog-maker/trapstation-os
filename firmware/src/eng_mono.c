/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Blair (blairysbeatlab), BBL JAM BOX */

#define MONO_NW 10u
static const char *const N_MWAVE[MONO_NW] = {"SW+SQ", "SW+TR", "SW+SN", "SQ+TR", "SQ+SN", "TR+SN",
                                             "SW+SW", "SQ+SQ", "TR+TR", "SN+SN"};
enum { MW_SAW, MW_SQR, MW_TRI, MW_SIN };
static const uint8_t MONO_W[MONO_NW][2] = {{MW_SAW, MW_SQR}, {MW_SAW, MW_TRI}, {MW_SAW, MW_SIN}, {MW_SQR, MW_TRI},
                                          {MW_SQR, MW_SIN}, {MW_TRI, MW_SIN}, {MW_SAW, MW_SAW}, {MW_SQR, MW_SQR},
                                          {MW_TRI, MW_TRI}, {MW_SIN, MW_SIN}};
static const char *const N_MSUB[3] = {"-1", "-2", "-3"};
static const char *const N_MMACH[1] = {"MONO"};
static const param_desc_t MONO_SAT = {"SAT", F_INT, 0, 127, 0, 0, 0};
static const param_desc_t MONO_TYPE = {"TYPE", F_INT, 0, 127, 0, 0, 0};

static uint32_t mono_ratio(int32_t d)
{
    uint32_t base = pitch_inc(60u * 16u), a, b, x;
    int32_t p = 60 * 16 * 4 + d;
    a = pitch_inc((uint32_t)(p >> 2));
    b = pitch_inc((uint32_t)(p >> 2) + 1u);
    x = a + (uint32_t)(((uint64_t)(b - a) * (uint32_t)(p & 3)) >> 2);
    return (uint32_t)(((uint64_t)x << 16) / base);
}

static inline int32_t mono_osc(uint32_t w, uint32_t *ph, uint32_t inc, uint32_t pw, int32_t *sp, int32_t *jp)
{
    uint32_t p = *ph, y;
    int32_t o;
    switch (w) {
    case MW_SAW:
        if (!pw) {
            o = osc_saw(p, inc);
            break;
        }
        {
            uint32_t si = (uint32_t)(((uint64_t)inc * (65536u + pw * 7u * 65536u / 127u)) >> 16), s = (uint32_t)*sp;
            if (p > 0xFFFFFFFFu - inc) {
                uint32_t at = s + (uint32_t)(((uint64_t)si * (0xFFFFFFFFu - p)) / inc);
                *jp = -(int32_t)(at >> 16);
            }
            o = ((int32_t)(s >> 16) - 32768) - blep(s, si);
            if (p < inc || p > 0xFFFFFFFFu - inc)
                o += (blep(p, inc) * *jp) >> 16;
            s += si;
            if (p + inc < p)
                s = (uint32_t)(((uint64_t)si * (p + inc)) / inc);
            *sp = (int32_t)s;
        }
        break;
    case MW_SQR:
        o = osc_pulse(p, inc, 0x80000000u - pw * (0x73333333u / 127u));
        break;
    case MW_TRI:
        o = osc_tri(p);
        if (pw) {
            o = (o * (int32_t)(127u + pw * 3u)) / 127;
            while (o > 32767 || o < -32768)
                o = o > 32767 ? 65534 - o : -65536 - o;
        }
        break;
    default:
        o = osc_sine(p);
        if (pw) {
            uint32_t sh = 1u + pw * 14u / 127u, q = 1u << sh;
            y = (uint32_t)(o + 32768);
            o = (int32_t)((y >> sh << sh) + (q >> 1)) - 32768;
            o = clamp(o, -32767, 32767);
        }
        break;
    }
    *ph = p + inc;
    return o;
}

static void mono_note_on(track_t *t, voice_t *v)
{
    const int16_t *p = t->p;
    v->s[2] = !p[P_FE_ATK] ? 1 << 24 : v->env ? v->s[2] : 0;
    v->s[3] = p[P_FE_ATK] ? 1 : 2;
}

static int32_t mono_cs[NTRK];

static void mono_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    uint32_t i, wv = (uint32_t)p[P_E0] % MONO_NW, w1 = MONO_W[wv][0], w2 = MONO_W[wv][1];
    uint32_t mix = (uint32_t)p[P_E1] & 127u, pw = (uint32_t)p[P_E6] & 127u, sub = (uint32_t)p[P_E4] % 3u;
    int32_t sublv = p[P_E5] & 127, sat = p[P_LN0] & 127;
    uint32_t inc1 = m->inc, inc2, incs;
    int32_t drv = 32768 + sat * sat * 4;
    inc2 = (uint32_t)(((uint64_t)inc1 * mono_ratio(p[P_E3] * 64 + p[P_E2])) >> 16);
    incs = inc1 >> (sub + 1u);

    int32_t *cs = &mono_cs[(uint32_t)(t - trk) % NTRK], ct = (p[P_FLT_CUT] << 8) + m->cutoff;
    int32_t cut = p[P_FLT_KTR] * (v->pitch16 - 60 * 16) >> 4;
    {
        int32_t fe = v->s[2], fs = v->s[3], sus = (int32_t)p[P_FE_SUS] * (1 << 17);
        if (!v->gate)
            fs = 4;
        if (fs == 1) {
            fe += (int32_t)ENV_LIN[p[P_FE_ATK] & 127];
            if (fe >= (1 << 24)) { fe = 1 << 24; fs = 2; }
        } else if (fs == 2) {
            fe = p[P_FE_DEC] ? fe + mulq16(sus - fe, ENV_EXP[p[P_FE_DEC] & 127]) : sus;
        } else if (fs == 4) {
            fe -= mulq16(fe, ENV_EXP[p[P_FE_REL] & 127]);
        }
        fe = clamp(fe, 0, 1 << 24);
        v->s[2] = fe;
        v->s[3] = fs;
        cut += ((fe >> 9) * p[P_FE_AMT]) >> 6;
        if (v == &t->v[t->m_vi])
            t->m_fenv = fe >= (1 << 24) ? 32767 : fe >> 9;
    }
    int32_t drive = 32768 + p[P_FLT_DRV] * 512, fdrv = p[P_FLT_DRV], ic1 = v->s[0], ic2 = v->s[1];
    int32_t mo = clamp(p[P_LN1], 0, 127);
    mono_flt_t flt;
    for (i = 0; i < n; i++) {
        if (!(i & 7u)) {
            mono_glide(cs, ct);
            mono_fcoef(&flt, *cs - 1 + cut, p[P_FLT_RES]);
        }
        int32_t a = mono_osc(w1, &v->ph[0], inc1, pw, &v->s[4], &v->s[6]);
        int32_t b = mono_osc(w2, &v->ph[1], inc2, 0u, &v->s[5], &v->s[7]);
        int32_t y = (a * (127 - (int32_t)mix) + b * (int32_t)mix) / 127;
        int32_t s = osc_pulse(v->ph[2], incs, 0x80000000u);
        v->ph[2] += incs;
        y = (y * 3 + ((s * sublv) >> 7) * 2) >> 2;
        if (sat)
            y = softclip((int32_t)(((int64_t)y * drv) >> 15));
        if (fdrv)
            y = softclip(((y >> 2) * (drive >> 2)) >> 11);
        y = mono_filter(&flt, mo, y, &ic1, &ic2);
        out[i] += voice_amp(y, m, i);
    }
    v->s[0] = ic1;
    v->s[1] = ic2;
}

static const preset_t MONO_PRESETS[1] = {
    {"SAW", {0, 0, 0, 0, 0, 0, 0, 0}, {0, 64, 127, 20}, 0, 1, FX(0, 0, 0, 0), PAT(0)},
};

static const engine_t ENG_MONO = {
    .name = "MONO",
    .page_title = {"OSC 1", "OSC 2"},
    .edit = {{"WAVE", F_ENUM, 0, MONO_NW - 1, 0, N_MWAVE, 0}, {"MIX", F_INT, 0, 127, 0, 0, 0},
             {"FINE", F_BIPCT, -64, 63, 0, 0, 0}, {"SEMI", F_INT, 0, 24, 0, 0, 0},
             {"SUB", F_ENUM, 0, 2, 0, N_MSUB, 0}, {"SUBVL", F_INT, 0, 127, 0, 0, 0}, {"PW", F_INT, 0, 127, 0, 0, 0},
             {"MACH", F_ENUM, 0, 0, 0, N_MMACH, 0}},
    .presets = MONO_PRESETS,
    .npresets = 1,
    .note_on = mono_note_on,
    .render = mono_render,
    .knob = {P_E0, P_E1, P_E2, P_E3},
    .poly = 1,
    .keep = 0xF3u,
};

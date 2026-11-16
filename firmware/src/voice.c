/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

static uint32_t vage;
static int32_t lfo_wave(track_t *t, uint32_t ph)
{
    switch (t->p[P_LWAVE]) {
    case 1:
        return osc_tri(ph);
    case 2:
        return (int32_t)(ph >> 16) - 32768;
    case 3:
        return ph < 0x80000000u ? 32767 : -32767;
    case 4:
        return (int32_t)(t->lfo_rnd >> 16) - 32768;
    default:
        return osc_sine(ph);
    }
}

#define LFO_SHARED_RNG 3u
static uint32_t lfo_rand(track_t *t)
{
    uint32_t s = t->lfo_rnd ? t->lfo_rnd : 0x9E3779B9u;
    if ((uint32_t)(t - trk) < LFO_SHARED_RNG)
        return rng();
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}

static uint32_t div_samples(uint32_t div);
static __attribute__((noinline)) uint32_t lfo_sync_inc(const track_t *t)

{
    uint32_t cyc, q, r;
    cyc = div_samples(LSYNC_DIV[(uint32_t)t->p[P_LSYNC] % NELEM(LSYNC_DIV)]);
    cyc = cyc ? cyc : 1u;
    q = 0xFFFFFFFFu / cyc;
    r = 0xFFFFFFFFu % cyc;
    return q * CTL + (r * CTL + CTL) / cyc;
}

#if FELUCCA_CPU2

static void track_lfo_adv(track_t *t)
{
    uint32_t old = t->lfo_ph;
    t->lfo_ph += t->p[P_LSYNC] ? lfo_sync_inc(t) : LFO_INC[t->p[P_LRATE] & 127];
    t->lfo_wrap = t->lfo_ph < old;
}
static void track_lfo_end(track_t *t)
{
    if (t->lfo_wrap) {
        t->lfo_wrap = 0;
        t->lfo_prev = (int16_t)mod_sh(t);
        t->lfo_rnd = lfo_rand(t);
    }
    t->lfo_val = lfo_wave(t, t->lfo_ph);
    if (t->p[P_LPOL])
        t->lfo_val = (t->lfo_val + 32768) >> 1;
    if (t->lfo_fade < 32767) {
        int32_t step = (int32_t)(ENV_LIN[t->p[P_LFADE] & 127] >> 9);
        t->lfo_fade = t->p[P_LFADE] ? clamp(t->lfo_fade + (step ? step : 1), 0, 32767) : 32767;
    }
}
#else
static void track_lfo_tick(track_t *t)
{
    uint32_t old = t->lfo_ph;
    t->lfo_ph += t->p[P_LSYNC] ? lfo_sync_inc(t) : LFO_INC[t->p[P_LRATE] & 127];
    if (t->lfo_ph < old) {
        t->lfo_prev = (int16_t)mod_sh(t);
        t->lfo_rnd = lfo_rand(t);
    }
    t->lfo_val = lfo_wave(t, t->lfo_ph);
    if (t->p[P_LPOL])
        t->lfo_val = (t->lfo_val + 32768) >> 1;
    if (t->lfo_fade < 32767) {
        int32_t step = (int32_t)(ENV_LIN[t->p[P_LFADE] & 127] >> 9);
        t->lfo_fade = t->p[P_LFADE] ? clamp(t->lfo_fade + (step ? step : 1), 0, 32767) : 32767;
    }
}
#endif

static uint32_t trk_nvoice(const track_t *t)
{
    uint32_t c = ENGINES[t->engine]->poly;
    return c && c < NVOICE ? c : NVOICE;
}

static uint32_t trk_vmode(const track_t *t)
{
    return t->engine == ENGI_MONO ? V_MONO : V_POLY;
}

static uint32_t voices_busy(void)
{
    uint32_t p, i, n = 0;
    for (p = 0; p < NPART; p++)
        for (i = 0; i < NVOICE; i++)
            n += trk[p].v[i].active && trk[p].v[i].stage != 4u;
    return n;
}

static uint32_t lowest_held(const track_t *t)
{
    uint32_t i, low = NVOICE;
    for (i = 0; i < NVOICE; i++)
        if (t->v[i].active && t->v[i].gate && (low == NVOICE || t->v[i].note < t->v[low].note))
            low = i;
    return low;
}

static uint32_t voice_victim(const track_t *self, int soft, track_t **pp)
{
    uint32_t p, i, best = NVOICE, cat = 4;
    for (p = 0; p < NPART; p++) {
        track_t *t = &trk[p];
        uint32_t mode = trk_vmode(t);
        uint32_t low = mode == V_POLY ? lowest_held(t) : NVOICE;
        if (soft && t == self)
            continue;
        for (i = 0; i < NVOICE; i++) {
            const voice_t *v = &t->v[i];
            uint32_t c;
            if (!v->active || v->stage == 4u)
                continue;
            if (!v->gate)
                c = 1;
            else if (mode != V_POLY && i > 0)
                c = 2;
            else if (!soft && mode == V_POLY && i != low)
                c = 3;
            else
                continue;
            if (c < cat || (c == cat && v->age < (*pp)->v[best].age)) {
                cat = c;
                best = i;
                *pp = t;
            }
        }
    }
    return best;
}

static uint32_t voice_kills;
static void voice_kill(voice_t *v)
{
    v->gate = 0;
    v->stage = 4;
    voice_kills++;
}

static int voice_room(track_t *t, int soft)
{
    track_t *vp = 0;
    uint32_t k;
    if (voices_busy() < NVOICE)
        return 1;
    k = voice_victim(t, soft, &vp);
    if (k == NVOICE)
        return !soft;

    voice_kill(&vp->v[k]);
    return 1;
}

static voice_t *voice_reuse(track_t *t, voice_t *v)
{
    if (v->stage == 4u)
        voice_room(t, 0);
    return v;
}

static voice_t *voice_alloc(track_t *t, uint32_t note)
{
    uint32_t i, np = trk_nvoice(t), best = np, low = np, nfree = 0;
    int32_t bd = 0x7FFFFFFF;
    for (i = 0; i < np; i++) {
        if (t->v[i].active && t->v[i].note == note)
            return voice_reuse(t, &t->v[i]);
        nfree += !t->v[i].active;
    }
    if (nfree && voices_busy() >= NVOICE) {
        track_t *vp = 0;
        uint32_t k = voice_victim(t, 0, &vp);
        if (k < np && vp == t)
            return &t->v[k];
        if (k < NVOICE)
            voice_kill(&vp->v[k]);
    }
    if (t->p[P_ALLOC] || t->p[P_GLIDE]) {
        for (i = 0; i < np; i++) {
            int32_t d = t->v[i].pitch_cur - (int32_t)note * 16;
            if (t->v[i].active)
                continue;
            d = d < 0 ? -d : d;
            if (!t->v[i].pitch_cur)
                d = 0x7FFFFFF0;
            if (d < bd) {
                bd = d;
                best = i;
            }
        }
    } else {
        for (i = 0; i < np && best == np; i++) {
            uint32_t k = (t->rr + i) % np;
            if (!t->v[k].active)
                best = k;
        }
        t->rr = (uint8_t)((best == np ? t->rr : best) + 1u) % np;
    }
    if (best < np)
        return &t->v[best];
    for (i = 0; i < np; i++)
        if (t->v[i].gate && (low == np || t->v[i].note < t->v[low].note))
            low = i;
    for (i = 0; i < np; i++)
        if (!t->v[i].gate && t->v[i].stage != 4u && (best == np || t->v[i].age < t->v[best].age))
            best = i;
    if (best == np)
        for (i = 0; i < np; i++)
            if (i != low && (best == np || t->v[i].age < t->v[best].age))
                best = i;
    return voice_reuse(t, &t->v[best == np ? 0 : best]);
}

static void glide_set(track_t *t, voice_t *v, int glide)
{
    if (!glide || !v->pitch_cur) {
        v->pitch_cur = v->pitch16;
        v->gstep = 0;
        return;
    }
    if (t->p[P_GLMODE] || t->engine == ENGI_ANALOG) {
        int32_t d = v->pitch16 - v->pitch_cur, ticks = 1 + t->p[P_GLIDE] * t->p[P_GLIDE] / 8;
        d = d < 0 ? -d : d;
        v->gstep = d / ticks > 0 ? d / ticks : 1;
    } else {
        v->gstep = 0;
    }
}

static int32_t voice_pitch16(const track_t *t, uint32_t note)
{
    int32_t n = (int32_t)note;
    if (FELUCCA_SIMPLE && (t->engine == ENGI_SAMPLER || (t->engine == ENGI_DIGITAL && t->p[P_E7] % 6 == 0)))
        n = clamp(n + t->p[P_TRANS], 0, 127);
    return n * 16;
}

static void voice_start(track_t *t, voice_t *v, uint32_t note, uint32_t vel, int glide)
{
    const engine_t *e = ENGINES[t->engine];
    int sounding = v->active && v->stage != 0;
    uint32_t ph0 = v->ph[0], ph1 = v->ph[1], ph2 = v->ph[2];
    int32_t s0 = v->s[0], s1 = v->s[1], s4 = v->s[4], s5 = v->s[5], s6 = v->s[6], s7 = v->s[7];
    uint32_t keep = e->keep;
    v->note = (uint8_t)note;
    v->vel = (uint8_t)vel;
    v->gate = 1;
    v->active = 1;
    v->stage = 1;
    v->age = ++vage;
    v->pitch16 = voice_pitch16(t, note);
    v->mvel = t->m_vel;
    v->mrnd = t->m_rnd;
    t->m_vi = (uint8_t)(v - t->v);
    glide_set(t, v, glide);
    if (!sounding) {
        v->env = 0;
        v->env_out = 0;
        v->side = t->sp_alt;
        t->sp_alt ^= 1u;
    }
    if (FELUCCA_SIMPLE && !MOD(t).on && mod_begin(t)) {
        e->note_on(t, v);
        mod_end(t);
    } else {
        e->note_on(t, v);
    }
    if (sounding && !e->sampled) {
        v->ph[0] = ph0;
        v->ph[1] = ph1;
        v->ph[2] = ph2;
        if (keep & 0x01u)
            v->s[0] = s0;
        if (keep & 0x02u)
            v->s[1] = s1;
        if (keep & 0x10u)
            v->s[4] = s4;
        if (keep & 0x20u)
            v->s[5] = s5;
        if (keep & 0x40u)
            v->s[6] = s6;
        if (keep & 0x80u)
            v->s[7] = s7;
    }
}

static void mono_play(track_t *t, uint32_t note, uint32_t vel, int retrig, int glide)
{
    uint32_t mode = (uint32_t)t->p[P_VOICE], nv = mode == V_UNISON ? trk_nvoice(t) : 1u, i;
    for (i = 0; i < nv; i++) {
        voice_t *v = &t->v[i];
        int32_t k = 2 * (int32_t)i - (int32_t)(nv - 1u);
        if (nv > 1u && nv < NVOICE)
            k = k * (NVOICE - 1) / (int32_t)(nv - 1u);
        v->fine = nv > 1u ? k * t->p[P_DETUNE] * 56 / 889 : 0;
        if (retrig || !v->active || !v->gate) {
            if ((!v->active || v->stage == 4u) && !voice_room(t, i > 0))
                continue;

            voice_start(t, v, note, nv >= NVOICE ? vel * 36u / 100u : nv > 1u ? vel / 2u : vel, glide);
            v->side = (uint8_t)(i & 1u);
            if (nv > 1u && i && !ENGINES[t->engine]->sampled) {
                static uint32_t seed = 0x1234567u;
                seed = seed * 1664525u + 1013904223u;
                v->ph[0] += seed;
            }
        } else {
            v->note = (uint8_t)note;
            v->pitch16 = voice_pitch16(t, note);
            v->mvel = t->m_vel;
            v->mrnd = t->m_rnd;
            if (vel > v->vel && nv == 1u)
                v->vel = (uint8_t)vel;
            glide_set(t, v, glide);
        }
    }
    t->mono_note = (uint8_t)note;
}

static uint32_t mono_pick(const track_t *t)
{
    uint32_t i, n;
    if (!t->nmono)
        return 0;
    n = t->mono_stack[t->nmono - 1u];
    for (i = 0; i < t->nmono; i++) {
        if (t->p[P_PRIO] == 1 && t->mono_stack[i] < n)
            n = t->mono_stack[i];
        if (t->p[P_PRIO] == 2 && t->mono_stack[i] > n)
            n = t->mono_stack[i];
    }
    return n;
}

static void mono_remove(track_t *t, uint32_t note)
{
    uint32_t j, k = 0;
    for (j = 0; j < t->nmono; j++)
        if (t->mono_stack[j] != note)
            t->mono_stack[k++] = t->mono_stack[j];
    t->nmono = (uint8_t)k;
}

static uint8_t trk_slide[NTRK];
static uint32_t trk_hit_ms[NTRK];
static uint32_t fol_on(track_t *t, uint32_t note);
static uint32_t fol_off(track_t *t, uint32_t note);
static void fol_drop(const track_t *t);
static void trk_note_off(track_t *t, uint32_t note);
static void trk_note_on(track_t *t, uint32_t note, uint32_t vel)
{
    uint32_t any = 0, i, mode = trk_vmode(t);
    if (t->p[P_MUTE])
        return;
    trk_hit_ms[(uint32_t)(t - trk) % NTRK] = fm1_ms | 1u;
    if (t->xf_on || t->eng_req != t->engine) {
        for (i = 0; i < t->xp_n && t->xp_note[i] != note; i++)
            ;
        if (i == t->xp_n && t->xp_n < 4u)
            t->xp_n++;
        i = i < t->xp_n ? i : t->xp_n - 1u;
        t->xp_note[i] = (uint8_t)note;
        t->xp_vel[i] = (uint8_t)vel;
        return;
    }
    note = fol_on(t, note);
    vel = mod_note(t, note, vel);
    for (i = 0; i < NVOICE; i++)
        any |= t->v[i].gate;
    if (!any) {
        if (!t->p[P_LTRIG])
            t->lfo_ph = (uint32_t)t->p[P_LPHASE] << 25;
        t->lfo_fade = 0;
    }
    if (mode == V_POLY && FELUCCA_SIMPLE && trk_machine(t)->drum) {
        voice_t *v = 0;
        int gl;
        for (i = 0; i < NVOICE; i++)
            if (t->v[i].active && t->v[i].stage != 4u && (!v || t->v[i].note == note || (v->note != note && t->v[i].age > v->age)))
                v = &t->v[i];
        gl = v != 0;
        if (!v)
            v = voice_alloc(t, note);
        for (i = 0; i < NVOICE; i++)
            if (&t->v[i] != v && t->v[i].active && t->v[i].stage != 4u)
                voice_kill(&t->v[i]);
        t->nmono = 0;
        t->mono_note = 0;
        v->fine = 0;
        voice_start(t, v, note, vel, gl && t->engine == ENGI_ANALOG && t->p[P_GLIDE] != 0);
        if (t->engine == ENGI_ANALOG && (uint32_t)t->p[P_E7] % AN_N == AN_HAT) {
            uint32_t k;
            for (k = 0; k < NTRK; k++) {
                track_t *u = &trk[k];
                if (u == t || u->engine != ENGI_ANALOG || (uint32_t)u->p[P_E7] % AN_N != AN_HAT || u->p[P_E1] <= t->p[P_E1])
                    continue;
                for (i = 0; i < NVOICE; i++)
                    if (u->v[i].active && u->v[i].stage != 4u)
                        voice_kill(&u->v[i]);
            }
        }
        return;
    }
    if (mode == V_POLY && FELUCCA_SIMPLE && t->slide_glide == 2u) {
        voice_t *v = 0;
        for (i = 0; i < NVOICE; i++)
            if (t->v[i].active && t->v[i].gate && (!v || t->v[i].age > v->age))
                v = &t->v[i];
        if (v) {
            v->note = (uint8_t)note;
            v->pitch16 = voice_pitch16(t, note);
            glide_set(t, v, 1);
            return;
        }
    }
    if (mode == V_POLY) {
        voice_t *v = voice_alloc(t, note);
        t->nmono = 0;
        t->mono_note = 0;
        v->fine = 0;
        voice_start(t, v, note, vel, t->p[P_GLIDE] != 0 && !ENGINES[t->engine]->oneshot);
        return;
    }
    {
        uint32_t sounding = t->mono_note && t->v[0].active && t->v[0].gate, want;
        mono_remove(t, note);
        if (t->nmono >= 8u)
            mono_remove(t, t->mono_stack[0]);
        t->mono_stack[t->nmono++] = (uint8_t)note;
        want = mono_pick(t);
        if (sounding && want == t->mono_note)
            return;
        if (mode == V_MONO)
            mono_play(t, want, vel, t->slide_glide != 2u, t->p[P_GLIDE] != 0 || t->slide_glide);
        else
            mono_play(t, want, vel, !sounding, sounding && (t->p[P_GLIDE] != 0 || t->slide_glide));
    }
}

static void trk_note_off(track_t *t, uint32_t note)
{
    uint32_t i, k = 0, mode = trk_vmode(t);
    for (i = 0; i < t->xp_n; i++)
        if (t->xp_note[i] != note) {
            t->xp_note[k] = t->xp_note[i];
            t->xp_vel[k++] = t->xp_vel[i];
        }
    t->xp_n = (uint8_t)k;
    note = fol_off(t, note);
    if (note > 127u) return;
    mono_remove(t, note);
    if (mode != V_POLY) {
        uint32_t nv = mode == V_UNISON ? trk_nvoice(t) : 1u;
        if (t->mono_note == note) {
            uint32_t next = mono_pick(t);
            if (next) {
                mono_play(t, next, t->v[0].vel, 0, t->p[P_GLIDE] != 0);
            } else {
                for (i = 0; i < nv; i++)
                    if (t->v[i].gate) {
                        t->v[i].gate = 0;
                        t->v[i].stage = 3;
                    }
                t->mono_note = 0;
            }
        }
    }
    for (i = 0; i < NVOICE; i++)
        if (t->v[i].note == note && t->v[i].gate) {
            t->v[i].gate = 0;
            t->v[i].stage = 3;
        }
}

static void trk_all_off(track_t *t)
{
    uint32_t i;
    fol_drop(t);
    for (i = 0; i < NVOICE; i++) {
        t->v[i].gate = 0;
        t->v[i].stage = t->v[i].active ? (t->v[i].stage == 4 ? 4 : 3) : 0;
    }
    t->nmono = 0;
    t->mono_note = 0;
    t->xp_n = 0;
}

#define XF_BLOCKS 4u
static void engine_block(track_t *t)
{
    uint32_t i, any = 0;
    if (t->xf_on || t->eng_req != t->engine) {
        if (!t->xf_on) {
            for (i = 0; i < NVOICE; i++)
                any |= t->v[i].active;
            if (any) {
                t->xf_on = 1;
                t->xf = XF_BLOCKS;
                return;
            }
        } else if (t->xf) {
            return;
        }
        for (i = 0; i < NVOICE; i++) {
            voice_t *v = &t->v[i];
            v->active = v->gate = 0;
            v->stage = 0;
            v->env = v->env_out = 0;
        }
        t->engine = eng_idx(t->eng_req);
        t->xf_on = 0;
        t->nmono = 0;
        t->mono_note = 0;
        {
            uint32_t n = t->xp_n;
            t->xp_n = 0;
            for (i = 0; i < n; i++)
                trk_note_on(t, t->xp_note[i], t->xp_vel[i]);
        }
    }
    for (i = 0; i < 8u; i++)
        t->pe_old[i] = t->p[P_E0 + i];
}

static uint32_t beat_samples(void);
_Static_assert(CTL == 32, "esync_one: 4.6 CTL in Q16 is 9646899");
#if FELUCCA_CPU2
#define T_ESYNC(t) ((t)->esync_k)
#else
static uint32_t esync_k[3];
#define T_ESYNC(t) esync_k
#endif
static uint32_t esync_one(uint32_t quarter, int32_t v, uint32_t i)
{
    uint32_t u = ESYNC_UNITS[esync_idx(v)], T = quarter * u / 96u, x, x2, x3, x4;
    if (T < CTL)
        return i ? ENV_EXP[0] : ENV_LIN[0];
    if (!i)
        return ((1u << 24) * CTL + T / 2u) / T;
    x = (9646899u + T / 2u) / T;
    if (x > 49152u)
        x = 49152u;
    x2 = x * x >> 16;
    x3 = x2 * x >> 16;
    x4 = x3 * x >> 16;
    x = x - x2 / 2u + x3 / 6u - x4 / 24u;
    return x < 1u ? 1u : x > 65535u ? 65535u : x;
}
static __attribute__((noinline)) void esync_coef(track_t *t)

{
    const int16_t *p = t->p;
    if (p[P_ESYNC]) {
        uint32_t q = beat_samples();
        T_ESYNC(t)[0] = esync_one(q, p[P_ATK], 0);
        T_ESYNC(t)[1] = esync_one(q, p[P_DEC], 1);
        T_ESYNC(t)[2] = esync_one(q, p[P_REL], 2);
    } else {
        T_ESYNC(t)[0] = ENV_LIN[p[P_ATK] & 127];
        T_ESYNC(t)[1] = ENV_EXP[p[P_DEC] & 127];
        T_ESYNC(t)[2] = ENV_EXP[p[P_REL] & 127];
    }
}

static int32_t env_tick(track_t *t, voice_t *v)
{
    const int16_t *p = t->p;
    int32_t sus = (int32_t)p[P_SUS] << 17;
    int ad = trk_machine(t)->drum == 1;
    if (ad) {
        sus = 0;
        if (v->stage == 3)
            v->stage = 2;
    }
    switch (v->stage) {
    case 1:
        v->env += (int32_t)T_ESYNC(t)[0];
        if (v->env >= (1 << 24)) {
            v->env = 1 << 24;
            v->stage = 2;
        }
        break;
    case 2:
        v->env += mulq16(sus - v->env, T_ESYNC(t)[1]);
        if (ad && v->env < (1 << 12)) {
            v->env = 0;
            v->stage = 0;
            v->active = 0;
        }
        break;
    case 3:
        v->env -= mulq16(v->env, T_ESYNC(t)[2]);
        if (v->env < (1 << 12)) {
            v->env = 0;
            v->stage = 0;
            v->active = 0;
        }
        break;
    case 4:
        v->env >>= 1;
        if (v->env < (1 << 16)) {
            v->env = 0;
            v->stage = 0;
            v->active = 0;
        }
        break;
    default:
        v->env = 0;
        break;
    }
    return v->env >> 9;
}

#if FELUCCA_CPU2
#define T_SPREAD(t) ((t)->spread_on)
static int32_t spread_buf[NTRK][CTL];
#define SPREAD_BUF(t) spread_buf[C1_PI(t)]
#define SPREAD_CLEAR(t) spread_clear(SPREAD_BUF(t))
#define SPREAD_SUM(out, t) spread_sum(out, SPREAD_BUF(t))
static __attribute__((noinline)) void spread_clear(int32_t *sb)
{
    uint32_t i;
    for (i = 0; i < CTL; i++)
        sb[i] = 0;
}
static __attribute__((noinline)) void spread_sum(int32_t *out, const int32_t *sb)
{
    uint32_t i;
    for (i = 0; i < CTL; i++)
        out[i] += sb[i];
}
#else
static uint8_t spread_on;
#define T_SPREAD(t) spread_on
static int32_t spread_buf[CTL];
#define SPREAD_BUF(t) spread_buf
#define SPREAD_CLEAR(t) spread_clear()
#define SPREAD_SUM(out, t) spread_sum(out)
static __attribute__((noinline)) void spread_clear(void)
{
    uint32_t i;
    for (i = 0; i < CTL; i++)
        spread_buf[i] = 0;
}
static __attribute__((noinline)) void spread_sum(int32_t *out)
{
    uint32_t i;
    for (i = 0; i < CTL; i++)
        out[i] += spread_buf[i];
}
#endif

static int32_t midi_bend_q8[NTRK], midi_bend_target[NTRK];
static uint32_t track_render(track_t *t, int32_t *out, uint32_t n)
{
    const engine_t *e = ENGINES[t->engine];
    const int16_t *p = t->p;
    uint32_t i;
    int32_t lfo = mulq15(t->lfo_val, t->lfo_fade);

    int32_t tune = song.g[G_TUNE] >= 0 ? song.g[G_TUNE] * 16 / 100 : -((-song.g[G_TUNE] * 16 + 99) / 100);
    int32_t tune_fine = (song.g[G_TUNE] * 16 - tune * 100) * 2367 / 16000;
    int32_t bend, bend16, bend_fine;
    uint32_t ti = (uint32_t)(t - trk);
    {
        int32_t d = midi_bend_target[ti] - midi_bend_q8[ti];

        midi_bend_q8[ti] += d > 0 ? (d < 4 ? d : (d + 3) / 4) : (d > -4 ? d : (d - 3) / 4);
    }
    bend = midi_bend_q8[ti];
    bend16 = bend >= 0 ? bend / 16 : -((-bend + 15) / 16);
    bend_fine = (bend - bend16 * 16) * 2367 / 2560;
    uint32_t nr = 0, fade = t->xf_on && t->xf;
    int16_t pe_new[8];
    for (i = 0; i < n; i++)
        out[i] = 0;
    if (MOD(t).on && MOD(t).ldep < 32767)
        lfo = mulq15(lfo, MOD(t).ldep);
    if (T_SPREAD(t))
        SPREAD_CLEAR(t);
    if (fade)
        for (i = 0; i < 8u; i++) {
            pe_new[i] = t->p[P_E0 + i];
            t->p[P_E0 + i] = t->pe_old[i];
        }
#if FELUCCA_CPU2
    track_lfo_adv(t);
#else
    track_lfo_tick(t);
#endif
    esync_coef(t);
    if (e->block)
        e->block(t);
    for (i = 0; i < NVOICE; i++) {
        voice_t *v = &t->v[i];
        vmod_t m;
        int32_t env, pitch;
        if (!v->active)
            continue;
        {
            env = e->ownenv && v->stage != 4u ? 32767 : env_tick(t, v);
            if (e->ownenv && v->active && v->stage != 4u) {

                if (e->done(t, v)) {
                    v->active = v->gate = 0;
                    v->stage = 0;
                    v->env = v->env_out = 0;
                    continue;
                }
                v->env = 1 << 24;
                env = 32767;
            }
            if (e->amp)
                env = e->amp(t, v, env);
            m.envq15 = e->ownenv ? 0 : env;
            m.amp1 = e->ownenv ? env : mulq15(env, v->vel * 258);
            if (p[P_LD_AMP])
                m.amp1 = mulq15(m.amp1, 32767 - mulq15(p[P_LPOL] ? lfo : (lfo + 32768) >> 1, p[P_LD_AMP] * 258));
            if (fade)
                m.amp1 = m.amp1 * (int32_t)(t->xf - 1u) / (int32_t)XF_BLOCKS;
            m.amp0 = v->env_out;
            v->env_out = m.amp1;
        }
        if (v->pitch_cur != v->pitch16) {
            int32_t d = v->pitch16 - v->pitch_cur;
            int32_t st = v->gstep ? v->gstep : (int32_t)(ENV_LIN[(p[P_GLIDE] ? p[P_GLIDE] : trk_slide[(uint32_t)(t - trk) % NTRK] ? trk_slide[(uint32_t)(t - trk) % NTRK] : 40) & 127] >> 14);
            if (st < 1)
                st = 1;
            v->pitch_cur += d > 0 ? (d < st ? d : st) : (-d < st ? d : -st);
        }
        if (!env && !m.amp0 && v->stage == 2 && !e->sampled)
            continue;
        pitch = v->pitch_cur + tune + bend16 + ((lfo * p[P_LD_PIT] * 3) >> 15) + ((m.envq15 * p[P_ED_PIT] * 3) >> 15);
        m.pitch16 = clamp(pitch, 0, 2047);
        m.inc = pitch_inc(m.pitch16);
        m.fine = v->fine + tune_fine + bend_fine;
        if (v->fine + tune_fine + bend_fine)
            m.inc += (uint32_t)((int32_t)(m.inc >> 12) * (v->fine + tune_fine + bend_fine));
        m.cutoff = ((lfo * p[P_LD_FLT]) >> 7) + ((m.envq15 * p[P_ED_FLT]) >> 7);
        if (v->vel > 110)
            m.cutoff += (m.envq15 * 24) >> 7;
        m.shape = (64 << 8) + ((lfo * p[P_LD_SHP]) >> 7) + ((m.envq15 * p[P_ED_SHP]) >> 7);
        if (MOD(t).on)
            mod_voice(t, v, &m, v->fine + tune_fine + bend_fine);
        e->render(t, v, T_SPREAD(t) > v->side ? SPREAD_BUF(t) : out, n, &m);
        nr++;
    }
    if (T_SPREAD(t))
        SPREAD_SUM(out, t);
    if (fade) {
        for (i = 0; i < 8u; i++)
            t->p[P_E0 + i] = pe_new[i];
        t->xf--;
    }
    if (e->post) e->post(t, out, n);
    return nr;
}

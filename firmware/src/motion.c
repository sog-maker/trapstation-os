/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

static const uint8_t MOTION_G[] = {G_MIDI, G_SYNC, G_CRATE, G_CDEPTH, G_INFO, G_DTIME, G_DFDBK, G_DCOLOR, G_RSIZE, G_RDAMP,
                                   G_RTYPE, G_AMP, G_F1AMT, G_F2AMT, G_F3AMT, G_PAN};
#define MOTION_NG 16u
_Static_assert(MOTION_NG <= 16u, "the FX lane's lk: 16 bits (seq.c fxl)");
#define MOTION_N (P_COUNT + MOTION_NG)
_Static_assert(MOTION_N <= 128u, "a record's id is 7 bits (MOTION_ID)");
static int16_t *motion_vp(track_t *t, uint32_t id)
{
    if (id >= P_COUNT)
        return &song.g[MOTION_G[(id - P_COUNT) % MOTION_NG]];
    return &t->p[id];
}
static const param_desc_t *motion_desc(const track_t *t, uint32_t id)
{
    if (id >= P_COUNT)
        return &GP[MOTION_G[(id - P_COUNT) % MOTION_NG]];
    return param_desc_of(eng_idx(t->eng_req), id);
}
static uint32_t motion_gid(uint32_t g)
{
    uint32_t j;
    for (j = 0; j < MOTION_NG; j++)
        if (MOTION_G[j] == g)
            return P_COUNT + j;
    (void)g;
    return 0xFFu;
}
static uint32_t step_samples(const track_t *t, uint32_t period, uint32_t idx);
static motion_store_t motion;
static int16_t motion_base[NTRK][MOTION_N];
static uint32_t motion_active[NTRK][(MOTION_N + 31u) / 32u];
static uint32_t motion_locked[NTRK][(MOTION_N + 31u) / 32u];
static void motion_snap(uint32_t k, track_t *t)
{
    uint32_t id;
    for (id = 0; id < MOTION_N; id++)
        motion_base[k][id] = *motion_vp(t, id);
}
static uint8_t motion_base_valid, motion_full;
static uint32_t motion_guard(void)
{
#if defined(FM1_IRQ_TARGET)
    uint32_t f = fm1_icfg();
    fm1_irq_off();
    return f;
#else
    return 0;
#endif
}
static void motion_unguard(uint32_t f)
{
#if defined(FM1_IRQ_TARGET)
    fm1_icfg_set(f);
#else
    (void)f;
#endif
}
static int motion_param(uint32_t id)
{

    if (id >= P_COUNT)
        return id < MOTION_N;
    if (FELUCCA_SIMPLE && (id == P_LSYNC || id == P_LTRIG || id == P_LPOL))
        return 1;
    return (id <= P_REL || (id >= P_ED_FLT && id <= P_LD_AMP) ||
        (id >= P_DIST && id <= P_REV) || id == P_GLIDE || id == P_PAN || (FELUCCA_SIMPLE && id == P_LEVEL) ||
        id == P_DETUNE || (id >= P_FM1_ATK && id <= P_FM4_LEVEL) || (id >= P_LN0 && id <= P_LN7) ||
        id == P_SPRD || (id >= P_ITYPE && id <= P_IMIX) || id >= P_E0 ||
        (FELUCCA_SIMPLE && (id == P_M1AMT || id == P_M1DST || id == P_M2AMT || id == P_M2DST || id == P_FTYPE || id == P_TRANS)));

}
static int motion_enabled(const track_t *t) { return (motion.on >> trk_index(t)) & 1u; }
static uint32_t motion_count(const track_t *t)
{
    uint32_t i, n = 0, k = trk_index(t);
    for (i = 0; i < motion.count; i++) n += (motion.event[i].place >> 6) == k;
    return n;
}

static uint64_t motion_lock_steps(uint32_t k)
{
    uint32_t i;
    uint64_t m = 0;
    for (i = 0; i < motion.count; i++)
        if ((motion.event[i].place >> 6) == k && (motion.event[i].param & MOTION_LOCK))
            m |= (uint64_t)1 << (motion.event[i].place & 63u);
    return m;
}

static int motion_lock_get(const track_t *t, uint32_t step, uint32_t id, int16_t *v)
{
    uint32_t i, place = trk_index(t) << 6 | step;
    for (i = 0; i < motion.count; i++)
        if (motion.event[i].place == place && motion.event[i].param == (id | MOTION_LOCK)) {
            *v = motion.event[i].value;
            return 1;
        }
    return 0;
}
static int16_t motion_base_value(const track_t *t, uint32_t id)
{
    uint32_t k = trk_index(t);
    return (motion_active[k][id / 32u] >> (id % 32u)) & 1u ? motion_base[k][id] : *motion_vp((track_t *)t, id);
}
static void motion_restore(track_t *t)
{
    uint32_t k = trk_index(t), id, f = motion_guard();
    for (id = 0; id < MOTION_N; id++)
        if ((motion_active[k][id / 32u] >> (id % 32u)) & 1u) *motion_vp(t, id) = motion_base[k][id];
    memset(motion_active[k], 0, sizeof motion_active[k]);
    memset(motion_locked[k], 0, sizeof motion_locked[k]);
    motion_unguard(f);
}
static void motion_rebase(track_t *t)
{
    uint32_t k = trk_index(t), f = motion_guard();
    motion_restore(t);
    motion_snap(k, t);
    motion_base_valid = (uint8_t)((motion_base_valid & ~(1u << k)) | (song.playing ? 1u << k : 0u));
    motion_unguard(f);
}
static void motion_begin(void)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++) {
        motion_restore(&trk[k]);
        motion_snap(k, &trk[k]);
    }
    motion_base_valid = (1u << NTRK) - 1u;
}
static void motion_end(void)
{
    for (uint32_t k = 0; k < NTRK; k++) motion_restore(&trk[k]);
    motion_base_valid = 0;
}
static void motion_set_enabled(track_t *t, uint32_t on)
{
    uint32_t f = motion_guard(), b = 1u << trk_index(t);
    motion.on = (uint8_t)(on ? motion.on | b : motion.on & ~b);
    if (!on) motion_restore(t);
    motion_unguard(f);
}
static void motion_clear(track_t *t)
{
    uint32_t f = motion_guard(), k = trk_index(t), i, n = 0;
    motion_restore(t);
    for (i = 0; i < motion.count; i++)
        if ((motion.event[i].place >> 6) != k) motion.event[n++] = motion.event[i];
    memset(motion.event + n, 0, (MOTION_MAX - n) * sizeof motion.event[0]);
    motion.count = (uint8_t)n;
    motion.on &= (uint8_t)~(1u << k);
    motion_rebase(t);
    motion_full = 0;
    motion_unguard(f);
}
static void motion_reset(track_t *t) { motion_clear(t); }

static int motion_put(track_t *t, uint32_t step, uint32_t id, int16_t value, uint32_t kind)
{
    uint32_t k = trk_index(t), i, f;
    const param_desc_t *d;
    if (k >= NTRK || step >= NSTEP || !motion_param(id)) return 1;
    d = motion_desc(t, id);
    if (value < d->min || value > d->max || value < -64 || value > 127) return 1;
    value = (int16_t)param_fit(d, value);
    f = motion_guard();
    for (i = 0; i < motion.count; i++)
        if (motion.event[i].place == (k << 6 | step) && MOTION_ID(&motion.event[i]) == id) break;
    if (i == MOTION_MAX) { motion_full = 1; motion_unguard(f); return 2; }
    motion.event[i].place = (uint16_t)(k << 6 | step);
    motion.event[i].param = (uint8_t)(id | kind);
    motion.event[i].value = (int8_t)value;
    RING_PUBLISH();
    if (i == motion.count) motion.count++;
    motion.on |= (uint8_t)(1u << k);
    motion_unguard(f);
    return 0;
}
static int motion_set_event(track_t *t, uint32_t step, uint32_t id, int16_t value) { return motion_put(t, step, id, value, 0); }
static int motion_set_lock(track_t *t, uint32_t step, uint32_t id, int16_t value)
{
    return motion_put(t, step, id, value, MOTION_LOCK);
}

static uint32_t motion_clear_locks(track_t *t, uint32_t step)
{
    uint32_t f = motion_guard(), k = trk_index(t), i, n = 0, gone;
    for (i = 0; i < motion.count; i++) {
        const motion_event_t *e = &motion.event[i];
        if ((e->place >> 6) == k && (e->param & MOTION_LOCK) && (step >= NSTEP || (e->place & 63u) == step)) continue;
        motion.event[n++] = *e;
    }
    gone = motion.count - n;
    memset(motion.event + n, 0, (MOTION_MAX - n) * sizeof motion.event[0]);
    motion.count = (uint8_t)n;
    motion_unguard(f);
    return gone;
}
static int motion_capture(track_t *t, uint32_t id, int16_t value)
{
    uint32_t k = trk_index(t), idx, len, period, f;
    if (!motion_param(id)) return 0;
    if (!rec_on(t) || t != TSEL) {

        f = motion_guard();
        if ((motion_base_valid >> k) & 1u) motion_base[k][id] = value;
        motion_unguard(f);
        return 0;
    }
    len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP);
    period = div_samples((uint32_t)t->p[P_SDIV]);
    f = motion_guard();
    idx = t->seq_idx % len;
    if (t->seq_pos < 0x7FFFFFFFu && t->seq_pos > step_samples(t, period, idx) / 2u) idx = (idx + 1u) % len;

    if ((motion_base_valid >> k) & 1u) motion_active[k][id / 32u] |= 1u << (id % 32u);
    motion_unguard(f);
    return motion_set_event(t, idx, id, value);
}

static __attribute__((noinline)) void motion_unlock(track_t *t, uint32_t step, const motion_store_t *m)
{
    uint32_t k = trk_index(t), id, i;
    for (id = 0; id < MOTION_N; id++) {
        int32_t best = -1;
        int16_t v;
        if (!((motion_locked[k][id / 32u] >> (id % 32u)) & 1u))
            continue;
        v = motion_base[k][id];
        for (i = 0; i < m->count; i++) {
            const motion_event_t *e = &m->event[i];
            int32_t s = (int32_t)(e->place & 63u);
            if ((e->place >> 6) != k || e->param != id || s > (int32_t)step || s <= best)
                continue;
            best = s;
            v = (int16_t)param_fit(motion_desc(t, id), e->value);
        }
        *motion_vp(t, id) = v;
    }
    memset(motion_locked[k], 0, sizeof motion_locked[k]);
}

static __attribute__((noinline)) void motion_step(track_t *t, uint32_t step, const motion_store_t *m, uint32_t play,
                                                  uint32_t trig)
{
    uint32_t k = trk_index(t), i, w, any = 0;
    if (!((m->on >> k) & 1u)) return;
    if (!((motion_base_valid >> k) & 1u)) {
        motion_snap(k, t);
        motion_base_valid |= (uint8_t)(1u << k);
    }
    for (w = 0; w < (MOTION_N + 31u) / 32u; w++)
        any |= motion_locked[k][w];
    if (!FELUCCA_SIMPLE) {
        if (!step) motion_restore(t);
        else if (any) motion_unlock(t, step, m);
    } else {
        if (any && trig) motion_unlock(t, step, m);
        if (!step) {
            uint32_t id;
            for (id = 0; id < MOTION_N; id++) {
                uint32_t b = 1u << (id % 32u);
                if ((motion_active[k][id / 32u] & b) && !(motion_locked[k][id / 32u] & b)) {
                    *motion_vp(t, id) = motion_base[k][id];
                    motion_active[k][id / 32u] &= ~b;
                }
            }
        }
    }
    for (i = 0; i < m->count; i++) {
        const motion_event_t *e = &m->event[i];
        uint32_t id = MOTION_ID(e);
        if (e->place != (k << 6 | step) || ((e->param & MOTION_LOCK) && !play)) continue;
        const param_desc_t *d = motion_desc(t, id);
        *motion_vp(t, id) = (int16_t)param_fit(d, e->value);
        motion_active[k][id / 32u] |= 1u << (id % 32u);
        if (e->param & MOTION_LOCK)
            motion_locked[k][id / 32u] |= 1u << (id % 32u);
    }
}
static void motion_snapshot_track(track_t *t, motion_store_t *out)
{
    uint32_t f = motion_guard(), k = trk_index(t), i;
    memset(out, 0, sizeof *out);
    out->on = motion.on & (uint8_t)(1u << k);
    for (i = 0; i < motion.count; i++)
        if ((motion.event[i].place >> 6) == k) out->event[out->count++] = motion.event[i];
    motion_unguard(f);
}

static int motion_engine_id(uint32_t id)
{
    return (id >= P_E0 && id <= P_E7) || (id >= P_FM1_ATK && id <= P_FM4_LEVEL) || (id >= P_LN0 && id <= P_LN7) || id == P_DIST;
}

static uint32_t motion_sound_loaded(track_t *t, uint32_t from, uint32_t to)
{
    uint32_t f, k = trk_index(t), i, n = 0, gone;
    if (eng_idx(from) == eng_idx(to))
        return 0;
    f = motion_guard();
    for (i = 0; i < motion.count; i++) {
        const motion_event_t *e = &motion.event[i];
        if ((e->place >> 6) == k && motion_engine_id(MOTION_ID(e)))
            continue;
        motion.event[n++] = *e;
    }
    gone = motion.count - n;
    memset(motion.event + n, 0, (MOTION_MAX - n) * sizeof motion.event[0]);
    motion.count = (uint8_t)n;
    motion_unguard(f);
    return gone;
}

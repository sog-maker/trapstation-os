/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

static const uint16_t SCALE_MASK[] = {
    0xFFF,
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 11),
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 8) | (1 << 10),
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 10),
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 10),
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 7) | (1 << 9),
    (1 << 0) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 10),
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 8) | (1 << 11),
    (1 << 0) | (1 << 1) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 8) | (1 << 10),
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 6) | (1 << 7) | (1 << 9) | (1 << 11),
    (1 << 0) | (1 << 1) | (1 << 3) | (1 << 5) | (1 << 6) | (1 << 8) | (1 << 10),
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 7) | (1 << 9) | (1 << 11),
    (1 << 0) | (1 << 3) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 10),
    (1 << 0) | (1 << 2) | (1 << 4) | (1 << 6) | (1 << 8) | (1 << 10),
    (1 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (1 << 6) | (1 << 7) | (1 << 9) | (1 << 10),
    (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 6) | (1 << 8) | (1 << 9) | (1 << 11),
};

#define KB_SILENT 255u
static uint32_t kb_prev;
static volatile uint8_t kb_boot_hold;

static uint8_t kb_note[27], kb_trk[27];

static uint8_t lastn[NTRK][4], lastc[NTRK], lheld[NTRK], kb_lh[27];
static uint32_t kb_eat;
static uint8_t kb_g[27];
static uint8_t in_key;
static uint8_t kb_vel = 100;

static uint8_t held_key[NTRK][16];
static int8_t held_dt[NTRK][16];
static volatile uint8_t kb_asleep;

static uint8_t last_note = 60;
static volatile uint8_t transport_req;
static volatile uint8_t panic_req;
#define PANIC_ALL 0xFFu
static volatile uint8_t midi_hint;
static volatile uint8_t midi_seen;

static uint32_t trk_index(const track_t *t) { return (uint32_t)(t - trk); }

static int32_t midi_base(void)
{
    uint32_t r = (uint32_t)song.g[G_ROUTE];
    return r == ROUTE_SEL ? -1 : r == ROUTE_TRACK ? -2 : r ? (int32_t)(r - 1u) * 4 : 0;
}

static uint32_t trk_midi_ch(uint32_t i)
{
    int32_t b = midi_base();
    if (b == -2)
        return mch_get(i) < 16u ? mch_get(i) : i % NTRK;
    return (b > 0 ? (uint32_t)b : 0u) + i % NTRK;
}

static int drum_track(const track_t *t) { (void)t; return 0; }

static int key_black(uint32_t k) { return (int)((0x54Au >> ((k + 5u) % 12u)) & 1u); }
static uint32_t key_place(uint32_t k)
{
    uint32_t i, n = 0;
    for (i = 0; i < k; i++)
        n += (uint32_t)(key_black(i) == key_black(k));
    return n;
}
enum { GK_ACC = NLANE, GK_PGDN, GK_PGUP };

static uint32_t grid_chord(const track_t *t, uint8_t *out);
static __attribute__((noinline)) uint8_t grid_key_note(uint32_t k)
{
    uint8_t c[4];
    return key_black(k) && key_place(k) < NTRK ? (grid_chord(&trk[key_place(k)], c), c[0]) : KB_SILENT;
}

#define SONG_KEYS (1u << 4 | 1u << 6 | 1u << 7 | 1u << 9)

static uint32_t scale_mask(const track_t *t)
{
    return SCALE_MASK[clamp(t->p[P_SCALE], 0, sizeof SCALE_MASK / sizeof SCALE_MASK[0] - 1)];
}

static int32_t scale_snap(const track_t *t, int32_t n)
{
    uint32_t mask = scale_mask(t), guard = 12;
    while (guard-- && !((mask >> (uint32_t)((n - t->p[P_ROOT] + 120) % 12)) & 1u))
        n--;
    return n;
}

enum { QN_OFF, QN_SNAP, QN_WHITE, QN_SEQ, QN_SEQT };

static int32_t kb_engine(const track_t *t, uint32_t k)
{
    const engine_t *e = ENGINES[eng_idx(t->eng_req)];
    return e->keys ? e->keys(t, k) : -1;
}

#define KB_TRN(t) (FELUCCA_SIMPLE ? 0 : (t)->p[P_TRANS])
static uint32_t kb_pitch(const track_t *t, uint32_t k, int32_t oct)
{
    static const int8_t DEGREE[12] = {0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6};
    int32_t n = 53 + (int32_t)k;
    if (t->p[P_QUANT] == QN_SNAP || t->p[P_QUANT] >= QN_SEQ)
        return (uint32_t)clamp(scale_snap(t, n + 12 * oct + KB_TRN(t)), 0, 127);
    if (t->p[P_QUANT] == QN_WHITE) {
        uint32_t mask = scale_mask(t), i;
        int32_t count = 0, degree = DEGREE[n % 12], wo;
        if (degree < 0)
            return KB_SILENT;

        degree += (n / 12 - 5) * 7;
        for (i = 0; i < 12u; i++)
            count += (mask >> i) & 1u;
        wo = degree / count;
        degree %= count;
        if (degree < 0) {
            degree += count;
            wo--;
        }
        for (i = 0; i < 12u; i++)
            if ((mask >> i) & 1u) {
                if (!degree)
                    break;
                degree--;
            }
        n = 60 + t->p[P_ROOT] + 12 * wo + (int32_t)i;
    }
    return (uint32_t)clamp(n + 12 * oct + KB_TRN(t), 0, 127);
}

static uint32_t kb_map(const track_t *t, uint32_t k)
{
    int32_t n = kb_engine(t, k);
    return n >= 0 ? (uint32_t)n : kb_pitch(t, k, song.octave);
}

#include "chord.c"
#include "trap_gen_live.c"
#include "trap_follow.c"

static uint32_t arp_am(const track_t *t);
static uint16_t arp_lv[NTRK];
static uint32_t arp_seed[NTRK];
static uint8_t arp_walk[NTRK];
static uint32_t arp_latch(const track_t *t) { return FELUCCA_SIMPLE ? 0u : (uint32_t)t->p[P_AHOLD]; }
static void arp_add(track_t *t, uint32_t note)
{
    uint32_t i, k = in_key - 1u;
    if (arp_latch(t) && t->arp_phys == 0u)
        t->nheld = 0;
    t->arp_phys++;
    for (i = 0; i < t->nheld; i++)
        if (t->held[i] == note)
            return;
    if (t->nheld < 16u) {

        uint8_t *hk = held_key[trk_index(t)];
        hk[t->nheld] = in_key && song.grid != 1u && kb_engine(t, k) < 0
                           ? (uint8_t)(in_key | (uint32_t)(song.octave + 3) << 5) : 0u;
        held_dt[trk_index(t)][t->nheld] = (int8_t)(hk[t->nheld] ? (int32_t)note - kb_note[k] : 0);
        t->held[t->nheld++] = (uint8_t)note;
    }
    if (t->nheld == 1u) {
        t->arp_pos = 0xFFFFFFF;
        t->arp_idx = 0xFFFFFFFFu;
        arp_lv[trk_index(t)] = 100u << 8;
    }
    arp_seed[trk_index(t)] = rng();
}

static void arp_remove(track_t *t, uint32_t note)
{
    uint32_t i, k = 0;
    uint8_t *hk = held_key[trk_index(t)];
    int8_t *dt = held_dt[trk_index(t)];
    if (t->arp_phys)
        t->arp_phys--;
    if (arp_latch(t))
        return;
    for (i = 0; i < t->nheld; i++)
        if (t->held[i] != note) {
            hk[k] = hk[i];
            dt[k] = dt[i];
            t->held[k++] = t->held[i];
        }
    t->nheld = (uint8_t)k;
}

#define ND_NONE 0u
static void rec_note(track_t *t, uint32_t note, uint32_t vel);
static void rec_release(track_t *t, uint32_t note);

static int rec_armed(uint32_t i) { return FELUCCA_SIMPLE ? song.rec != 0u : ((song.rec >> i) & 1u) != 0u; }
static int rec_on(const track_t *t) { return rec_armed(trk_index(t)) && song.playing; }

#include "motion.c"

enum { AM_OFF, AM_UP, AM_DN, AM_UPDN, AM_RND, AM_ORD, AM_REPEAT, AM_DNUP, AM_UP8, AM_CONV, AM_DIVG, AM_PINKY,
       AM_THUMB, AM_WALK, AM_CHORD, AM_PAT };

static const uint8_t ARP_MODE[6] = {AM_UP, AM_DN, AM_UPDN, AM_ORD, AM_RND, AM_PAT};
static const uint8_t ARP_DIV[7] = {10, 3, 5, 2, 4, 1, 0};

static uint32_t arp_am(const track_t *t)
{
    if (FELUCCA_SIMPLE)
        return t->p[P_AHOLD] ? ARP_MODE[clamp(t->p[P_AMODE], 0, 5)] : AM_OFF;
    return (uint32_t)t->p[P_AMODE];
}

static void arp_forget(track_t *t)
{
    t->arp_note = t->arp_ch[0] = t->arp_ch[1] = t->arp_ch[2] = 0;
}

static __attribute__((noinline)) void arp_chord_off(track_t *t, int rec)
{
    uint32_t i;
    for (i = 0; i < 3u; i++)
        if (t->arp_ch[i]) {
            if (rec)
                rec_release(t, t->arp_ch[i]);
            trk_note_off(t, t->arp_ch[i]);
            t->arp_ch[i] = 0;
        }
}

static __attribute__((noinline)) void arp_chord(track_t *t, const uint32_t *list, uint32_t n)
{
    uint32_t i, nt, top = 0, o = t->arp_idx % (uint32_t)(t->p[P_AOCT] > 0 ? t->p[P_AOCT] : 1);
    if (n > 4u)
        n = 4u;
    for (i = 0; i < n; i++) {
        nt = (uint32_t)clamp((int32_t)list[i] + 12 * (int32_t)o, 0, 127);
        if (nt == 127u && top++)
            continue;
        if (!t->arp_note)
            t->arp_note = (uint8_t)nt;
        else
            t->arp_ch[i - 1u] = (uint8_t)nt;
        if (rec_on(t))
            rec_note(t, nt, 100);
        trk_note_on(t, nt, 100);
    }
}

static void arp_silence(track_t *t)
{
    if (t->arp_ch[0] | t->arp_ch[1] | t->arp_ch[2])
        arp_chord_off(t, 0);
    trk_note_off(t, t->arp_note);
    t->arp_note = 0;
}

static void arp_off(track_t *t)
{
    if (t->arp_ch[0] | t->arp_ch[1] | t->arp_ch[2])
        arp_chord_off(t, rec_on(t) && !FELUCCA_SIMPLE);
    trk_note_off(t, t->arp_note);
    if (rec_on(t) && !FELUCCA_SIMPLE)
        rec_release(t, t->arp_note);
    t->arp_note = 0;
}

static __attribute__((noinline)) uint32_t arp_pick(track_t *t, uint32_t *list, uint32_t len)
{
    uint32_t i = t->arp_idx, m = (uint32_t)arp_am(t), cyc, k, w;
    if (m == AM_UP8 && len < 64u)
        list[len++] = (uint32_t)clamp((int32_t)list[0] + 12 * t->p[P_AOCT], 0, 127);
    cyc = len > 1u ? 2u * len - 2u : 1u;
    k = i % cyc;
    switch (m) {
    case AM_DN:
        return len - 1u - i % len;
    case AM_UPDN:
        return k < len ? k : cyc - k;
    case AM_DNUP:
        return k < len ? len - 1u - k : k - (len - 1u);
    case AM_RND:
        return rng() % len;
    case AM_PAT: {
        uint8_t pm[64];
        uint32_t sd = arp_seed[trk_index(t)];
        for (k = 0; k < len; k++)
            pm[k] = (uint8_t)k;
        for (k = len - 1u; k > 0u; k--) {
            uint8_t x;
            sd = sd * 1664525u + 1013904223u;
            w = (sd >> 8) % (k + 1u);
            x = pm[k];
            pm[k] = pm[w];
            pm[w] = x;
        }
        return pm[i % len];
    }
    case AM_CONV:
    case AM_DIVG:
        k = i % len;
        if (m == AM_DIVG)
            k = len - 1u - k;
        return (k & 1u) ? len - 1u - k / 2u : k / 2u;
    case AM_PINKY:
        return len < 2u ? 0u : (k & 1u) ? len - 1u : k / 2u;
    case AM_THUMB:
        return len < 2u ? 0u : (k & 1u) ? 1u + k / 2u : 0u;
    case AM_WALK:
        w = t->arp_walk;
        if (!i)
            w = 0;
        else if (w >= len)
            w = len - 1u;
        else if (len > 1u)
            w = !w ? 1u : w == len - 1u ? w - 1u : (rng() >> 7) & 1u ? w + 1u : w - 1u;
        t->arp_walk = (uint8_t)w;
        return w;
    default:
        return i % len;
    }
}

static volatile uint32_t beat_pos, beat_n;

#define CLK_START 0xFFFFFFFFu
static uint32_t clk_pos = CLK_START, clk_step;
static volatile uint8_t cin_left, cin_total;
static volatile uint32_t cin_pos;
static uint8_t cin_flush;
static uint8_t cin_note[NTRK][4], cin_vel[NTRK][4], cin_n[NTRK], cin_up[NTRK];
static int seq_counting(void) { return cin_left != 0u; }
static uint32_t click_beat_len(void) { return 4u * div_samples(2); }
static int click_wanted(void)
{
    return click_mode == CLICK_ON || (click_mode == CLICK_REC && song.rec);
}

static uint32_t arp_list(const track_t *t, uint32_t *list, uint32_t *cnt)
{
    uint32_t i, j, n = 0, x, s, last = 0, len = 0, o;
    const uint8_t *hk = held_key[trk_index(t)];
    const int8_t *dt = held_dt[trk_index(t)];
    for (i = 0; i < t->nheld; i++) {
        x = t->held[i];
        if ((s = hk[i]) != 0u) {
            x = kb_pitch(t, (s & 31u) - 1u, (int32_t)(s >> 5) - 3);
            if (x == KB_SILENT)
                continue;
            x = (uint32_t)clamp((int32_t)x + dt[i], 0, 127);
        }
        last = x;
        for (j = 0; j < n && list[j] != x; j++)
            ;
        if (j == n)
            list[n++] = x;
    }
    *cnt = n;
    if (!t->p[P_AORDER] && arp_am(t) != AM_ORD)
        for (i = 1; i < n; i++)
            for (j = i; j > 0 && list[j - 1] > list[j]; j--) {
                x = list[j];
                list[j] = list[j - 1];
                list[j - 1] = x;
            }
    for (o = 0; o < (uint32_t)t->p[P_AOCT]; o++)
        for (i = 0; i < n && len < 64u; i++)
            list[len++] = clamp((int32_t)list[i] + 12 * (int32_t)o, 0, 127);
    if (n && arp_am(t) == AM_REPEAT) { list[0] = last; len = 1; }
    return len;
}

static __attribute__((noinline)) void arp_fire(track_t *t, uint32_t period)
{
    uint32_t cnt, list[64], len, j;
    if (!(len = arp_list(t, list, &cnt))) {
        if (t->arp_note)
            arp_off(t);
        return;
    }
    t->arp_idx++;
    j = arp_am(t) == AM_CHORD ? 0u : arp_pick(t, list, len);
    if (t->arp_note)
        arp_off(t);
    if (FELUCCA_SIMPLE) {
        uint32_t ti = trk_index(t), vel = arp_lv[ti] >> 8;
        arp_lv[ti] = (uint16_t)(arp_lv[ti] * (uint32_t)(200 - clamp(t->p[P_ASWING], 0, 100)) / 200u);
        if (!vel)
            return;
        t->arp_off = period * (uint32_t)t->p[P_AGATE] / 128u;
        t->arp_note = (uint8_t)list[j];
        trk_note_on(t, t->arp_note, vel);
        return;
    }
    if ((uint32_t)(rng() & 127u) <= (uint32_t)t->p[P_APROB]) {
        t->arp_off = period * (uint32_t)t->p[P_AGATE] / 128u;
        if (arp_am(t) == AM_CHORD) {
            arp_chord(t, list, cnt);
            return;
        }
        t->arp_note = (uint8_t)list[j];
        if (rec_on(t))
            rec_note(t, t->arp_note, 100);
        trk_note_on(t, t->arp_note, 100);
    }
}

static void arp_step(track_t *t, uint32_t n, uint32_t gate_n)
{
    uint32_t period;
    int32_t sw;
    if (t->arp_note) {
        if (t->arp_off <= gate_n)
            arp_off(t);
        else
            t->arp_off -= gate_n;
    }
    if (!arp_am(t) || !t->nheld) {
        if (!t->nheld && t->arp_note)
            arp_off(t);
        return;
    }
    period = div_samples(FELUCCA_SIMPLE ? ARP_DIV[clamp(t->p[P_ARATE], 0, 6)] : (uint32_t)t->p[P_ARATE]);
    sw = FELUCCA_SIMPLE ? 0 : t->p[P_ASWING] * (int32_t)period / 250;
    t->arp_pos += n;
    if (t->arp_pos < period + (uint32_t)((t->arp_idx & 1u) ? sw : -sw) && t->arp_pos != 0xFFFFFFF + n)
        return;
    t->arp_pos = 0;
    arp_fire(t, period);
}

static uint32_t step_samples(const track_t *t, uint32_t period, uint32_t idx)
{
    return swing_step_len(t, period, idx);
}

static __attribute__((noinline)) int32_t rec_nudge(int32_t off, uint32_t period)
{
    return (off * NUDGE_DIV + (off < 0 ? -(int32_t)period / 2 : (int32_t)period / 2)) / (int32_t)(period ? period : 1u);
}

static void rec_note(track_t *t, uint32_t note, uint32_t vel)
{
    uint32_t len = t->p[P_SLEN] > 0 ? (uint32_t)t->p[P_SLEN] : 1u, idx = t->seq_idx % len, k;
    uint32_t period = div_samples((uint32_t)t->p[P_SDIV]), cur = step_samples(t, period, t->seq_idx);
    uint32_t next = t->seq_pos > cur / 2u, sounds;
    int32_t off = rec_nudge(t->seq_pos >= cur ? 0 : next ? -(int32_t)(cur - t->seq_pos) : (int32_t)t->seq_pos, period);
    step_t *s;
    if (next)
        idx = (idx + 1u) % len;
    s = &t->step[idx];

    sounds = next ? t->nd_fired != idx + 1u : t->nd_late != ND_NONE;
    if (s->time != ST_NOTE || !s->n || (trk_vmode(t) != V_POLY && !(FELUCCA_SIMPLE && arp_am(t)))) {
        s->n = 0;
        s->flags = 0;
        s->vel = 0;
        step_set_nudge(s, FELUCCA_SIMPLE && t->p[P_SQNT] ? 0 : off);
    }
    for (k = 0; k < s->n && s->note[k] != note; k++)
        ;
    if (k == s->n) {
        if (s->n < 4u)
            s->n++;
        s->note[s->n - 1u] = (uint8_t)note;
    }
    s->time = ST_NOTE;
    if (vel > 110)
        s->flags |= SF_ACCENT;
    if (vel > s->vel)
        s->vel = (uint8_t)vel;
    t->seq_active = 1;
    if (sounds) {
        if (t->rskip_idx != idx)
            t->rskip_n = 0;
        t->rskip_idx = (uint8_t)idx;
        if (t->rskip_n < 4u)
            t->rskip[t->rskip_n++] = (uint8_t)note;
    }
    if (!t->rh_n || t->rh_start != idx) {
        t->rh_n = 0;
        t->rh_start = (uint8_t)idx;
        t->rh_ties = 0;
    }
    for (k = 0; k < t->rh_n && t->rh_note[k] != note; k++)
        ;
    if (k == t->rh_n && t->rh_n < 4u)
        t->rh_note[t->rh_n++] = (uint8_t)note;
}

static void rec_hold(track_t *t, uint32_t idx, uint32_t len)
{
    step_t *s;
    uint32_t k;
    if (!t->rh_n)
        return;
    if (!rec_armed(trk_index(t)) || t->rh_ties + 1u >= len) {
        t->rh_n = 0;
        return;
    }
    if (idx == t->rh_start)
        return;
    s = &t->step[idx];
    t->rh_bak = *s;
    t->rh_last = (uint8_t)idx;
    t->rh_ties++;
    for (k = 0; k < 4u; k++)
        s->note[k] = 0;
    s->n = 0;
    s->time = ST_TIE;
    s->flags = 0;
    s->vel = 0;
    s->hit = s->acc = 0;
}

static void rec_release(track_t *t, uint32_t note)
{
    uint32_t i, k = 0;
    for (i = 0; i < t->rh_n; i++)
        if (t->rh_note[i] != note)
            t->rh_note[k++] = t->rh_note[i];
    if (k == t->rh_n || (t->rh_n = (uint8_t)k))
        return;
    if (t->rh_ties && t->seq_idx == t->rh_last &&
        t->seq_pos < step_samples(t, div_samples((uint32_t)t->p[P_SDIV]), t->seq_idx) / 2u)
        t->step[t->rh_last] = t->rh_bak;
}

static __attribute__((noinline)) void cin_keep(track_t *t, uint32_t note, uint32_t vel)
{
    uint32_t i = trk_index(t), k;
    if (cin_left != 1u || cin_pos < click_beat_len() / 2u || !rec_armed(i) || arp_am(t))
        return;
    for (k = 0; k < cin_n[i] && cin_note[i][k] != note; k++)
        ;
    if (k == cin_n[i]) {
        if (k == 4u)
            return;
        cin_n[i]++;
    }
    cin_note[i][k] = (uint8_t)note;
    cin_vel[i][k] = (uint8_t)vel;
    cin_up[i] &= (uint8_t)~(1u << k);
}

static void input_on(track_t *t, uint32_t note, uint32_t vel)
{
    fol_key(t, note, 1);
    last_note = (uint8_t)note;
    if (cin_left)
        cin_keep(t, note, vel);
    if (rec_on(t) && (FELUCCA_SIMPLE || !arp_am(t)))
        rec_note(t, note, vel);
    if (arp_am(t))
        arp_add(t, note);
    else
        trk_note_on(t, note, vel);
}

static int midi_note_held(const track_t *t, uint32_t note);
static int midi_local_held(const track_t *t, uint32_t note);
static void input_off(track_t *t, uint32_t note)
{
    if (midi_note_held(t, note))
        return;
    if (cin_left) {
        uint32_t i = trk_index(t), k;
        for (k = 0; k < cin_n[i]; k++)
            if (cin_note[i][k] == note)
                cin_up[i] |= (uint8_t)(1u << k);
    }
    fol_key(t, note, 0);
    rec_release(t, note);
    arp_remove(t, note);
    trk_note_off(t, note);
}

static uint8_t kb_fx[27];
static __attribute__((noinline)) void perf_key_up(uint32_t k)
{
    uint32_t e, j;
    if (!kb_fx[k])
        return;
    e = kb_fx[k] - 1u;
    kb_fx[k] = 0;
    for (j = 0; j < 27u; j++)
        if (kb_fx[j] == e + 1u)
            return;
    perf_press(e, 0);
}

static uint32_t grid_chord(const track_t *t, uint8_t *out)
{
    uint32_t i = trk_index(t), n = 0;
    const machine_t *m = trk_machine(t);
    if (lastc[i % NTRK]) {
        for (; n < lastc[i % NTRK]; n++)
            out[n] = lastn[i % NTRK][n];
        return n;
    }
    out[0] = m->note;
    return 1;
}
static void key_on(uint32_t k, track_t *t)
{
    uint32_t n = FELUCCA_SIMPLE && kb_g[k] ? grid_chord(t, kb_chord[k]) : chord_build(t, kb_note[k], kb_chord[k]);
    uint32_t i, mc = trk_midi_ch(trk_index(t)), vel = kb_vel;
    kb_chn[k] = 0;
    in_key = (uint8_t)(k + 1u);
    for (i = 0; i < n; i++) {
        uint32_t x = kb_chord[k][i];
        if (midi_local_held(t, x))
            continue;
        input_on(t, x, vel);
        midi_out_event(0x09u | (0x90u | mc) << 8 | x << 16 | vel << 24);
    }
    in_key = 0;
    kb_chn[k] = (uint8_t)n;
    last_note = kb_note[k];
    if (!kb_g[k]) {
        uint32_t ti = trk_index(t) % NTRK, j;
        if (!lheld[ti])
            lastc[ti] = 0;
        for (i = 0; i < n; i++) {
            for (j = 0; j < lastc[ti] && lastn[ti][j] != kb_chord[k][i]; j++)
                ;
            if (j == lastc[ti] && lastc[ti] < 4u)
                lastn[ti][lastc[ti]++] = kb_chord[k][i];
        }
        lheld[ti]++;
        kb_lh[k] = 1;
    }
}

static void key_off(uint32_t k, track_t *t)
{
    if (kb_lh[k] && lheld[trk_index(t) % NTRK])
        lheld[trk_index(t) % NTRK]--;
    kb_lh[k] = 0;
    uint32_t n = kb_chn[k], i, mc = trk_midi_ch(trk_index(t));
    kb_chn[k] = 0;
    for (i = 0; i < n; i++) {
        uint32_t x = kb_chord[k][i];
        if (midi_local_held(t, x))
            continue;
        input_off(t, x);
        midi_out_event(0x08u | (0x80u | mc) << 8 | x << 16);
    }
}

#ifdef FM1_INPUT_LAT

static uint32_t kb_out_tick;
static __attribute__((noinline)) void kb_lat(uint32_t k)
{
    uint32_t t0 = fm1_in.note_t0[k], d = fm1_ticks() - t0, o = kb_out_tick - t0;
    fm1_in_stat.kb_n++;
    fm1_in_stat.kb_sum += d;
    fm1_in_stat.dac_sum += o;
    if (d > fm1_in_stat.kb_max)
        fm1_in_stat.kb_max = d;
    if (o > fm1_in_stat.dac_max)
        fm1_in_stat.dac_max = o;
}
#endif

static uint8_t kb_octup, mt_used, kb_mt[27], mt_prev[NTRK];

static uint8_t par_id = 0xFFu, kb_seqdn;
static int16_t par_lo, par_hi;
static __attribute__((noinline)) int par_key(uint32_t k, track_t *t)
{
    uint32_t tk = trk_index(t), id = par_id, len, idx, period, b = 1u << (id % 32u);
    int16_t v = (int16_t)(par_lo + (int32_t)(par_hi - par_lo) * (int32_t)key_place(k) / 15);
    if (id >= MOTION_N)
        return 1;
    if (rec_on(t)) {
        len = t->p[P_SLEN] > 0 ? (uint32_t)t->p[P_SLEN] : 1u;
        period = div_samples((uint32_t)t->p[P_SDIV]);
        idx = t->seq_idx % len;
        if (t->seq_pos < 0x7FFFFFFFu && t->seq_pos > step_samples(t, period, t->seq_idx) / 2u)
            idx = (idx + 1u) % len;
        (void)motion_set_lock(t, idx, id, v);
        if (kb_octup && !(t->step[idx].time == ST_NOTE && t->step[idx].n))
            t->step[idx].time = ST_LOCK;
        if ((motion_base_valid >> tk) & 1u) {
            motion_active[tk][id / 32u] |= b;
            motion_locked[tk][id / 32u] |= b;
        }
    } else if ((motion_base_valid >> tk) & 1u) {
        motion_base[tk][id] = v;
    }
    *motion_vp(t, id) = v;
    if (kb_octup)
        mt_used = 1;
    return !kb_octup;
}

#define ACID_N 16u
enum { AG_MUTE, AG_FIRST, AG_EVERY, AG_HOLD, AG_N };
enum { AD_FWD, AD_REV, AD_PING, AD_RAND, AD_BRWN, AD_N };
#define AF_SLIDE 1u
#define AF_SKIP 2u
#define AF_ACC 4u
typedef struct { uint8_t pitch, oct, pulses, gate, flags, stime, rsv[2]; } acid_st_t;

typedef struct {
    uint8_t play, stages, dir, slide, gate, scale, root, rsv;
    acid_st_t st[ACID_N];
} acid_t;
static acid_t acid[NTRK];
static struct { uint32_t pos; uint16_t cnt; uint8_t go, st, pul, nx, back; } acid_rt[NTRK];
static uint8_t acid_sel[NTRK];
static void acid_reset(acid_t *a)
{
    uint32_t i;
    memset(a, 0, sizeof *a);
    a->stages = 8;
    a->slide = 40;
    a->gate = 64;
    for (i = 0; i < ACID_N; i++) {
        a->st[i].oct = 3;
        a->st[i].pulses = 1;
        a->st[i].gate = AG_FIRST;
        a->st[i].stime = 40;
    }
}
static acid_t *acid_of(uint32_t k)
{
    acid_t *a = &acid[k % NTRK];
    if (!a->stages || a->stages > ACID_N)
        acid_reset(a);
    return a;
}
static int acid_on(const track_t *t) { return FELUCCA_SIMPLE && acid[(uint32_t)(t - trk) % NTRK].play != 0u; }

static uint32_t acid_note(const acid_t *a, const acid_st_t *s)
{
    (void)a;
    return (uint32_t)clamp(60 + (s->pitch > 12u ? 12 : (int32_t)s->pitch) + ((int32_t)s->oct - 3) * 12, 0, 127);
}

static uint32_t acid_next(const acid_t *a, uint32_t k, uint32_t x)
{
    uint32_t n = a->stages ? a->stages : 1u, i;
    if (x >= n)
        x = n - 1u;
    for (i = 0; i < 2u * ACID_N + 2u; i++) {
        uint32_t r;
        switch (a->dir) {
        case AD_REV:
            x = (x + n - 1u) % n;
            break;
        case AD_PING:
            if (n == 1u)
                x = 0;
            else if (!acid_rt[k].back)
                x = x + 1u >= n ? (acid_rt[k].back = 1, x - 1u) : x + 1u;
            else
                x = x == 0u ? (acid_rt[k].back = 0, 1u) : x - 1u;
            break;
        case AD_RAND:
            x = rng() % n;
            break;
        case AD_BRWN:
            r = rng() & 3u;
            x = r < 2u ? (x + 1u) % n : r == 2u ? x : (x + n - 1u) % n;
            break;
        default:
            x = (x + 1u) % n;
            break;
        }
        if (!(a->st[x].flags & AF_SKIP))
            return x;
    }
    return x;
}

static uint32_t acid_key(uint32_t k, const track_t *t)
{
    uint32_t p = key_place(k);
    acid_t *a = acid_of((uint32_t)(t - trk));
    if (p >= ACID_N || (song.playing && a->play))
        return KB_SILENT;
    return acid_note(a, &a->st[p]);
}

#define KIT_N 8u
typedef struct { uint8_t n, id[KIT_N]; int16_t v[KIT_N]; } kit_key_t;
static kit_key_t kit[NTRK][16];
#define KIT_KEYS 8u
static struct { uint8_t n, id[KIT_N]; int16_t v[KIT_N], base[KIT_N]; } kit_snd[NTRK];
static uint8_t kit_cur[NTRK];
static void kit_undo(track_t *t)
{
    uint32_t tk = trk_index(t), i;
    for (i = 0; i < kit_snd[tk].n; i++) {
        int16_t *vp = motion_vp(t, kit_snd[tk].id[i]);
        if (*vp == kit_snd[tk].v[i])
            *vp = kit_snd[tk].base[i];
    }
    kit_snd[tk].n = 0;
    kit_cur[tk] = 0;
}

static int kit_base(const track_t *t, uint32_t id, int16_t *v)
{
    uint32_t tk = trk_index(t), i;
    for (i = 0; i < kit_snd[tk].n; i++)
        if (kit_snd[tk].id[i] == id) {
            *v = kit_snd[tk].base[i];
            return 1;
        }
    return 0;
}

static void kit_drop(track_t *t, uint32_t id)
{
    uint32_t tk = trk_index(t), i;
    for (i = 0; i < kit_snd[tk].n; i++)
        if (kit_snd[tk].id[i] == id) {
            int16_t *vp = motion_vp(t, id);
            if (*vp == kit_snd[tk].v[i])
                *vp = kit_snd[tk].base[i];
            kit_snd[tk].n--;
            kit_snd[tk].id[i] = kit_snd[tk].id[kit_snd[tk].n];
            kit_snd[tk].v[i] = kit_snd[tk].v[kit_snd[tk].n];
            kit_snd[tk].base[i] = kit_snd[tk].base[kit_snd[tk].n];
            return;
        }
}
static void kit_apply(track_t *t, uint32_t p)
{
    uint32_t tk = trk_index(t), i, id, b;
    const kit_key_t *s = &kit[tk][p & 15u];
    kit_undo(t);
    for (i = 0; i < s->n; i++) {
        id = s->id[i];
        b = 1u << (id % 32u);
        kit_snd[tk].id[i] = (uint8_t)id;
        kit_snd[tk].base[i] = *motion_vp(t, id);
        kit_snd[tk].v[i] = s->v[i];
        if ((motion_base_valid >> tk) & 1u) {
            motion_active[tk][id / 32u] |= b;
            motion_locked[tk][id / 32u] |= b;
        }
        *motion_vp(t, id) = s->v[i];
    }
    kit_snd[tk].n = s->n;
    kit_cur[tk] = (uint8_t)(p + 1u);
}
static __attribute__((noinline)) void kit_key(uint32_t k, track_t *t)
{
    uint32_t p = key_place(k) & 15u, len, period, idx, i;
    const kit_key_t *s = &kit[trk_index(t)][p];
    if (rec_on(t) && s->n) {
        len = t->p[P_SLEN] > 0 ? (uint32_t)t->p[P_SLEN] : 1u;
        period = div_samples((uint32_t)t->p[P_SDIV]);
        idx = t->seq_idx % len;
        if (t->seq_pos < 0x7FFFFFFFu && t->seq_pos > step_samples(t, period, t->seq_idx) / 2u)
            idx = (idx + 1u) % len;
        for (i = 0; i < s->n; i++)
            if (motion_set_lock(t, idx, s->id[i], s->v[i]) == 2)
                break;
    }
    kit_apply(t, p);
}

static uint32_t mt_t0[NTRK];
static void keyboard_block(void)
{
    uint32_t cur = fm1_in.notes, ch, k;
    uint32_t mute;
    ch = cur ^ kb_prev;
    if (!ch)
        return;
    if (kb_boot_hold) {
        for (k = 0; k < 27u; k++)
            if ((ch & cur) >> k & 1u)
                kb_note[k] = KB_SILENT;
        kb_prev = cur;
        return;
    }
    mute = song.grid == 4u || song.grid == 5u || song.grid == 9u || song.grid == 10u ? ~0u : 0u;

    for (k = 0; k < 27u; k++) {
        if (!((ch >> k) & 1u))
            continue;
        if ((cur >> k) & 1u) {
            kb_trk[k] = song.sel;
            if ((kb_eat >> k) & 1u) {
                kb_eat &= ~(1u << k);
                kb_note[k] = KB_SILENT;
                continue;
            }
            kb_g[k] = (uint8_t)(song.grid == 1u || song.grid == 7u || (song.grid == 6u && (key_black(k) || par_id != 0xFFu)) ||
                                (song.grid == 8u && key_black(k)));
            if ((song.grid == 1u || song.grid >= 6u) && key_black(k) && key_place(k) < NTRK)
                kb_trk[k] = (uint8_t)key_place(k);
            if (kb_asleep) {
                kb_note[k] = KB_SILENT;
                continue;
            }
            if (sc_keys && key_black(k) && key_place(k) <= NTRK) {
                uint32_t p = key_place(k);
                kb_note[k] = KB_SILENT;
                kb_layer |= 1u << k;
                if (song.playing) {
                    if (p < NTRK)
                        trk[p].p[P_MUTE] = (int16_t)!trk[p].p[P_MUTE];
                    else
                        fxl_mute = (uint8_t)!fxl_mute;
                } else
                    ph_masks[(uint32_t)ph_scene % 4u] = (int16_t)((uint32_t)ph_masks[(uint32_t)ph_scene % 4u] ^ (1u << (p < NTRK ? p : 6u)));
                continue;
            }
            if ((dj_keys || cb_keys) && key_black(k)) {
                uint32_t p = key_place(k), j, last = 0;
                for (j = 0; j < 27u; j++)
                    if (key_black(j) && key_place(j) > last)
                        last = key_place(j);
                if (p < NTRK || p == last) {
                    kb_note[k] = KB_SILENT;
                    kb_layer |= 1u << k;
                    if (cb_keys)
                        cb_sel = (uint8_t)(p < NTRK ? p : NTRK);
                    else
                        dj_sel = (uint8_t)(p < NTRK ? p : NTRK);
                    continue;
                }
            }
            if (kb_octup && key_black(k) && key_place(k) < NTRK && song.grid != 9u) {
                uint32_t tr = key_place(k);
                kb_note[k] = KB_SILENT;
                kb_layer |= 1u << k;
                kb_mt[k] = (uint8_t)(tr + 1u);
                mt_prev[tr] = (uint8_t)trk[tr].p[P_MUTE];
                mt_t0[tr] = fm1_ms;
                trk[tr].p[P_MUTE] = (int16_t)!mt_prev[tr];
                mt_used = 1;
                continue;
            }
            if (song.grid == 4u && key_black(k) && (key_place(k)==7u || key_place(k)==8u)) {
                uint32_t e=key_place(k)==7u?PF_R64:PF_STOP;
                kb_note[k]=KB_SILENT;kb_layer|=1u<<k;kb_fx[k]=(uint8_t)(e+1u);
                perf_press(e,1);continue;
            }
            if (song.grid == 4u && !key_black(k) && key_place(k) >= 12u && key_place(k) < 16u) {
                uint32_t e = key_place(k) - 12u;
                kb_note[k] = KB_SILENT;
                kb_layer |= 1u << k;
                kb_fx[k] = (uint8_t)(e + 1u);
                perf_press(e, 1);
                continue;
            }
            if (song.grid == 6u && key_black(k) && kb_seqdn) {
                kb_note[k] = KB_SILENT;
                kb_layer |= 1u << k;
                continue;
            }
            if (song.grid == 6u && !key_black(k) && par_id != 0xFFu) {
                uint8_t c[4];
                kb_note[k] = par_key(k, &trk[kb_trk[k]]) ? (grid_chord(&trk[kb_trk[k]], c), c[0]) : KB_SILENT;
                if (kb_note[k] == KB_SILENT) {
                    kb_layer |= 1u << k;
                    continue;
                }
            } else if (song.grid == 8u && !key_black(k)) {
                kb_note[k] = (uint8_t)acid_key(k, &trk[kb_trk[k]]);
                if (kb_note[k] == KB_SILENT) {
                    kb_layer |= 1u << k;
                    continue;
                }
            } else if (song.grid == 7u && !key_black(k) && key_place(k) >= KIT_KEYS) {
                kb_note[k] = KB_SILENT;
                kb_layer |= 1u << k;
                continue;
            } else if (song.grid == 7u && !key_black(k)) {
                uint8_t c[4];
                kit_key(k, &trk[kb_trk[k]]);
                grid_chord(&trk[kb_trk[k]], c);
                kb_note[k] = c[0];
            } else if ((mute >> k) & 1u)
                kb_note[k] = KB_SILENT;
            else if (song.grid == 1u || (song.grid >= 6u && key_black(k)))
                kb_note[k] = grid_key_note(k);
            else
                kb_note[k] = (uint8_t)kb_map(&trk[kb_trk[k]], k);
            if (kb_note[k] == KB_SILENT)
                continue;
            key_on(k, &trk[kb_trk[k]]);
#ifdef FM1_INPUT_LAT
            kb_lat(k);
#endif
        } else {
            kb_eat &= ~(1u << k);
            if ((kb_layer >> k) & 1u) {
                kb_layer &= ~(1u << k);
                if (kb_mt[k]) {
                    uint32_t tr = kb_mt[k] - 1u;
                    kb_mt[k] = 0;
                    if (fm1_ms - mt_t0[tr] > 350u)
                        trk[tr].p[P_MUTE] = (int16_t)mt_prev[tr];
                    continue;
                }
                perf_key_up(k);
                continue;
            }
            if (kb_note[k] == KB_SILENT)
                continue;
            key_off(k, &trk[kb_trk[k] % NTRK]);
        }
    }
    kb_prev = cur;
}

static void seq_start(void)
{
    uint32_t i;
    motion_begin();
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        t->seq_idx = (uint16_t)(t->p[P_SLEN] - 1);
        t->seq_pos = 0x7FFFFFFF;
        t->rskip_n = 0;
        t->rh_n = 0;
        t->rat_left = 0;
        t->nd_late = t->nd_early = ND_NONE;
        t->nd_fired = 0;
        acid_rt[i].go = 0;
        if (t->p[P_LTRIG] && t->p[P_LSYNC])
            t->lfo_ph = (uint32_t)t->p[P_LPHASE] << 25;
    }
    song.tick = 0;
    beat_pos = 0; beat_n = 0;
    fxlfo_reset();
    clk_pos = CLK_START;
    mg_ov_n = 0;
    song.playing = 1;
    perf_start();
}

static void seq_note_on(track_t *t, uint32_t note, uint32_t vel)
{
    if (FELUCCA_SIMPLE && arp_am(t)) {
        uint32_t ti = trk_index(t) % NTRK, idx = t->arp_idx, sd = arp_seed[ti];
        arp_add(t, note);
        if (arp_walk[ti]) {
            t->arp_idx = idx;
            arp_seed[ti] = sd;
        }
        arp_walk[ti] = 1;
    } else {
        trk_note_on(t, note, vel);
    }
}
static void seq_note_off(track_t *t, uint32_t note)
{
    if (FELUCCA_SIMPLE)
        arp_remove(t, note);
    trk_note_off(t, note);
}
static void seq_release(track_t *t)
{
    uint32_t i;
    for (i = 0; i < t->seq_n; i++)
        seq_note_off(t, t->seq_notes[i]);
    t->seq_n = 0;
    t->seq_hold = 0;
    t->slide_glide = 0;
}

static void seq_stop(void)
{
    uint32_t i;
    song.playing = 0;
    mg_ov_n = 0;
    cin_left = 0;
    cin_flush = 0;
    memset(cin_n, 0, sizeof cin_n);
    if (song.g[G_CLOCK])
        for (i = 0; i < NPART; i++)
            if (trk[i].arp_note)
                arp_off(&trk[i]);
    for (i = 0; i < NTRK; i++) {
        seq_release(&trk[i]);
        fol_clear_source(i);
        trk[i].rh_n = 0;
        trk[i].rat_left = 0;
        trk[i].nd_late = trk[i].nd_early = ND_NONE;
        trk[i].nd_fired = 0;
    }
    motion_end();
}

#define COND_LOOPS 12u
static volatile uint8_t seq_fill;
static const uint8_t COND_A[9] = {1, 2, 1, 2, 3, 1, 2, 3, 4}, COND_B[9] = {2, 2, 3, 3, 3, 4, 4, 4, 4};
static __attribute__((noinline)) int seq_cond_miss(const track_t *t, const step_t *s, uint32_t idx)
{
    uint32_t c = step_cond(s), loop;
    if (c >= COND_FILL)
        return (c == COND_FILL) != (seq_fill != 0u);
    if (!c)
        return 0;
    loop = t->cond_loop + (idx < t->seq_idx);
    return loop % COND_B[c - 1u] != COND_A[c - 1u] - 1u;
}

static __attribute__((noinline)) int gb_tc_miss(const track_t *tc, uint32_t c, uint32_t idx)
{
    track_t *t = &trk[trk_index(tc)];
    uint32_t loop = t->gb_loop + (idx < t->seq_idx), a, b, play, tk = trk_index(t);
    if (c <= GB_NAB) {
        gb_tc_ab(c, &a, &b);
        play = loop % b == a - 1u;
    } else {
        c -= GB_TC_FILL;
        play = c < 2u ? seq_fill != 0u : c < 4u ? t->tc_last != 0u : c < 6u ? tk && trk[tk - 1u].tc_last : loop == 0u;
        play ^= c & 1u;
        if (c == 2u || c == 3u || c == 4u || c == 5u)
            return !play;
    }
    t->tc_last = (uint8_t)play;
    return !play;
}

static int seq_miss(const track_t *t, const step_t *s, uint32_t idx)
{
    uint32_t c;
    if (s->probability >= COND_FIRST && seq_cond_miss(t, s, idx))
        return 1;
    if (gb_tc(s) && gb_tc_miss(t, gb_tc(s), idx))
        return 1;
    c = s->probability && s->probability < COND_FIRST ? step_chance(s) : (uint32_t)t->p[P_APROB];
    return c < 100u && rng() % 100u >= c;
    if (s->probability >= COND_FIRST)
        return seq_cond_miss(t, s, idx);
    return step_chance(s) < 100u && rng() % 100u >= step_chance(s);
}
#define SEQ_REP (1u << 16)
#define SEQ_ROLLED (1u << 17)
#define SEQ_MISS (1u << 18)
static __attribute__((noinline)) void seq_step(track_t *t, const step_t *s, uint32_t idx, uint32_t period, uint32_t skip)
{
    uint32_t i, j, gate = period * (uint32_t)t->p[P_SGATE] / 128u;
    uint32_t vel = (s->flags & SF_ACCENT) ? 127u : (s->vel ? s->vel : 96u);
    uint32_t slide_in = t->seq_hold && t->seq_n && !(skip & SEQ_REP);
    uint32_t len = t->p[P_SLEN] ? (uint32_t)t->p[P_SLEN] : 1u;
    uint32_t next_tie = t->step[(idx + 1u) % len].time == ST_TIE;
    uint32_t hits = step_ratchet(s);
    uint8_t nn[4 + NLANE], vv[4 + NLANE];
    uint32_t m = 0, sk = 0;
    int32_t rstair;

    const engine_t *e = ENGINES[eng_idx(t->eng_req)];
    uint32_t qseq = t->p[P_QUANT] >= QN_SEQ && !(e->keys && e->keys(t, 0) >= 0);
    int32_t qtrn = qseq && t->p[P_QUANT] == QN_SEQT ? KB_TRN(t) : 0;
    if (!(skip & SEQ_REP)) {
        t->rat_left = 0;
        if ((skip & SEQ_MISS) || (!(skip & SEQ_ROLLED) && seq_miss(t, s, idx))) {
            seq_release(t);
            return;
        }
    }
    if (s->time == ST_TIE) {
        if (t->seq_n) {
            t->seq_off = gate + period / 2u;
            t->seq_hold = (s->flags & SF_SLIDE) != 0 || next_tie;
        }
        return;
    }
    if (s->time >= ST_REST || (!s->n && !s->hit)) {
        seq_release(t);
        return;
    }
    if (FELUCCA_SIMPLE && arp_am(t))
        gate = step_samples(t, period, idx) - 1u;
    if (hits > 1u) {
        gate = step_samples(t, period, idx) / hits * (uint32_t)t->p[P_SGATE] / 128u;
        if (!(skip & SEQ_REP))
            t->rat_left = (uint8_t)(hits - 1u);
    }
    {
        uint32_t k = (skip & SEQ_REP) ? hits - 1u - t->rat_left : 0u, ramp = step_ramp(s);
        if (hits > 1u && ramp != RAMP_FLAT && !(s->flags & SF_ACCENT)) {
            uint32_t pos = ramp == RAMP_UP ? k : hits - 1u - k;
            vel = vel * (64u + 64u * pos / (hits - 1u)) / 128u;
        }
        if (hits > 1u && (s->roll & SR_LAST) && k == hits - 1u)
            vel = 127u;
        rstair = hits > 1u ? step_stair(s) * (int32_t)k : 0;
        if (vel < 1u)
            vel = 1u;
    }
    uint8_t generated[4];uint32_t gn=gen_live_notes(t,s,idx,generated);
    const uint8_t *notes=gn?generated:s->note;
    uint32_t note_count=gn?gn:s->n;
    for (i = 0; i < note_count && i < 4u; i++) {
        uint32_t x = (uint32_t)clamp((int32_t)notes[i] + rstair, 0, 127);
        if (qseq) {
            x = (uint32_t)clamp(scale_snap(t, (int32_t)x + qtrn), 0, 127);
            for (j = 0; j < m && nn[j] != x; j++)
                ;
            if (j < m)
                continue;
        }
        nn[m] = (uint8_t)x;
        vv[m] = (uint8_t)vel;
        sk |= ((skip >> i) & 1u) << m;
        m++;
    }
    fol_step(t, nn, m);
    t->slide_glide = (uint8_t)slide_in;
    if (!slide_in)
        seq_release(t);
    for (i = 0; i < m; i++)
        if (!((sk >> i) & 1u))
            seq_note_on(t, nn[i], vv[i]);
    if (slide_in)
        for (i = 0; i < t->seq_n; i++) {
            for (j = 0; j < m && nn[j] != t->seq_notes[i]; j++)
                ;
            if (j == m)
                seq_note_off(t, t->seq_notes[i]);
        }
    t->seq_n = 0;
    for (i = 0; i < m; i++)
        if (!((sk >> i) & 1u))
            t->seq_notes[t->seq_n++] = nn[i];
    t->seq_off = gate;
    t->seq_hold = hits == 1u && ((s->flags & SF_SLIDE) != 0 || next_tie);
}

static volatile uint16_t aud_req;
static __attribute__((noinline)) void seq_audition(void)
{
    uint32_t r = aud_req, i = (r & 0xFFu) - 1u, idx = r >> 8;
    track_t *t;
    uint16_t at;
    aud_req = 0;
    if (song.playing || cin_left || i >= NTRK || idx >= NSTEP)
        return;
    t = &trk[i];
    at = t->seq_idx;
    t->seq_idx = (uint16_t)idx;
    seq_step(t, &t->step[idx], idx, div_samples(2), SEQ_ROLLED);
    t->seq_idx = at;
    t->seq_hold = 0;
    t->rat_left = 0;
}

static __attribute__((noinline)) void seq_ratchet(track_t *t, uint32_t period)
{
    const step_t *s = &t->step[t->seq_idx];
    uint32_t hits = step_ratchet(s);
    if (t->rat_left >= hits) {
        t->rat_left = 0;
        return;
    }
    if (step_trip(s)) {
        if (t->rat_left != 2u || t->seq_pos < step_samples(t, period, t->seq_idx) * 2u / 3u)
            return;
        t->rat_left--;
        seq_step(t, s, t->seq_idx, period, SEQ_REP);
        return;
    }
    if (t->seq_pos < (hits - t->rat_left) * (step_samples(t, period, t->seq_idx) / hits))
        return;
    t->rat_left--;
    seq_step(t, s, t->seq_idx, period, SEQ_REP);
}

#define SEQ_QPLAY(t) (!FELUCCA_SIMPLE && (t)->p[P_SQNT])
static int32_t seq_nudge(const track_t *t, const step_t *s)
{
    if (SEQ_QPLAY(t) || s->time != ST_NOTE || (!s->n && !s->hit) || step_ratchet(s) != 1u)
        return 0;
    return step_nudge(s);
}

static __attribute__((noinline)) uint32_t seq_time(track_t *t, uint32_t idx, uint32_t period)
{
    const step_t *s = &t->step[idx];
    uint32_t vel = (s->flags & SF_ACCENT) ? 127u : (s->vel ? s->vel : 96u);
    if (s->time != ST_NOTE || !s->n || step_ratchet(s) != 1u)
        return 0;
    return mod_time(t, s->note[0], vel, period);
}

static __attribute__((noinline)) void seq_play(track_t *t, uint32_t idx, uint32_t period)
{
    const step_t *s = &t->step[idx];
    uint32_t skip = SEQ_ROLLED, i, k;
    if (seq_miss(t, s, idx))
        skip = SEQ_MISS;
    motion_step(t, idx, &motion, skip == SEQ_ROLLED, skip == SEQ_ROLLED && s->time == ST_NOTE && (s->n || s->hit));
    if (t->rskip_n && t->rskip_idx == idx) {
        for (k = 0; k < t->rskip_n; k++) {
            for (i = 0; i < s->n; i++)
                if (s->note[i] == t->rskip[k])
                    skip |= 1u << i;
        }
        t->rskip_n = 0;
    }
    seq_step(t, s, idx, period, skip);
}

static uint32_t seq_next(uint32_t idx, uint32_t len)
{
    uint32_t b = (uint32_t)seq_bar, base, w;
    idx = (idx + 1u) % len;
    if (!b || len <= 16u)
        return idx;
    base = (b - 1u) * 16u;
    if (base >= len)
        return idx;
    w = len - base < 16u ? len - base : 16u;
    return base + idx % 16u % w;
}

static __attribute__((noinline)) void seq_play_on(track_t *t, uint32_t period)
{
    uint32_t h = seq_time(t, t->seq_idx, period);
    if (h)
        t->nd_late = h + 1u;
    else
        seq_play(t, t->seq_idx, period);
}

static __attribute__((noinline)) void seq_nd_due(track_t *t, uint32_t period, uint32_t len, uint32_t cur_len, uint32_t n)
{
    if (t->nd_late != ND_NONE && (t->seq_pos >= t->nd_late - 1u || t->seq_pos >= cur_len)) {
        t->nd_late = ND_NONE;
        seq_play(t, t->seq_idx, period);
    }
    if (t->nd_early != ND_NONE && t->seq_pos >= t->nd_early - 1u && t->seq_pos != 0x7FFFFFFFu + n) {
        uint32_t nx = seq_next(t->seq_idx, len);
        t->nd_early = ND_NONE;
        t->nd_fired = (uint8_t)(nx + 1u);
        seq_play(t, nx, period);
    }
}

static __attribute__((noinline)) void seq_nd_start(track_t *t, uint32_t period, uint32_t len)
{
    uint32_t nx = seq_next(t->seq_idx, len);
    int32_t d;
    t->nd_early = ND_NONE;
    if (t->nd_fired == t->seq_idx + 1u) {
        ;
    } else {
        uint32_t h = seq_time(t, t->seq_idx, period);
        d = seq_nudge(t, &t->step[t->seq_idx]);
        if (d > 0 || h)
            t->nd_late = (uint32_t)(d > 0 ? d : 0) * period / NUDGE_DIV + h + 1u;
        else
            seq_play(t, t->seq_idx, period);
    }
    t->nd_fired = 0;
    if ((d = seq_nudge(t, &t->step[nx])) < 0) {
        uint32_t at = step_samples(t, period, t->seq_idx), back = (uint32_t)-d * period / NUDGE_DIV;
        at = (at > back ? at - back : 0u) + 1u;
        t->nd_early = at < t->nd_late ? t->nd_late : at;
    }
}

static __attribute__((noinline)) void seq_enter(track_t *t, uint32_t top, uint32_t len)
{
    if (!t->seq_idx) {
        t->cond_loop = (uint8_t)(top || t->cond_loop + 1u >= COND_LOOPS ? 0u : t->cond_loop + 1u);
        t->gb_loop = (uint16_t)(top || t->gb_loop + 1u >= GB_LOOPS ? 0u : t->gb_loop + 1u);
        if (top)
            t->tc_last = 0;
    }
    rec_hold(t, t->seq_idx, len);
}

static void acid_pulse(track_t *t, acid_t *a, uint32_t k, uint32_t pl)
{
    const acid_st_t *s = &a->st[acid_rt[k].st % ACID_N], *nx = &a->st[acid_rt[k].nx % ACID_N];
    uint32_t first = acid_rt[k].pul == 0u, last = acid_rt[k].pul + 1u >= s->pulses;
    uint32_t hold_out = (nx->flags & AF_SLIDE) && nx->gate != AG_MUTE && !(nx->flags & AF_SKIP);
    uint32_t g = (s->flags & AF_SKIP) ? AG_MUTE : s->gate, gate = pl * (a->gate ? a->gate : 1u) / 128u;
    uint32_t note, vel, slide_in, i;
    if (g == AG_MUTE || (g != AG_EVERY && !first)) {
        if (g == AG_HOLD && t->seq_n && last) {
            t->seq_hold = (uint8_t)hold_out;
            t->seq_off = gate;
        }
        if (g == AG_MUTE)
            seq_release(t);
        return;
    }
    note = acid_note(a, s);
    vel = (s->flags & AF_ACC) ? 127u : 96u;
    slide_in = (s->flags & AF_SLIDE) && first && t->seq_n && t->seq_hold;
    if (!slide_in)
        seq_release(t);
    trk_slide[k] = s->stime ? s->stime : 40u;
    t->slide_glide = (uint8_t)(slide_in ? 2u : 0u);
    seq_note_on(t, note, vel);
    for (i = 0; i < t->seq_n; i++)
        if (t->seq_notes[i] != note)
            seq_note_off(t, t->seq_notes[i]);
    t->seq_n = 1;
    t->seq_notes[0] = (uint8_t)note;
    t->seq_hold = (uint8_t)(g == AG_HOLD ? !last || hold_out : last && hold_out);
    t->seq_off = gate ? gate : 1u;
}

static void acid_tick(track_t *t, uint32_t n, uint32_t period)
{
    uint32_t k = (uint32_t)(t - trk) % NTRK;
    acid_t *a = acid_of(k);
    if (acid_rt[k].go == 1u) {
        uint32_t x = a->dir == AD_REV ? a->stages - 1u : 0u;
        acid_rt[k].go = 2;
        acid_rt[k].back = 0;
        if (a->st[x].flags & AF_SKIP)
            x = acid_next(a, k, x);
        acid_rt[k].st = (uint8_t)x;
        acid_rt[k].pul = 0;
        acid_rt[k].nx = (uint8_t)acid_next(a, k, x);
        acid_pulse(t, a, k, step_samples(t, period, acid_rt[k].cnt));
    } else {
        acid_rt[k].pos += n;
    }
    for (;;) {
        uint32_t pl = step_samples(t, period, acid_rt[k].cnt);
        const acid_st_t *s;
        if (acid_rt[k].pos < pl)
            break;
        acid_rt[k].pos -= pl;
        acid_rt[k].cnt++;
        s = &a->st[acid_rt[k].st % ACID_N];
        if (++acid_rt[k].pul >= s->pulses || acid_rt[k].st >= a->stages) {
            acid_rt[k].pul = 0;
            acid_rt[k].st = acid_rt[k].nx < a->stages ? acid_rt[k].nx : (uint8_t)acid_next(a, k, acid_rt[k].st);
            acid_rt[k].nx = (uint8_t)acid_next(a, k, acid_rt[k].st);
        }
        acid_pulse(t, a, k, step_samples(t, period, acid_rt[k].cnt));
    }
}

static uint32_t trip_at[NTRK];
static uint8_t trip_ix[NTRK];
static void seq_tick(track_t *t, uint32_t n)
{
    uint32_t period, len, top, k = (uint32_t)(t - trk) % NTRK, ac = acid_on(t), carry, cix;
    if (t->seq_n && !t->seq_hold) {
        if (t->seq_off <= n)
            seq_release(t);
        else
            t->seq_off -= n;
    }
    if (FELUCCA_SIMPLE && !ac && acid_rt[k].go) {
        acid_rt[k].go = 0;
        trk_slide[k] = 0;
        seq_release(t);
    }
    if (!song.playing) {
        trip_at[k] = 0;
        return;
    }
    period = div_samples((uint32_t)t->p[P_SDIV]);
    len = (uint32_t)t->p[P_SLEN];
    len = len ? len : 1u;
    t->seq_pos += n;
    for (;;) {
        uint32_t cur_len = step_samples(t, period, t->seq_idx);
        if (!ac && (t->nd_late | t->nd_early))
            seq_nd_due(t, period, len, cur_len, n);
        if (t->seq_pos < cur_len && t->seq_pos != 0x7FFFFFFFu + n)
            break;
        top = t->seq_pos >= 0x7FFFFFFFu;
        carry = !top && !ac && t->rat_left == 1u && step_trip(&t->step[t->seq_idx]) ? cur_len / 3u + 1u : 0u;
        cix = t->seq_idx;
        t->seq_pos = top ? 0u : t->seq_pos - cur_len;
        t->seq_idx = (uint16_t)seq_next(t->seq_idx, len);
        if (ac) {
            if (!acid_rt[k].go) {
                seq_release(t);
                acid_rt[k].go = 1;
                acid_rt[k].pos = t->seq_pos;
                acid_rt[k].cnt = t->seq_idx;
            }
            continue;
        }
        seq_enter(t, top, len);
        if (SEQ_QPLAY(t) && !t->nd_fired)
            seq_play_on(t, period);
        else
            seq_nd_start(t, period, len);
        trip_at[k] = carry;
        trip_ix[k] = (uint8_t)cix;
    }
    if (ac && acid_rt[k].go)
        acid_tick(t, n, period);
    else {
        if (trip_at[k] && t->seq_pos + 1u >= trip_at[k]) {
            uint8_t rl = t->rat_left;
            trip_at[k] = 0;
            t->rat_left = 0;
            seq_step(t, &t->step[trip_ix[k] % NSTEP], trip_ix[k], period, SEQ_REP);
            t->rat_left = rl;
        }
        if (t->rat_left)
            seq_ratchet(t, period);
    }
}

static uint8_t midi_route;

#define MIDI_ROUTE_AGAIN 0xFFu
static int32_t midi_slot(uint32_t ch)
{
    int32_t b = midi_base(), omni = -1;
    uint32_t i;
    if (b != -2)
        return b >= 0 && ch - (uint32_t)b < NPART ? (int32_t)(ch - (uint32_t)b) : -1;
    for (i = 0; i < NPART; i++) {
        uint32_t c = mch_get(i);
        if (c == ch)
            return (int32_t)i;
        if (c == MCH_OMNI && omni < 0)
            omni = (int32_t)i;
    }
    return omni;
}
static int midi_in_block(uint32_t ch) { return midi_slot(ch) >= 0; }
static track_t *midi_track(uint32_t ch)
{
    int32_t k = midi_slot(ch);
    return k >= 0 ? &trk[k] : TSEL;
}

#include "midi_control.c"
#include "midi_clock.c"

static __attribute__((noinline)) void cin_begin(void)
{
    cin_total = cin_left = (uint8_t)(4u * cin_bars);
    cin_pos = 0;
    memset(cin_n, 0, sizeof cin_n);
    click_req = 2;
}

static __attribute__((noinline)) void cin_block(uint32_t n)
{
    uint32_t b = click_beat_len();
    cin_pos += n;
    if (cin_pos < b)
        return;
    cin_pos -= b;
    cin_pos = cin_pos < b ? cin_pos : 0u;
    if (--cin_left) {
        click_req = (uint8_t)((cin_total - cin_left) % 4u ? 1u : 2u);
        return;
    }
    seq_start();
    cin_flush = 1;
}

static __attribute__((noinline)) void cin_record(void)
{
    uint32_t i, k;
    cin_flush = 0;
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        for (k = 0; k < cin_n[i]; k++)
            if (rec_on(t)) {
                rec_note(t, cin_note[i][k], cin_vel[i][k]);
                if ((cin_up[i] >> k) & 1u)
                    rec_release(t, cin_note[i][k]);
            }
        cin_n[i] = 0;
    }
}

#define FXN 64u
static struct { uint64_t trig; int16_t len; uint16_t lk[FXN]; int8_t v[FXN][MOTION_NG]; } fxl = {0, 16};
static uint32_t fxl_pos;
static uint32_t fxl_len(void) { return fxl.len >= 1 && fxl.len <= (int16_t)FXN ? (uint32_t)fxl.len : 16u; }
static void fxl_step(uint32_t s)
{
    uint32_t j, n = 0;
    s %= FXN;
    if (!((fxl.trig >> s) & 1u) || fxl_mute)
        return;
    for (j = 0; j < MOTION_NG; j++)
        if ((fxl.lk[s] >> j) & 1u) {
            mg_ov_g[n] = MOTION_G[j];
            mg_ov_v[n++] = fxl.v[s][j];
        }
    mg_ov_n = (uint8_t)n;
}

static uint32_t mst_step;

static uint8_t mst_len_was[NTRK], mst_div_was[NTRK];
static __attribute__((noinline)) void mst_follow(uint32_t n)
{
    uint32_t i;
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        uint32_t len = t->p[P_SLEN] ? (uint32_t)t->p[P_SLEN] : 1u, div = (uint32_t)t->p[P_SDIV] & 0xFFu;
        uint32_t ch = len != mst_len_was[i] || div != mst_div_was[i];
        mst_len_was[i] = (uint8_t)len;
        mst_div_was[i] = (uint8_t)div;
        if (!ch || !song.playing || clk_pos == CLK_START || t->seq_pos >= 0x7FFFFFFFu)
            continue;
        {
            uint32_t period = div_samples(div), s0 = step_samples(t, period, 0u), s1 = step_samples(t, period, 1u);
            uint64_t e = (uint64_t)mst_step * div_samples(2) + clk_pos;
            uint32_t k = (uint32_t)(e / (s0 + s1)) * 2u, r = (uint32_t)(e % (s0 + s1)), idx;
            if (r >= s0) {
                r -= s0;
                k++;
            }
            idx = k % len;
            if (idx == t->seq_idx && (r > t->seq_pos ? r - t->seq_pos : t->seq_pos - r) <= n)
                continue;
            if (r < n) {
                uint32_t pv = (idx + len - 1u) % len;
                t->seq_idx = (uint16_t)pv;
                t->seq_pos = step_samples(t, period, pv) + r;
            } else {
                t->seq_idx = (uint16_t)idx;
                t->seq_pos = r;
            }
            t->nd_late = t->nd_early = ND_NONE;
            t->nd_fired = 0;
            t->rat_left = 0;
        }
    }
}

static void sc_mask_set(uint32_t mask)
{
    uint32_t k;
    for (k = 0; k < NTRK; k++)
        trk[k].p[P_MUTE] = (int16_t)!((mask >> k) & 1u);
    fxl_mute = (uint8_t)((mask >> 6) & 1u);
}
static void sc_due(uint32_t n)
{
    int32_t m = sc_pend;
    if (m < 0 || !song.playing)
        return;
    if (clk_pos == CLK_START || (clk_step == 15u && clk_pos + n >= div_samples(2))) {
        sc_mask_set((uint32_t)m);
        sc_pend = -1;
    }
}

static void click_tick(uint32_t adv)
{
    uint32_t p = div_samples(2), beat = 0;
    if (clk_pos == CLK_START) {
        clk_pos = 0;
        clk_step = 0;
        mst_step = 0;
        memset(arp_walk, 0, sizeof arp_walk);
        beat = 2;
        fxl_pos = 0;
        fxl_step(0);
    } else {
        clk_pos += adv;
        while (clk_pos >= p) {
            clk_pos -= p;
            clk_step = (clk_step + 1u) & 15u;
            mst_step++;
            fxl_pos = mst_step % fxl_len();
            fxl_step(fxl_pos);
            if (!(clk_step & 3u))
                beat = clk_step ? 1u : 2u;
        }
    }
    if (beat && click_wanted())
        click_req = (uint8_t)beat;
}

static void events_block(uint32_t n)
{
    uint32_t i, pr, seq_n = n;
    uint32_t clock_mode = (uint32_t)song.g[G_CLOCK];
    if (midi_clock.mode != clock_mode) {
        seq_stop();
        memset(&midi_clock, 0, sizeof midi_clock);
        midi_clock.mode = (uint8_t)clock_mode;
        midi_beat_samples = 0;
    }
    if (cin_left && !clock_mode)
        cin_block(n);
    if (transport_req == 1u) {
        if (clock_mode)
            midi_clock_transport(0xFAu, fm1_ms);
        else if (cin_left)
            ;
        else if (cin_bars && song.rec && !song.playing)
            cin_begin();
        else
            seq_start();
        transport_req = 0;
    } else if (transport_req == 2u) {
        seq_stop();
        transport_req = 0;
    } else if (transport_req == 3u) {
        if (!clock_mode) {
            seq_stop();
            seq_start();
        }
        transport_req = 0;
    }
    if (midi_route != (uint8_t)song.g[G_ROUTE]) {
        midi_route = (uint8_t)song.g[G_ROUTE];
        if (midi_route != ROUTE_SEL)
            midi_route_block();
    }
    pr = panic_req;
    panic_req = 0;
    if (midi_in_overflow) {
        mi_r = mi_w;
        memset(midi_sel_on, 0, sizeof midi_sel_on);
        memset(midi_ch, 0, sizeof midi_ch);
        memset(midi_owners, 0, sizeof midi_owners);
        memset(mchord, 0, sizeof mchord);
        midi_hint = 0;
        for (i = 0; i < NTRK; i++) {
            trk[i].rh_n = trk[i].rskip_n = 0;
            trk[i].seq_n = trk[i].seq_hold = trk[i].slide_glide = trk[i].rat_left = 0;
        }
        pr |= (1u << NTRK) - 1u;
        RING_PUBLISH();
        midi_in_overflow = 0;
    }
    if (pr == PANIC_ALL) {
        uint32_t k;
        for (i = 0; i < NTRK; i++)
            for (k = 0; k < NVOICE; k++)
                if (trk[i].v[k].active)
                    voice_kill(&trk[i].v[k]);
        fx_panic();
    }
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        if ((pr >> i) & 1u) {
            midi_forget_track(i);
            trk_all_off(t);
            t->nheld = 0;
            t->arp_phys = 0;
            arp_forget(t);
        }
        engine_block(t);

        if ((t->armp && !arp_am(t)) || (t->aholdp && !arp_latch(t) && !t->arp_phys)) {
            t->nheld = 0;
            if (!arp_am(t))
                t->arp_phys = 0;
            if (t->arp_note)
                arp_silence(t);
        }
        t->armp = arp_am(t);
        t->aholdp = arp_latch(t);
    }
    keyboard_block();
    while (mi_r != mi_w) {
        uint32_t at = mi_r % MQ, pkt = midi_in_q[at], status = (pkt >> 8) & 0xFFu;
        uint32_t st = status & 0xF0u, ch = status & 0x0Fu;
        uint32_t d1 = (pkt >> 16) & 0x7Fu, d2 = (pkt >> 24) & 0x7Fu;
        uint32_t source = midi_in_source[at] ? midi_in_source[at] : 1u, ms = midi_in_ms[at];
        mi_r++;
        if (status >= 0xF8u) {
            if (clock_mode == source) {
                if (status == 0xF8u)
                    midi_clock_pulse(ms);
                else
                    midi_clock_transport(status, ms);
            }
        } else
            midi_event(st, ch, d1, d2);
    }
    if (clock_mode) {
        seq_n = 0;
        if (song.playing) {
            uint32_t last = midi_clock.have_pulse ? midi_clock.last_ms : midi_clock.start_ms;
            if (fm1_ms - last > 500u) {
                seq_stop();
                midi_clock.tempo_valid = 0;
            } else
                seq_n = midi_clock_advance(fm1_ms);
        }
    }
    if (aud_req)
        seq_audition();
#if FELUCCA_SIMPLE
    mst_follow(seq_n);
#endif
    sc_due(seq_n);
    {
        uint32_t ord = fol_order();
        for (i = 0; i < NTRK; i++, ord >>= 4)
            seq_tick(&trk[ord & 15u], seq_n);
    }
    if (cin_flush)
        cin_record();
    if (song.playing)
        click_tick(seq_n);
    for (i = 0; i < NPART; i++)
        arp_step(&trk[i], clock_mode ? seq_n : n, clock_mode && song.playing ? seq_n : n);
    beat_pos += clock_mode ? seq_n : n;
    if (beat_pos >= beat_samples()) {
        beat_pos -= beat_samples();
        beat_pos = beat_pos < beat_samples() ? beat_pos : 0u;
        beat_n = (beat_n + 1u) & 3u;
    }
    if (song.playing)
        song.tick++;
}

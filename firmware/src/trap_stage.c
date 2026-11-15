/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION contributors */

enum { SG_KILL, SG_ECHO, SG_VERB, SG_COMB, SG_SWEEP, SG_WASH, SG_RISE, SG_STUT, SG_N };
static const char *const SG_NAME[SG_N] = {"KILL", "ECHO", "VERB", "COMB", "SWEEP", "WASH", "RISE", "STUT"};
#define SG_MASTER NTRK
static uint8_t sg_tgt;
static uint8_t sg_keys, sg_lat, sg_was;
static uint8_t sg_fx_ch[SG_N];
static uint32_t sg_t0[SG_N];

static uint8_t sg_lvl_m, sg_dly_m, sg_rev_m, sg_dj_m, sg_cb_m;
static int16_t sg_lvl_b[NTRK], sg_dly_b[NTRK], sg_rev_b[NTRK], sg_dj_b[NTRK + 1];
static uint8_t sg_cb_sel;
static int16_t sg_cb_mix, sg_cb_tune;
static uint8_t sg_tune_m;
static uint8_t sg_shown, sg_sgrid;

static int sg_page(void)
{
    return !ui.home && !page_over && !ui.confirm && cur_page()->graph == GR_STAGE;
}
static uint32_t sg_mask(uint32_t ch) { return ch < NTRK ? 1u << ch : (1u << NTRK) - 1u; }
static int sg_active(uint32_t k)
{
    const track_t *t = &trk[k % NTRK];
    uint32_t i, len = t->p[P_SLEN] > 0 ? (uint32_t)t->p[P_SLEN] : 1u;
    for (i = 0; i < NVOICE; i++)
        if (t->v[i].active)
            return 1;
    for (i = 0; i < len && i < NSTEP; i++)
        if (t->step[i].time == ST_NOTE && t->step[i].n)
            return 1;
    return 0;
}
static uint32_t sg_white(uint32_t place)
{
    uint32_t k;
    for (k = 0; k < 27u; k++)
        if (!key_black(k) && key_place(k) == place)
            return (fm1_in.notes >> k) & 1u;
    return 0;
}
static int16_t *sg_djp(uint32_t ch, int16_t *tmp)
{
    if (ch >= NTRK)
        return &song.g[G_MIDI];
    *tmp = dj_trk[ch];
    return tmp;
}
static void sg_dj_set(uint32_t ch, int32_t v)
{
    v = clamp(v, -64, 63);
    if (ch >= NTRK)
        song.g[G_MIDI] = (int16_t)v;
    else
        dj_trk[ch] = (int8_t)v;
}
static int32_t sg_dj_get(uint32_t ch) { return ch >= NTRK ? song.g[G_MIDI] : dj_trk[ch % NTRK]; }

static void sg_ov(int16_t *p, int16_t *base, uint8_t *m, uint32_t bit, int want, int32_t val)
{
    if (want) {
        if (!(*m & bit)) {
            *base = *p;
            *m |= (uint8_t)bit;
        }
        *p = (int16_t)val;
    } else if (*m & bit) {
        *p = *base;
        *m &= (uint8_t)~bit;
    }
}
static uint32_t sg_bar_ms(void)
{
    uint32_t bpm = song.g[G_BPM] >= 40 ? (uint32_t)song.g[G_BPM] : 120u;
    return 240000u / bpm;
}
static uint32_t sg_ramp(uint32_t fx)
{
    uint32_t e = fm1_ms - sg_t0[fx], b = sg_bar_ms();
    return e >= b ? 1000u : e * 1000u / b;
}
static int sg_gate(uint32_t fx)
{
    if (song.playing) {
        uint32_t q = beat_samples() / 4u;
        return q ? beat_pos % q < q / 2u : 1;
    } else {
        uint32_t q = sg_bar_ms() / 16u;
        return q ? (fm1_ms - sg_t0[fx]) % q < q / 2u : 1;
    }
}

static int sg_later(uint32_t a, uint32_t b) { return (int32_t)(sg_t0[a] - sg_t0[b]) >= 0; }
static void sg_apply(uint32_t on)
{
    uint32_t k, lvl = 0, dly = 0, rev = 0, djw = 0, cbk = SG_N, sweep = (on >> SG_SWEEP) & 1u;
    uint8_t djk[NTRK + 1];
    if ((on >> SG_KILL) & 1u)
        lvl |= sg_mask(sg_fx_ch[SG_KILL]);
    if (((on >> SG_STUT) & 1u) && !sg_gate(SG_STUT))
        lvl |= sg_mask(sg_fx_ch[SG_STUT]);
    if ((on >> SG_ECHO) & 1u)
        dly |= sg_mask(sg_fx_ch[SG_ECHO]);
    if ((on >> SG_VERB) & 1u)
        rev |= sg_mask(sg_fx_ch[SG_VERB]);
    for (k = SG_WASH; k <= SG_RISE; k++)
        if ((on >> k) & 1u) {
            uint32_t c = sg_fx_ch[k] % (NTRK + 1u);
            if (!((djw >> c) & 1u) || sg_later(k, djk[c]))
                djk[c] = (uint8_t)k;
            djw |= 1u << c;
        }
    for (k = SG_COMB; k <= SG_SWEEP; k++)
        if (((on >> k) & 1u) && (cbk == SG_N || sg_later(k, cbk)))
            cbk = k;
    for (k = 0; k < NTRK; k++) {
        sg_ov(&trk[k].p[P_LEVEL], &sg_lvl_b[k], &sg_lvl_m, 1u << k, (lvl >> k) & 1u, 0);
        sg_ov(&trk[k].p[P_DLY], &sg_dly_b[k], &sg_dly_m, 1u << k, (dly >> k) & 1u, 127);
        sg_ov(&trk[k].p[P_REV], &sg_rev_b[k], &sg_rev_m, 1u << k, (rev >> k) & 1u, 127);
    }
    for (k = 0; k <= NTRK; k++) {
        int16_t tmp, *p = sg_djp(k, &tmp), was = *p;
        int32_t v = 0;
        if ((djw >> k) & 1u) {
            uint32_t r = sg_ramp(djk[k]);
            v = djk[k] == SG_WASH ? -(int32_t)(6u + r * 54u / 1000u) : (int32_t)(6u + r * 50u / 1000u);
        }
        sg_ov(p, &sg_dj_b[k], &sg_dj_m, 1u << k, (djw >> k) & 1u, v);
        if (k < NTRK && *p != was)
            sg_dj_set(k, *p);
    }
    if (cbk < SG_N) {
        if (!sg_cb_m) {
            sg_cb_m = 1;
            sg_cb_sel = cb_sel;
            sg_cb_mix = song.g[G_SYNC];
        }
        cb_sel = (uint8_t)(sg_fx_ch[cbk] < NTRK ? sg_fx_ch[cbk] : NTRK);
        song.g[G_SYNC] = 110;
    } else if (sg_cb_m) {
        sg_cb_m = 0;
        cb_sel = sg_cb_sel;
        song.g[G_SYNC] = sg_cb_mix;
    }
    if (sweep) {
        if (!sg_tune_m) {
            sg_tune_m = 1;
            sg_cb_tune = song.g[G_CRATE];
        }
        song.g[G_CRATE] = (int16_t)clamp(sg_cb_tune + (int32_t)(sg_ramp(SG_SWEEP) * 32u / 1000u), 0, 127);
    } else if (sg_tune_m) {
        sg_tune_m = 0;
        song.g[G_CRATE] = sg_cb_tune;
    }
}
static void sg_poll(void)
{
    uint32_t i, on, page = (uint32_t)sg_page();
    if (page != sg_shown) {
        if (page) {
            sg_sgrid = ui.sgrid;
            if (ui.sgrid == 1u || ui.sgrid == 2u)
                ui.sgrid = 0;
            sg_tgt = (uint8_t)(fxl_sel ? SG_MASTER : song.sel % NTRK);
        } else {
            sg_keys = 0;
            sg_lat = 0;
            if (ui.sgrid == 0u && (sg_sgrid == 1u || sg_sgrid == 2u))
                ui.sgrid = sg_sgrid;
        }
        sg_shown = (uint8_t)page;
    }
    for (i = 0; i < SG_N; i++)
        if (((sg_keys >> i) & 1u) && !sg_white(i))
            sg_keys &= (uint8_t)~(1u << i);
    on = sg_keys | sg_lat;
    for (i = 0; i < SG_N; i++)
        if (((on >> i) & 1u) && !((sg_was >> i) & 1u)) {
            sg_fx_ch[i] = sg_tgt;
            sg_t0[i] = fm1_ms;
        }
    if (on != sg_was)
        ui.force = 1;
    sg_was = (uint8_t)on;
    sg_apply(on);
    if (on & ((1u << SG_SWEEP) | (1u << SG_WASH) | (1u << SG_RISE) | (1u << SG_STUT)))
        ui.force = 1;
}

static void sg_press(uint32_t pressed)
{
    uint32_t k;
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k);
        if (!((pressed >> k) & 1u))
            continue;
        if (key_black(k)) {
            if (p < NTRK && !kb_octup)
                trk[p].p[P_MUTE] = (int16_t)!trk[p].p[P_MUTE];
            else if (p < NTRK) {
                mt_used = 1;
                sg_tgt = (uint8_t)p;
                track_select(p);
            } else if (p == NTRK)
                sg_tgt = SG_MASTER;
            else if (p <= NTRK + 4u)
                seq_bar = (int16_t)(seq_bar == (int16_t)(p - NTRK) ? 0 : p - NTRK);

        } else if (p < SG_N)
            sg_keys |= (uint8_t)(1u << p);
        else if (p < 2u * SG_N)
            sg_lat ^= (uint8_t)(1u << (p - SG_N));
        ui.force = 1;
    }
}
static int32_t accel(uint32_t role, int32_t s, int32_t range);
static int16_t *sg_dlyp(uint32_t k)
{
    k %= NTRK;
    return (sg_dly_m >> k) & 1u ? &sg_dly_b[k] : &trk[k].p[P_DLY];
}
static int32_t sg_dly_get(uint32_t ch)
{
    uint32_t k;
    int32_t s = 0;
    if (ch < NTRK)
        return *sg_dlyp(ch);
    for (k = 0; k < NTRK; k++)
        s += *sg_dlyp(k);
    return s / (int32_t)NTRK;
}
static void sg_knob(uint32_t slot, int32_t steps)
{
    static const uint8_t GK[2] = {G_CRATE, G_CDEPTH};
    if (slot == 0u) {
        uint32_t c = sg_tgt;
        int32_t v = (sg_dj_m >> c) & 1u ? sg_dj_b[c] : sg_dj_get(c);
        v = param_turn(&GP[G_MIDI], v, accel(EN_K1, steps, 127));
        if ((sg_dj_m >> c) & 1u)
            sg_dj_b[c] = (int16_t)v;
        else
            sg_dj_set(c, v);
    } else if (slot == 3u) {
        int32_t a = accel(EN_K4, steps, 127);
        uint32_t k;
        for (k = 0; k < NTRK; k++)
            if (sg_tgt >= NTRK || k == sg_tgt) {
                int16_t *p = sg_dlyp(k);
                *p = (int16_t)param_turn(&TP[P_DLY], *p, a);
            }
    } else {
        int16_t *gp = &song.g[GK[(slot - 1u) % 2u]];
        if (slot == 1u && sg_tune_m)
            gp = &sg_cb_tune;
        *gp = (int16_t)param_turn(&GP[GK[(slot - 1u) % 2u]], *gp, accel(EN_K1 + slot, steps, 127));
    }
    ui.hot_col = (uint8_t)slot;
    ui.hot_t = 30;
    ui.force = 1;
}
static uint32_t sg_leds(void)
{
    uint32_t k, m = 0, on = sg_keys | sg_lat;
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k);
        if (key_black(k) ? p == sg_tgt || (seq_bar && p == NTRK + (uint32_t)seq_bar) : p < SG_N ? (on >> p) & 1u : p < 2u * SG_N && ((sg_lat >> (p - SG_N)) & 1u))
            m |= 1u << k;
    }
    return m;
}

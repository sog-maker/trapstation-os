/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION contributors */

#if FELUCCA_WEB
#define HG_N 20u
typedef struct {
    const char *name;
    uint8_t bpm, swing;
    const char *bar1, *bar2, *roll1, *roll2;
} hg_preset_t;
static const hg_preset_t HG[HG_N] = {
    {"BASIC 8", 140, 0, "x.x.x.x.x.x.x.x.", "x.x.x.x.x.x.2.x.", 0, 0},
    {"END ROLL", 140, 0, "x.x.x.x.x.2.x.x.", "x.x.x.x.x.x.2.2.", 0, "............u.U."},
    {"TRIPLE 16", 140, 0, "x.x.xxx.x.x.x.x.", "x.x.xxx.x.x.2.x.", 0, 0},
    {"ROLL 48", 140, 0, "x.x.x.x.x.x.x.x.", "x.x.x.x.x.x.3.3.", 0, "............d.U."},
    {"ROLL 64", 140, 0, "x.x.x.x.x.x.x.x.", "x.x.x.x.x.x.x.4.", 0, "..............U."},
    {"STAIR UP", 140, 0, "x.x.x.2.x.x.x.x.", "x.x.x.x.x.x.4.2.", "......1.........", "............2.1."},
    {"STAIR DOWN", 140, 0, "x.x.x.x.x.x.2.x.", "x.x.x.x.x.x.4.3.", "............a...", "............D.b."},
    {"TRIPLET 16", 140, 0, "x.x.x.x.x.x.x.x.", "x.x.x.x.x.x.T.T.", 0, "............u.U."},
    {"TRIPLET 8", 140, 0, "x.x.x.x.x.x.x.x.", "x.x.x>.<x.x.x.x.", 0, 0},
    {"GALLOP", 140, 0, "x.x.x.x.x.x.x.x.", "22x.x.x.x.x.22x.", 0, 0},
    {"SNARE GAP", 140, 0, "x...x.x...x.x.x.", "x.x.x.x...x.2.2.", 0, "............u.u."},
    {"GHOST 16", 140, 0, "xgx.x.x.xgx.x.x.", "xgx.x.x.xgx2x.x.", 0, "...........d...."},
    {"DRILL BURST", 142, 0, "x.x.xx.x..x.xx..", "x.x.xx.x..3.x.2.", 0, "..........u...U."},
    {"DRILL TRIP", 142, 0, "x.x.x.x.T.x.x.x.", "T.x.x.x.T.x.T.T.", "........1.......", "1.......1...u.U."},
    {"MEMPHIS 70", 70, 0, "XgxgXgxgXgxgXgxg", "XgxgXgxgXgxg2g2g", 0, "............u.U."},
    {"MEMPHIS SHUF", 70, 30, "x.xgx.xgx.xgx.xg", "x.xgx.xgx.xg3.xg", 0, "............u..."},
    {"PHONK 155", 155, 0, "x.x.2.x.x.x.x.x.", "x.x.x.x.x.x.42xx", 0, "............uu.."},
    {"DRIFT RUSH", 160, 0, "x.xgx.xgx.xgx.xg", "2.xgx.2gx.xg4.2.", 0, "......u.....U.d."},
    {"LOFI SWING", 85, 35, "x.g.x.g.x.g.x.g.", "x.g.x.g.x.g.x>.<", 0, 0},
    {"DROP OUT", 140, 0, "x.x.g.g.g.g.g.g.", "g.g.x.x.4.......", 0, "........U......."},
};
_Static_assert(sizeof N_HGP / sizeof N_HGP[0] == HG_N, "a short name for each preset (params.c N_HGP)");

#endif
static int hg_is_hat(const track_t *t) { return trk_machine(t)->icon == IC_HATS; }

static uint32_t hg_target(void)
{
    uint32_t k;
    if (hg_is_hat(&trk[3]) && !acid_on(&trk[3]))
        return 3u;
    if (hg_is_hat(TSEL) && !acid_on(TSEL))
        return song.sel % NTRK;
    for (k = 0; k < NTRK; k++)
        if (hg_is_hat(&trk[k]) && !acid_on(&trk[k]))
            return k;
    return NTRK;
}

static char hg_hit(char c, uint32_t v)
{
    if (v & 2u)
        c = c == 'x' ? 'g' : c == 'g' ? 'x' : c;
    if (v & 4u)
        c = c == '2' ? '3' : c == '3' ? '4' : c == '4' ? '2' : c;
    return c;
}
static char hg_roll(char c, uint32_t v)
{
    if (v & 1u)
        c = c == 'u' ? 'd' : c == 'd' ? 'u' : c == 'U' ? 'D' : c == 'D' ? 'U' : c;
    if (v & 16u)
        c = c == '1' ? 'a' : c == '2' ? 'b' : c == '3' ? 'c' : c == 'a' ? '1' : c == 'b' ? '2' : c == 'c' ? '3' : c;
    return c;
}
static void hg_write(step_t *s, char c, char r, uint32_t note)
{
    int32_t stair = 0;
    uint32_t ramp = RAMP_FLAT;
    step_clear(s);
    if (c == '.')
        return;
    s->time = ST_NOTE;
    s->n = 1;
    s->note[0] = (uint8_t)note;
    s->vel = c == 'g' ? 60u : 100u;
    if (c == 'X')
        s->flags |= SF_ACCENT;
    if (c >= '2' && c <= '4')
        s->flags |= (uint8_t)((uint32_t)(c - '1') << SF_RATCH_SH);
    if (c == 'T') {
        s->flags |= (uint8_t)(2u << SF_RATCH_SH);
        s->roll |= SR_TRIP;
    }
    if (c == '<' || c == '>')
        step_set_nudge(s, c == '<' ? -5 : 5);
    if (r == 'u' || r == 'U')
        ramp = RAMP_UP;
    else if (r == 'd' || r == 'D')
        ramp = RAMP_DOWN;
    if (r >= '1' && r <= '3')
        stair = r - '0';
    else if (r >= 'a' && r <= 'c')
        stair = -(r - 'a' + 1);
    else if (r == 'U')
        stair = 1;
    else if (r == 'D')
        stair = -1;
    if (ramp != RAMP_FLAT || stair)
        step_set_roll(s, stair, ramp, 0);
}

#if FELUCCA_WEB

static void trap_hat_generate(track_t *t, uint32_t p, uint32_t v, uint32_t len)
{
    const hg_preset_t *g = &HG[p % HG_N];
    uint32_t i, note = trk_machine(t)->note;
    v = (v - 1u) % 20u;
    for (i = 0; i < NSTEP; i++) {
        uint32_t b2 = (i / 16u) & 1u, k = i % 16u, fill = !b2 && (v & 8u) && k >= 12u;
        const char *bar = b2 || fill ? g->bar2 : g->bar1, *rl = b2 || fill ? g->roll2 : g->roll1;
        if (i >= len) {
            step_clear(&t->step[i]);
            continue;
        }
        hg_write(&t->step[i], hg_hit(bar[k], v), rl ? hg_roll(rl[k], v) : '.', note);
    }
    t->p[P_SLEN] = (int16_t)len;
}

static int hg_nudged(const hg_preset_t *g)
{
    uint32_t k;
    for (k = 0; k < 16u; k++)
        if (g->bar1[k] == '<' || g->bar1[k] == '>' || g->bar2[k] == '<' || g->bar2[k] == '>')
            return 1;
    return 0;
}

#endif

static void hg_rbar(char *hit, char *roll, int fill)
{
    static const uint8_t AT[7] = {2, 3, 6, 7, 10, 14, 15}, W[7] = {2, 3, 2, 3, 2, 4, 3};
    static const char RL[8] = {'.', '.', 'u', 'u', '1', 'U', 'd', 'a'};
    uint32_t n = fill ? 2u + tr_rnd(2u) : 1u + tr_rnd(2u), k, tries;
    for (k = 0; k < n; k++)
        for (tries = 0; tries < 8u; tries++) {
            uint32_t at = fill && k == 0u ? 12u + tr_rnd(4u) : tr_pick(AT, W, 7u), t = tr_rnd(10u);
            char c = t < 4u ? '2' : t < 6u ? '3' : t < 8u ? '4' : 'T';
            if (hit[at] == '2' || hit[at] == '3' || hit[at] == '4' || hit[at] == 'T' || (at && hit[at - 1u] == 'T'))
                continue;
            if (c == 'T' && (at & 1u || at >= 15u))
                c = '3';
            hit[at] = c;
            roll[at] = RL[tr_rnd(8u)];
            if (c == 'T')
                hit[at + 1u] = '.', roll[at + 1u] = '.';
            break;
        }
}
static void hg_random(track_t *t, uint32_t len)
{
    char hit[3][17], roll[3][17];
    uint32_t i, k, hk = tr_rnd(3u), note = trk_machine(t)->note;
    for (k = 0; k < 16u; k++) {
        hit[0][k] = hk == 1u ? (k & 1u ? 'g' : 'x') : (k & 1u ? '.' : (k & 2u ? 'g' : 'x'));
        roll[0][k] = '.';
    }
    hit[0][16] = roll[0][16] = 0;
    if (hk == 2u)
        for (k = 0; k < 4u; k++)
            if (tr_pct(40u))
                hit[0][k * 4u + 3u] = 'g';
    if (tr_pct(20u))
        hit[0][0] = 'X';
    hg_rbar(hit[0], roll[0], 0);
    for (i = 1; i < 3u; i++) {
        memcpy(hit[i], hit[0], 17);
        memcpy(roll[i], roll[0], 17);
        hg_rbar(hit[i], roll[i], 1);
    }
    for (i = 0; i < NSTEP; i++) {
        uint32_t b = (i / 16u) % 4u, w = b == 0u || b == 2u ? 0u : b == 1u ? 1u : 2u;
        if (i >= len) {
            step_clear(&t->step[i]);
            continue;
        }
        hg_write(&t->step[i], hit[w][i % 16u], roll[w][i % 16u], note);
    }
    t->p[P_SLEN] = (int16_t)len;
}

static step_t hg_undo_step[NSTEP];
static int16_t hg_undo_len, hg_undo_swing, hg_undo_qnt;
static uint8_t hg_undo_trk;
static int hg_undo(void)
{
    track_t *t;
    if (!hg_undo_trk)
        return 0;
    t = &trk[(hg_undo_trk - 1u) % NTRK];
    fm1_irq_off();
    memcpy(t->step, hg_undo_step, sizeof hg_undo_step);
    t->p[P_SLEN] = hg_undo_len;
    t->p[P_SSWING] = hg_undo_swing;
    t->p[P_SQNT] = hg_undo_qnt;
    fm1_irq_on();
    hg_undo_trk = 0;
    return 1;
}
static void trap_hat_make(track_t *t)
{
    int32_t l = t->p[P_SLEN];
    memcpy(hg_undo_step, t->step, sizeof hg_undo_step);
    hg_undo_len = t->p[P_SLEN];
    hg_undo_swing = t->p[P_SSWING];
    hg_undo_qnt = t->p[P_SQNT];
    hg_undo_trk = (uint8_t)(trk_index(t) + 1u);
    tr_seed();
    fm1_irq_off();
    hg_random(t, l > 32 ? 64u : l > 16 ? 32u : 16u);
    tr_mix=100;
    fm1_irq_on();
    lastc[trk_index(t)] = 0;
}

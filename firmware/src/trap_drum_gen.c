/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION contributors */

#define TG_ROWS 5u
static void tg_write(step_t *s, char c, int32_t note)
{
    step_clear(s);
    if (c == '.')
        return;
    s->time = ST_NOTE;
    s->n = 1;
    s->note[0] = (uint8_t)clamp(note, 0, 127);
    s->vel = c == 'g' ? 60u : 100u;
    if (c == 'X')
        s->flags |= SF_ACCENT;
    if (c >= '2' && c <= '4')
        s->flags |= (uint8_t)((uint32_t)(c - '1') << SF_RATCH_SH);
    if (c == 'r' || c == 'u' || c == 'R') {
        s->flags |= (uint8_t)((c == 'r' ? 1u : c == 'u' ? 2u : 3u) << SF_RATCH_SH);
        step_set_roll(s, 0, RAMP_UP, 0);
    }
    if (c == 'T') {
        s->flags |= (uint8_t)(2u << SF_RATCH_SH);
        s->roll |= SR_TRIP;
    }
}

#if FELUCCA_WEB
#define TG_N 20u
typedef struct {
    const char *name;
    uint8_t bpm, swing;
    const char *bar1[TG_ROWS], *bar2[TG_ROWS];
    const char *note1[2], *note5[2];
} tg_preset_t;
#define H8 "x.g.x.g.x.g.x.g."
#define NONE "................"
#define S9 "........x......."

static const tg_preset_t TG[TG_N] = {
    {"MEMPHIS 70", 70, 0,
     {"x......x..x.....", "....x.......x...", "............x...", "xgxgxgxgxgxgxgxg", "..x...x...x...x."},
     {"x.........x.....", "....x.......x...", "............x...", "xgxgxgxgxgxgxg2g", "..x...x...x...X."},
     {0}, {0}},
    {"MEM COWBELL", 70, 0,
     {"x......x..x.....", "....x.......x...", NONE, NONE, "X.x.x.x.X.x.x.x."},
     {"x......x........", "....x.......x...", NONE, NONE, "X.x.x.x.X.x.x.xx"},
     {"0......3..5.....", "0......3........"}, {"0.3.5.3.7.5.3.0.", "0.3.5.3.7.5.3.05"}},
    {"MEMPHIS 140", 140, 0,
     {"x.........x.....", S9, S9, H8, "..x...x...x...x."},
     {"x.........x..x..", S9, S9, "x.g.x.g.x.g.x.2.", "..x...x...x...X."},
     {0}, {0}},
    {"TRAP BASIC", 140, 0,
     {"x.........x.....", S9, S9, H8, NONE},
     {"x......x..x.....", "........x.....g.", S9, "x.g.x.g.x.g.2...", "..............x."},
     {0}, {0}},
    {"TRAP 8THS", 130, 0,
     {"x...........x...", NONE, S9, "x.x.x.2xx.x.x.x.", NONE},
     {"....x...........", "............x.x.", S9, "x.x.x.2xx.x.x.x.", NONE},
     {0}, {0}},
    {"TRAP 16THS", 140, 0,
     {"x.........x.....", S9, S9, "xgxgxgxgxgxgxgxg", NONE},
     {"x......x..x.....", S9, S9, "xgxgxgxgxgxgrgRg", NONE},
     {0}, {0}},
    {"TRIPLET", 140, 0,
     {"x.........x.....", S9, S9, H8, NONE},
     {"x.........x.....", S9, S9, "x.g.x.g.x.g.T.T.", NONE},
     {0}, {0}},
    {"ROLL END", 140, 0,
     {"x.........x.....", S9, S9, H8, NONE},
     {"x.........x.....", S9, S9, "x.g.x.g.x.g.x.xu", NONE},
     {0}, {0}},
    {"ROLL 64", 140, 0,
     {"x.........x.....", S9, S9, "x.g.x.gRx.g.x.g.", NONE},
     {"x.........x.....", S9, S9, "x.g.x.g.x.g.x.gR", NONE},
     {0}, {0}},
    {"OPEN HAT", 140, 0,
     {"x.........x.....", S9, S9, "x.g.x...x.g.x...", "......x.......x."},
     {"x.........x.....", S9, S9, "x.g.x...x.g...x.", "......x.....x..."},
     {0}, {0}},
    {"808 BOUNCE", 140, 0,
     {"x..x..x...x.....", S9, S9, H8, NONE},
     {"x..x..x...x..x..", S9, S9, "x.g.x.g.x.g.2.g.", NONE},
     {"0..0..7...c.....", "0..0..7...5..c.."}, {0}},
    {"GHOST SNARE", 140, 0,
     {"x.........x.....", "........xg......", S9, H8, NONE},
     {"x.........x.....", ".......gxg.....g", S9, "x.g.x.g.x.g.x.2.", NONE},
     {0}, {0}},
    {"SNARE FILL", 140, 0,
     {"x.........x.....", S9, S9, H8, NONE},
     {"x.........x.....", "........x...rrrr", S9, "x.g.x.g.x.......", NONE},
     {0}, {0}},
    {"UK DRILL", 142, 0,
     {"x.....x...x.....", ".......gx.......", NONE, "x..g..T.x..g..x.", NONE},
     {"..x....x........", "............x...", NONE, "x..g..x.g..x.T..", NONE},
     {"0.....0...7.....", "..0....c........"}, {0}},
    {"UK DRILL B", 142, 0,
     {"x....x....x.....", "....x..g....x...", NONE, "x..x..x...x.x...", NONE},
     {"x....x....x..x..", "....x..g....x...", NONE, "x..x..x...x.xX2.", NONE},
     {"0....0....3.....", "0....0....3..7.."}, {0}},
    {"NY DRILL", 144, 0,
     {"x..x......x.....", S9, NONE, "x.g...x.g.T...x.", "..............x."},
     {"x..x....x..x....", "............x...", NONE, "x.g...x.g.x.T...", "..............x."},
     {"0..0......7.....", "0..0....3..c...."}, {0}},
    {"PHONK", 140, 15,
     {"x......x..x.....", S9, S9, H8, "X.x.x.x.X.x.x.x."},
     {"x.........x.....", S9, S9, "x.g.x.g.x.g.x.2.", "X.x.x.x.X.x.x.x."},
     {0}, {"0.3.0.7.0.3.0.5.", "0.3.0.7.0.3.5.3."}},
    {"DRIFT", 150, 0,
     {"x..x..x...x.....", S9, NONE, H8, "x.X.x.X.x.X.x.X."},
     {"x..x..x...x.....", S9, NONE, "x.g.x.g.x.g.2.2.", "x.X.x.X.x.X.x.X."},
     {"0..0..1...0.....", "0..0..B...0....."}, {"0.0.1.1.7.7.3.0.", "0.0.1.1.7.7.1.0."}},
    {"CLAP 2+4", 70, 0,
     {"x.........x.....", "....x.......x...", "............x...", H8, NONE},
     {"x......x........", "....x.......x...", "............x...", "x.g.x.g.x.g.x.2.", NONE},
     {0}, {0}},
    {"LOFI TRAP", 87, 15,
     {"x..g......x.....", "....x.......x...", NONE, H8, NONE},
     {"x..g......x.....", "....x.......x..g", NONE, H8, NONE},
     {0}, {0}},
};
static const char *const N_TGP[] = {"MEM70", "MEMCOW", "MEM140", "BASIC", "8THS", "16THS", "TRIPL", "ROLEND", "ROLL64",
                                    "OPENHT", "808BNC", "GHOST", "SNFILL", "UKDRL", "UKDRLB", "NYDRL", "PHONK", "DRIFT",
                                    "CLAP24", "LOFI"};
_Static_assert(sizeof N_TGP / sizeof N_TGP[0] == TG_N, "a short name for each preset");

static int32_t tg_semi(char c)
{
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'c' ? c - 'a' + 10 : c >= 'A' && c <= 'L' ? -(c - 'A' + 1) : 0;
}

static int tg_in(const char *set, char c)
{
    for (;; set++) {
        if (*set == c)
            return 1;
        if (!*set)
            return 0;
    }
}
static int tg_has(const char *row, const char *set)
{
    for (uint32_t k = 0; k < 16u; k++)
        if (tg_in(set, row[k]))
            return 1;
    return 0;
}
static void tg_ghost(char *row, const uint8_t *at, uint32_t n)
{
    for (uint32_t j = 0; j < n; j++)
        if (row[at[j]] == '.') {
            row[at[j]] = 'g';
            return;
        }
}

static void tg_rows(const tg_preset_t *g, uint32_t b, uint32_t v, char rw[TG_ROWS][17], char n1[17], char n5[17])
{
    static const uint8_t LATE[4] = {13, 14, 15, 5}, EARLY[3] = {3, 2, 6};
    uint32_t r, k, c = v / 4u, h;
    for (r = 0; r < TG_ROWS; r++) {
        const char *src = b && g->bar2[r] ? g->bar2[r] : g->bar1[r];
        memcpy(rw[r], src, 16); rw[r][16] = 0;
    }
    memcpy(n1, g->note1[b] ? g->note1[b] : "................", 16); n1[16] = 0;
    memcpy(n5, g->note5[b] ? g->note5[b] : "................", 16); n5[16] = 0;
    h = tg_has(rw[3], "xXg234ruRT") ? 3u : 4u;
    if (v & 1u)
        for (k = 0; k < 16u; k++)
            rw[h][k] = rw[h][k] == 'x' ? 'g' : rw[h][k] == 'g' ? 'x' : rw[h][k];
    if ((v & 2u) && rw[h][13] != 'T') {
        char *x = &rw[h][14];
        *x = *x == '2' ? '3' : *x == '3' ? '4' : *x == '4' ? '2' : *x == 'r' ? 'u' : *x == 'u' ? 'R' : *x == 'R' ? 'r' : *x == 'T' ? 'T' : '2';
    }
    if (c == 1u)
        tg_ghost(rw[0], LATE, 4);
    else if (c == 2u) {
        for (k = 15; k >= 8u; k--)
            if (rw[0][k] != '.' && rw[0][k - 1u] == '.') {
                rw[0][k - 1u] = rw[0][k]; rw[0][k] = '.';
                n1[k - 1u] = n1[k]; n1[k] = '.';
                break;
            }
        if (k < 8u) {
            static const uint8_t ALT[2] = {9, 6};
            tg_ghost(rw[0], ALT, 2);
        }
    } else if (c == 3u)
        tg_ghost(rw[0], EARLY, 3);
    else if (c == 4u) {
        uint32_t sr = tg_has(rw[1], "xX") ? 1u : 2u, main = 16u;
        for (k = 0; k < 16u; k++)
            if (rw[sr][k] == 'x' || rw[sr][k] == 'X') { main = k; break; }
        if (main < 16u) {
            uint8_t at[2] = {(uint8_t)(main + 1u < 16u ? main + 1u : main - 1u), (uint8_t)(main ? main - 1u : main + 1u)};
            tg_ghost(rw[sr], at, 2);
        }
    }
}

static void trap_drum_generate(uint32_t p, uint32_t v, uint32_t len)
{
    const tg_preset_t *g = &TG[p % TG_N];
    char rw[2][TG_ROWS][17], n1[2][17], n5[2][17];
    uint32_t r, i;
    v = (v - 1u) % 20u;
    tg_rows(g, 0, v, rw[0], n1[0], n5[0]);
    tg_rows(g, 1, v, rw[1], n1[1], n5[1]);
    for (r = 0; r < TG_ROWS; r++) {
        track_t *t = &trk[r];
        uint32_t base = trk_machine(t)->note;
        for (i = 0; i < NSTEP; i++) {
            step_t *s = &t->step[i];
            uint32_t bar2 = (i / 16u) & 1u, k = i % 16u;
            const char *nt = r == 0u ? n1[bar2] : r == 4u ? n5[bar2] : 0;
            char c;
            step_clear(s);
            if (i >= len)
                continue;
            c = rw[bar2][r][k];
            tg_write(s, c, (int32_t)base + (nt ? tg_semi(nt[k]) : 0));
        }
        t->p[P_SLEN] = (int16_t)len;
        lastc[r] = 0;
    }
}

#endif

static const int8_t TR_808[6] = {3, 5, 7, 10, 12, -2};
static void tr_kicks(char *row, int8_t *semi)
{
    static const uint8_t AT[9] = {2, 3, 6, 7, 10, 11, 13, 14, 15}, W[9] = {1, 2, 2, 4, 4, 3, 2, 2, 1};
    uint32_t n = 1u + tr_rnd(3u), k, tries;
    for (k = 1; k < 16u; k++)
        if (row[k] != '.') {
            row[k] = '.';
            semi[k] = 0;
        }
    for (k = 0; k < n; k++)
        for (tries = 0; tries < 8u; tries++) {
            uint32_t at = tr_pick(AT, W, 9u);
            if (row[at] == '.') {
                row[at] = tr_pct(20u) ? 'g' : 'x';
                semi[at] = tr_pct(35u) ? TR_808[tr_rnd(6u)] : 0;
                break;
            }
        }
}
static void tr_rolls(char *row, uint32_t n)
{
    static const uint8_t AT[7] = {2, 3, 6, 7, 10, 14, 15}, W[7] = {2, 3, 2, 3, 2, 4, 3};
    uint32_t k, tries;
    for (k = 0; k < n; k++)
        for (tries = 0; tries < 8u; tries++) {
            uint32_t at = tr_pick(AT, W, 7u), t = tr_rnd(10u);
            char c = t < 4u ? '2' : t < 6u ? '3' : t < 7u ? '4' : t < 9u ? 'T' : 'u';
            if (row[at] == '2' || row[at] == '3' || row[at] == '4' || row[at] == 'T' || row[at] == 'u')
                continue;
            if (c == 'T' && (at & 1u || at >= 15u))
                c = '3';
            if (at && row[at - 1u] == 'T')
                continue;
            row[at] = c;
            if (c == 'T')
                row[at + 1u] = '.';
            break;
        }
}
static void tr_bar(char rw[TG_ROWS][17], int8_t *semi, uint32_t hk)
{
    uint32_t r, k;
    for (r = 0; r < TG_ROWS; r++) {
        memset(rw[r], '.', 16);
        rw[r][16] = 0;
    }
    memset(semi, 0, 16);
    rw[0][0] = 'x';
    rw[1][8] = 'x';
    if (tr_pct(70u))
        rw[2][8] = 'x';
    tr_kicks(rw[0], semi);
    if (tr_pct(45u)) {
        static const uint8_t AT[4] = {6, 11, 14, 15}, W[4] = {2, 3, 2, 1};
        rw[1][tr_pick(AT, W, 4u)] = 'g';
    }
    for (k = 0; k < 16u; k++)
        rw[3][k] = hk == 1u ? (k & 1u ? 'g' : 'x') : (k & 1u ? '.' : (k & 2u ? 'g' : 'x'));
    if (hk == 2u)
        for (k = 0; k < 4u; k++)
            if (tr_pct(40u))
                rw[3][k * 4u + 3u] = 'g';
    if (tr_pct(15u))
        rw[3][0] = 'X';
    tr_rolls(rw[3], 1u + tr_rnd(2u));
    switch (tr_rnd(10u)) {
    case 0: case 1: case 2: case 3:
        break;
    case 4: case 5: case 6: case 7: {
        static const uint8_t AT[6] = {2, 5, 6, 10, 13, 14}, W[6] = {2, 2, 2, 2, 2, 2};
        uint32_t n = 2u + tr_rnd(2u);
        for (k = 0; k < n; k++)
            rw[4][tr_pick(AT, W, 6u)] = tr_pct(30u) ? 'g' : 'x';
        break;
    }
    default:
        for (k = 2; k < 16u; k += 4u)
            rw[4][k] = 'x';
        if (tr_pct(50u))
            rw[4][15] = 'g';
        break;
    }
}
static void tr_fill(char rw[TG_ROWS][17], int8_t *semi)
{
    uint32_t t = tr_rnd(4u);
    if (tr_pct(60u))
        tr_kicks(rw[0], semi);
    if (t == 0u) {
        rw[1][14] = '2';
        rw[1][15] = tr_pct(50u) ? '3' : '2';
    } else if (t == 1u) {
        rw[3][12] = '2';
        rw[3][13] = '.';
        rw[3][14] = tr_pct(50u) ? 'T' : '3';
        rw[3][15] = rw[3][14] == 'T' ? '.' : '4';
    } else if (t == 2u) {
        uint32_t k;
        for (k = 12; k < 16u; k++) {
            rw[0][k] = '.';
            rw[3][k] = k == 15u ? 'u' : '.';
        }
    } else
        tr_rolls(rw[3], 1u);
}

static void trap_drum_random(uint32_t seed, uint32_t len)
{
    char rw[3][TG_ROWS][17];
    int8_t semi[3][16];
    uint32_t hk, r, i;
    tr_seed_with(seed);
    hk = tr_rnd(3u);
    tr_bar(rw[0], semi[0], hk);
    memcpy(rw[1], rw[0], sizeof rw[0]);
    memcpy(semi[1], semi[0], sizeof semi[0]);
    tr_fill(rw[1], semi[1]);
    memcpy(rw[2], rw[0], sizeof rw[0]);
    memcpy(semi[2], semi[0], sizeof semi[0]);
    tr_fill(rw[2], semi[2]);
    for (r = 0; r < TG_ROWS; r++) {
        track_t *t = &trk[r];
        int32_t base = (int32_t)trk_machine(t)->note;
        for (i = 0; i < NSTEP; i++) {
            uint32_t b = (i / 16u) % 4u, k = i % 16u, w = b == 0u || b == 2u ? 0u : b == 1u ? 1u : 2u;
            if (i >= len) {
                step_clear(&t->step[i]);
                continue;
            }
            tg_write(&t->step[i], rw[w][r][k], base + (r == 0u ? semi[w][k] : 0));
        }
        t->p[P_SLEN] = (int16_t)len;
        lastc[r] = 0;
    }
}

static step_t tg_undo_step[TG_ROWS][NSTEP];
static int16_t tg_undo_len[TG_ROWS], tg_undo_swing[TG_ROWS];
static uint8_t tg_undo_ok;
static void tg_undo_take(void)
{
    uint32_t r;
    for (r = 0; r < TG_ROWS; r++) {
        memcpy(tg_undo_step[r], trk[r].step, sizeof tg_undo_step[r]);
        tg_undo_len[r] = trk[r].p[P_SLEN];
        tg_undo_swing[r] = trk[r].p[P_SSWING];
    }
    tg_undo_ok = 1;
}
static int tg_undo(void)
{
    uint32_t r;
    if (!tg_undo_ok)
        return 0;
    fm1_irq_off();
    for (r = 0; r < TG_ROWS; r++) {
        memcpy(trk[r].step, tg_undo_step[r], sizeof tg_undo_step[r]);
        trk[r].p[P_SLEN] = tg_undo_len[r];
        trk[r].p[P_SSWING] = tg_undo_swing[r];
    }
    fm1_irq_on();
    tg_undo_ok = 0;
    return 1;
}
static uint32_t tg_seed;
static void trap_drum_make(void)
{
    int32_t l = trk[0].p[P_SLEN];
    tg_undo_take();
    tg_seed = tr_seed();
    fm1_irq_off();
    trap_drum_random(tg_seed, l > 32 ? 64u : l > 16 ? 32u : 16u);
    tr_mix=100;
    fm1_irq_on();
}

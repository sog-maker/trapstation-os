/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION OS contributors */

#define TK_NP 12u
typedef struct { uint8_t id; int16_t v; } trap_pv_t;
typedef struct { const char *pill; trap_pv_t p[TK_NP]; } trap_snd_t;
_Static_assert(P_COUNT <= 256, "trap_pv_t.id is a byte");
typedef struct { const char *name; trap_snd_t s[NTRK]; } trap_kit_t;
#define E(k, v) {P_E0 + (k), (v)}
#define LVL(v) {P_LN0, (v)}
#define CRS(v) {P_LN1, (v)}
#define SUB(v) {P_DIST, (v)}
#define GLD(v) {P_GLIDE, (v)}
static const trap_kit_t TRAP_KITS[] = {
    {"MEMPHIS 808", {

        {"8BD", {E(0, 41), E(1, 94), E(2, 25), E(3, 60), E(4, 38), E(5, 28), E(6, 60), LVL(110), SUB(50)}},

        {"8SD", {E(0, 70), E(1, 48), E(2, 72), E(3, 49), E(4, 91), E(5, 44), E(6, 30), LVL(105), CRS(20)}},

        {"8CLP", {E(0, 64), E(1, 43), E(2, 38), E(3, 50), E(4, 64), E(5, 25), E(6, 20), LVL(100), CRS(15)}},

        {"8HH", {E(0, 64), E(1, 25), E(2, 56), E(3, 80), E(4, 76), E(5, 50), E(6, 10), LVL(90), CRS(15)}},

        {"8COW", {E(0, 64), E(1, 45), E(2, 80), E(3, 40), E(4, 46), E(5, 50), E(6, 30), LVL(95)}},
        {"FMSYN", {{0, 0}}},
    }},
    {"DR660 KIT", {

        {"8BD", {E(0, 50), E(1, 48), E(2, 70), E(3, 90), E(4, 70), E(5, 20), E(6, 40), LVL(110), CRS(20), SUB(30)}},

        {"8SD", {E(0, 74), E(1, 56), E(2, 85), E(3, 42), E(4, 80), E(5, 35), E(6, 35), LVL(105), CRS(35)}},

        {"8RIM", {E(0, 70), E(1, 29), E(2, 70), E(3, 60), E(4, 64), E(5, 70), E(6, 20), LVL(100), CRS(25)}},

        {"8HH", {E(0, 64), E(1, 22), E(2, 62), E(3, 85), E(4, 80), E(5, 60), E(6, 10), LVL(90), CRS(25)}},

        {"8HH", {E(0, 64), E(1, 69), E(2, 62), E(3, 85), E(4, 80), E(5, 40), E(6, 10), LVL(85), CRS(25)}},

        {"8CLP", {E(0, 64), E(1, 50), E(2, 34), E(3, 55), E(4, 80), E(5, 30), E(6, 20), LVL(100), CRS(25)}},
    }},
};
#undef E
#undef LVL
#undef CRS
#undef SUB
#undef GLD
#define TRAP_NKITS (sizeof TRAP_KITS / sizeof TRAP_KITS[0])

static uint32_t trap_mach(const char *pill)
{
    uint32_t m;
    for (m = 0; m < NMACH; m++)
        if (str_eq(MACHINES[m].pill, pill))
            return m;
    return 0;
}

static void trap_snd_apply(track_t *t, const trap_snd_t *s)
{
    uint32_t i;
    machine_set(t, trap_mach(s->pill));
    fm1_irq_off();
    for (i = 0; i < TK_NP && s->p[i].id; i++)
        t->p[s->p[i].id] = (int16_t)param_fit(track_desc(t, s->p[i].id), s->p[i].v);
    fm1_irq_on();
}

static int trap_kit_has(uint32_t k) { return k < TRAP_NKITS; }
static const char *trap_kit_name(uint32_t k) { return k < TRAP_NKITS ? TRAP_KITS[k].name : 0; }

static void trap_kit_sounds(uint32_t k, int macros)
{
    uint32_t i, j;
    if (k >= TRAP_NKITS)
        return;
    xf_leave(XF_ALL);
    for (i = 0; i < NTRK; i++) {
        kit_undo(&trk[i]);
        for (j = 0; j < 16u; j++)
            kit[i][j].n = 0;
        trap_snd_apply(&trk[i], &TRAP_KITS[k].s[i]);
        lastc[i] = 0;
    }
    if (macros)
        memset(pm_mac, 0, sizeof pm_mac);
    ui.force = 1;
}
static void trap_kit_load(uint32_t k) { trap_kit_sounds(k, 1); }

static void trap_kit_power(void) { trap_kit_sounds(0, 0); }

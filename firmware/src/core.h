/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

#include <stdint.h>

#ifndef TRAP_UX_TEST
#define TRAP_UX_TEST 0
#endif

#define NVOICE 8
#define NPART 6
#define NTRK NPART
#define NSTEP 64
#define HALF_FRAMES 128
#ifndef FELUCCA_SLICE
#define FELUCCA_SLICE 0
#endif
#ifndef FELUCCA_SIMPLE
#define FELUCCA_SIMPLE 1
#endif
#ifndef FELUCCA_FM4
#define FELUCCA_FM4 1

#endif
#ifndef FELUCCA_CPU2
#define FELUCCA_CPU2 1

#endif
#define C1_NP (FELUCCA_CPU2 ? NPART : 1)

#define C1_PI(t) ((uint32_t)((t) - trk) % C1_NP)
#ifndef TRAP_FM6_TEST
#define TRAP_FM6_TEST 0
#endif
#define ENGI_FM6 5u
#define ENGI_SYNTH (TRAP_FM6_TEST ? ENGI_FM6 : ENGI_DIGITAL)
#define NENGINES (5u + TRAP_FM6_TEST)
#define ENGI_DIGITAL 1u
#define ENGI_ANALOG 2u
#define ENGI_SAMPLER 3u
#define ENGI_MONO 4u
#define NENG_SHOWN (FELUCCA_SIMPLE ? 1 : NENGINES - !FELUCCA_FM4)

#define UP_SLOTS 32u

#include "engine.h"

enum {
    P_LEVEL,
    P_ATK, P_DEC, P_SUS, P_REL,
    P_ED_FLT, P_ED_PIT, P_ED_SHP, P_ED_FX,
    P_LRATE, P_LWAVE, P_LPHASE, P_LFADE,
    P_LD_PIT, P_LD_FLT, P_LD_SHP, P_LD_AMP,
    P_AMODE, P_ARATE, P_AOCT, P_AGATE,
    P_ASWING, P_APROB, P_AHOLD, P_AORDER,
    P_ROOT, P_SCALE, P_QUANT, P_TRANS,
    P_SLEN, P_SDIV, P_SSWING, P_SGATE,
    P_DIST, P_CHOR, P_DLY, P_REV,
    P_VOICE, P_GLIDE, P_PAN, P_MUTE,
    P_GLMODE, P_PRIO, P_ALLOC, P_DETUNE,
    P_SLCR, P_SLPAT, P_SLRATE, P_SLDEPTH,
    P_M1SRC, P_M1DST, P_M1AMT,
    P_M2SRC, P_M2DST, P_M2AMT,
    P_M3SRC, P_M3DST, P_M3AMT,
    P_M4SRC, P_M4DST, P_M4AMT,
    P_FM1_ATK, P_FM1_DEC, P_FM1_SUS, P_FM1_REL, P_FM1_LEVEL,
    P_FM2_ATK, P_FM2_DEC, P_FM2_SUS, P_FM2_REL, P_FM2_LEVEL,
    P_FM3_ATK, P_FM3_DEC, P_FM3_SUS, P_FM3_REL, P_FM3_LEVEL,
    P_FM4_ATK, P_FM4_DEC, P_FM4_SUS, P_FM4_REL, P_FM4_LEVEL,
    P_CHRD, P_VOIC,
    P_LN0, P_LN1, P_LN2, P_LN3, P_LN4, P_LN5, P_LN6, P_LN7,
    P_LSYNC, P_LTRIG, P_LPOL,
    P_SQNT,
    P_SPRD,
    P_ITYPE, P_IA, P_IB, P_IC, P_IMIX,
    P_FTYPE,
    P_ESYNC,
    P_E0, P_E1, P_E2, P_E3, P_E4, P_E5, P_E6, P_E7,
    P_COUNT
};

#define P_FLT_CUT P_FM1_ATK
#define P_FLT_RES P_FM1_DEC
#define P_FLT_DRV P_FM1_SUS
#define P_FLT_KTR P_FM1_REL
#define P_FE_ATK P_FM2_ATK
#define P_FE_DEC P_FM2_DEC
#define P_FE_SUS P_FM2_SUS
#define P_FE_AMT P_FM2_LEVEL
#define P_FE_REL P_FM2_REL

enum {
    G_BPM, G_SWING, G_CLOCK, G_TUNE,
    G_DTIME, G_DFDBK, G_DCOLOR, G_DMIX,
    G_RSIZE, G_RDAMP, G_CRATE, G_CDEPTH,
    G_MIDI, G_SYNC, G_ROUTE, G_INFO,

    G_F1RATE, G_F1DEST, G_F1AMT, G_F2RATE,
    G_F2DEST, G_F2AMT,
    G_F3RATE, G_F3DEST,
    G_RTYPE,

    G_F3AMT, G_AMP,
    G_F1WAVE, G_F2WAVE, G_F3WAVE,
    G_PAN,
    G_COUNT
};

static void params_by_count(int16_t *out, const int16_t *in, uint32_t np, const int16_t *def)
{
    uint32_t i, nc = np - 8u;
    for (i = 0; i < P_E0; i++)
        out[i] = i < nc ? in[i] : def[i];
    for (i = 0; i < 8u; i++)
        out[P_E0 + i] = in[nc + i];
}

#define ENGI_PHYS 9u
#define ENGI_DRUM 10u

#define ENGI_SAMPLE 4u
#define SMP_SET_PERC 4u
#define DRUM_KIT_E {0, 64, 70, 64, 64, 100, 0, 0}

enum { V_POLY, V_MONO, V_LEGATO, V_UNISON };

enum { ST_NOTE, ST_TIE, ST_REST, ST_LOCK };
#define SF_ACCENT 1u
#define SF_SLIDE 2u
#define SF_RATCH_SH 3u
#define SF_RATCH (3u << SF_RATCH_SH)
#define NLANE 8
typedef struct {
    uint8_t note[4];
    uint8_t n;
    uint8_t time;
    uint8_t flags;
    uint8_t vel;
    uint8_t hit;
    uint8_t acc;
    uint8_t probability;
    uint8_t roll;
} step_t;

#define SR_RAMP_SH 3u
#define SR_LAST 0x20u
#define SR_TRIP 0x40u
enum { RAMP_FLAT, RAMP_UP, RAMP_DOWN };
static inline int32_t step_stair(const step_t *s) { int32_t v = s->roll & 7; return v > 3 ? v - 8 : v; }
static inline uint32_t step_ramp(const step_t *s) { return (s->roll >> SR_RAMP_SH) & 3u; }
static inline void step_set_roll(step_t *s, int32_t stair, uint32_t ramp, uint32_t last)
{
    s->roll = (uint8_t)((s->roll & SR_TRIP) | ((uint32_t)stair & 7u) | (ramp % 3u) << SR_RAMP_SH | (last ? SR_LAST : 0u));
}
typedef struct {
    uint8_t note[4];
    uint8_t n, time, flags, vel;
} step8_t;

#define COND_FIRST 102u
#define NCOND 11u
#define COND_FILL 10u
#define COND_NFILL 11u
#define CHANCE_V_MAX (100u + NCOND)

#define GB_TC0 150u
#define GB_NAB 35u
#define GB_NTC (GB_NAB + 8u)
#define GB_TC_FILL (GB_NAB + 1u)
#define GB_LOOPS 840u
static uint32_t gb_tc(const step_t *s)
{
    return s->acc > GB_TC0 && s->acc <= GB_TC0 + GB_NTC ? s->acc - GB_TC0 : 0u;
}
static void gb_tc_ab(uint32_t c, uint32_t *a, uint32_t *b)
{
    uint32_t n = 2u;
    c--;
    while (c >= n) {
        c -= n;
        n++;
    }
    *a = c + 1u;
    *b = n;
}
static uint32_t step_cond(const step_t *s)
{
    return s->probability >= COND_FIRST && s->probability < COND_FIRST + NCOND ? s->probability - COND_FIRST + 1u : 0u;
}
static uint32_t step_chance(const step_t *s)
{
    return !s->probability ? 100u : s->probability <= 100u ? s->probability : s->probability == 101u ? 0u : 100u;
}

static uint32_t step_ratchet(const step_t *s) { return ((s->flags & SF_RATCH) >> SF_RATCH_SH) + 1u; }

static inline int step_trip(const step_t *s) { return (s->roll & SR_TRIP) && step_ratchet(s) == 3u; }

#define NUDGE_DIV 16
#define SF_NUDGE (4u | 0xE0u)
static int32_t step_nudge(const step_t *s)
{
    uint32_t u = (uint32_t)(s->flags >> 5) | (uint32_t)(s->flags & 4u) << 1;
    return (int32_t)(u ^ 8u) - 8;
}
static void step_set_nudge(step_t *s, int32_t v)
{
    uint32_t u = (uint32_t)(v < -8 ? -8 : v > 7 ? 7 : v) & 15u;
    s->flags = (uint8_t)((s->flags & ~SF_NUDGE) | (u & 7u) << 5 | (u >> 1 & 4u));
}
#define MOTION_MAX 128u

#define MOTION_LOCK 0x80u
#define MOTION_ID(e) ((uint32_t)(e)->param & 0x7Fu)
typedef struct { uint16_t place; uint8_t param; int8_t value; } motion_event_t;
typedef struct { uint8_t count, on, rsv[2]; motion_event_t event[MOTION_MAX]; } motion_store_t;
_Static_assert(sizeof(motion_store_t) == 516u, "motion: 128 records of 4 bytes");

typedef struct track {
    int16_t p[P_COUNT];
    uint8_t engine, preset;
    uint8_t eng_req;
    uint8_t user;
    voice_t v[NVOICE];

    uint32_t lfo_ph;
    int32_t lfo_val;
    int32_t lfo_fade;
    uint32_t lfo_rnd;
    int16_t lfo_prev;

    uint8_t held[16];
    uint8_t nheld;
    uint8_t arp_phys;
    uint8_t latched;

    uint32_t arp_pos;
    uint32_t arp_idx;
    uint8_t arp_note;
    uint8_t arp_walk;
    uint8_t arp_ch[3];
    uint32_t arp_off;

    step_t step[NSTEP];
    uint32_t seq_pos;
    uint16_t seq_idx;
    uint8_t seq_notes[4 + NLANE];
    uint8_t seq_n;
    uint8_t seq_hold;
    uint8_t slide_glide;
    uint8_t rat_left;
    uint32_t seq_off;
    uint8_t seq_active;
    uint8_t rskip_idx;
    uint8_t rskip_n, rskip[NLANE];

    uint32_t nd_late;
    uint32_t nd_early;
    uint8_t nd_fired;

    uint8_t rh_n, rh_note[4];
    uint8_t rh_start;
    uint8_t rh_ties;
    uint8_t rh_last;
    step_t rh_bak;

    uint8_t mono_stack[8];
    uint8_t nmono;
    uint8_t mono_note;
    uint8_t rr;

    int32_t peak;
    int32_t dist_hp, dist_lp1, dist_lp2;
    uint8_t tail;
    uint8_t ins_run;
    uint8_t cond_loop;
#if FELUCCA_CPU2

    uint8_t lfo_wrap;
    uint32_t esync_k[3];
    uint8_t spread_on;
    uint8_t rsv_sz[1];
#else
    uint8_t rsv_sz[14];
#endif
    int16_t armp, aholdp;

    uint8_t xf_on, xf;
    int16_t pe_old[8];
    uint8_t xp_n, xp_note[4], xp_vel[4];

    uint8_t mw, at;
    uint8_t ex_off;
    uint8_t m_vel, m_key, m_vi;
    uint8_t sp_alt;
    int16_t m_rnd;
    int32_t m_env;
    int32_t m_fenv;
    uint16_t gb_loop;
    uint8_t tc_last;
    uint8_t rsv_fe[9];
} track_t;

_Static_assert(sizeof(track_t) % 16u == 0, "track_t: a multiple of 16 bytes (a cheap trk[] index on the target)");

typedef struct {
    int16_t g[G_COUNT];
    uint8_t playing, seq_mode;
    uint8_t grid;

    uint8_t rec;
    uint8_t sel;
    int8_t octave;
    uint32_t tick;
    uint32_t cpu_q8;
    uint32_t master_q12;
    int32_t batt_raw;
    uint8_t mch[NTRK];

} song_t;
#define ROUTE_SEL 1
#define ROUTE_TRACK 5
#define MCH_OMNI 16u
#define MCH_OFF 17u
#define MCH_N 18u

static track_t trk[NTRK];
static song_t song;
#define TSEL (&trk[song.sel])

static inline uint32_t mch_get(uint32_t i)
{
    uint32_t v = song.mch[i % NTRK];
    return v < 16u ? (v + i) & 15u : v < MCH_N ? v : (i & 15u);
}
static inline void mch_set(uint32_t i, uint32_t ch) { song.mch[i % NTRK] = (uint8_t)(ch < 16u ? (ch - i) & 15u : ch < MCH_N ? ch : 0u); }

#define SWING_MAX 100
static inline int32_t track_swing(const track_t *t)
{
    int32_t sw = t->p[P_SSWING] + song.g[G_SWING];
    return sw < 0 ? 0 : sw > SWING_MAX ? SWING_MAX : sw;
}

static inline uint32_t swing_step_len(const track_t *t, uint32_t base, uint32_t idx)
{
    int32_t sw = track_swing(t) * (int32_t)base / 250;
    return base + (uint32_t)((idx & 1u) ? -sw : sw);
}

#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
static volatile uint32_t fm1_ms;

#define BOOTGUARD_MAGIC 0x42475244u
struct { uint32_t magic, failed, pending; } bootguard __attribute__((section(".noinit")));

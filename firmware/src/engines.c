/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

#include "dsp.c"                /* the shared DSP blocks (MIT, from Felucca-Engines) */
#include "eng_digital.c"
#include "eng_analog.c"
#include "eng_sampler.c"
#include "eng_mono.c"
#if TRAP_FM6_TEST
#include "eng_trap_fm6.c"
#endif
#define ICON_MUTE GBI_MUTE
#define ICON_DELAY GBI_DELAY
#define ICON_FADE GBI_FADE
#include "icons_gb.h"
#include "icons_808.h"
#undef ICON_MUTE
#undef ICON_DELAY
#undef ICON_FADE

enum { IC_NONE, IC_SYN, IC_KICK, IC_SNARE, IC_HATS, IC_CYMBAL, IC_PERC, IC_TOM, IC_RIM, IC_CLAP, IC_COWBELL, IC_AGE,
       IC_LP, IC_BP, IC_HP, IC_DJ, IC_COMB, IC_SINE, IC_TRI, IC_SAW, IC_SQUARE, IC_RANDOM, IC_DELAY, IC_VERB, IC_MUTE,
       IC_FADE, IC_REPEAT, IC_REVERSE, IC_MACRO, IC_SAMPLE, IC_ONCE, IC_LOOP, IC_PING, IC_REV, IC_COUNT };
static const uint16_t *const GB_ICONS[IC_COUNT] = {
    0, ICON_SYN, ICON_KICK, ICON_SNARE, ICON_HATS, ICON_CYMBAL, ICON_PERC, ICON_TOM, ICON_RIM, ICON_CLAP, ICON_COWBELL,
    ICON_AGE, ICON_LOW_PASS, ICON_BAND_PASS, ICON_HIGH_PASS, ICON_DJ_FILTER, ICON_COMB, ICON_SINE, ICON_TRI, ICON_SAW,
    ICON_SQUARE, ICON_RANDOM, GBI_DELAY, ICON_VERB, GBI_MUTE, GBI_FADE, ICON_REPEAT, ICON_REVERSE, ICON_MACRO, ICON_SMP,
    ICON_SMP_ONCE, ICON_SMP_LOOP, ICON_SMP_PING, ICON_SMP_REV};

static const engine_t *const ENGINES[NENGINES] = {&ENG_DIGITAL, &ENG_DIGITAL, &ENG_ANALOG, &ENG_SAMPLER, &ENG_MONO
#if TRAP_FM6_TEST
, &ENG_TRAP_FM6
#endif
};

static inline uint32_t eng_idx(uint32_t e) { return e < NENGINES ? e : 0u; }

typedef struct {
    const char *pill;
    uint8_t engine, sub;
    uint8_t drum;
    uint8_t note;
    uint8_t preset;
    uint8_t icon;
} machine_t;
static const machine_t MACHINES[] = {

    {"8BD", ENGI_ANALOG, AN_KICK, 1, AN_NOTE, AN_KICK, IC_KICK},
    {"FMBD", ENGI_DIGITAL, 1, 1, 36, 8, IC_KICK},
    {"8SD", ENGI_ANALOG, AN_SNARE, 1, AN_NOTE, AN_SNARE, IC_SNARE},
    {"FMSD", ENGI_DIGITAL, 2, 1, 50, 9, IC_SNARE},
    {"8TOM", ENGI_ANALOG, AN_TOM, 1, AN_NOTE, AN_TOM, IC_TOM},
    {"8RIM", ENGI_ANALOG, AN_RIM, 1, AN_NOTE, AN_RIM, IC_RIM},
    {"8CLP", ENGI_ANALOG, AN_CLAP, 1, AN_NOTE, AN_CLAP, IC_CLAP},
    {"8COW", ENGI_ANALOG, AN_COW, 1, AN_NOTE, AN_COW, IC_COWBELL},
    {"8HH", ENGI_ANALOG, AN_HAT, 1, AN_NOTE, AN_HAT, IC_HATS},
    {"FMHH", ENGI_DIGITAL, 3, 1, 84, 10, IC_HATS},
    {"8CYM", ENGI_ANALOG, AN_CYM, 1, AN_NOTE, AN_CYM, IC_CYMBAL},
    {"FMCYM", ENGI_DIGITAL, 4, 1, 72, 12, IC_CYMBAL},
    {"FMPRC", ENGI_DIGITAL, 5, 1, 48, 13, IC_PERC},
    {"FMSYN", ENGI_SYNTH, 0, 0, 60, 0, IC_SYN},
    {"MONO", ENGI_MONO, 0, 0, 60, 0, IC_SAW},
    {"SMPL", ENGI_SAMPLER, 0, 2, SMP_NOTE, 0, IC_SAMPLE},
    {"8BAS", ENGI_ANALOG, AN_BASS, 1, AN_NOTE, AN_BASS, IC_KICK},
};
#define NMACH (sizeof MACHINES / sizeof MACHINES[0])

static inline uint32_t trk_mach(const track_t *t)
{
    uint32_t m;
    for (m = 0; m < NMACH; m++)
        if (ENGINES[MACHINES[m].engine] == ENGINES[eng_idx(t->eng_req)] && MACHINES[m].sub == (uint32_t)t->p[P_E7])
            return m;
    return 0;
}
static inline const machine_t *trk_machine(const track_t *t) { return &MACHINES[trk_mach(t)]; }

enum { GB_KICK, GB_SNARE, GB_CYMBAL, GB_PERC, GB_SYNTH, GB_SAMPLE, GB_NPOOL };
static const char *const GB_POOL_NAME[GB_NPOOL] = {"KICK", "SNARE", "CYMBAL", "PERC", "SYNTH", "SAMPLE"};
static uint32_t gb_pool(uint32_t mach)
{
    switch (MACHINES[mach % NMACH].icon) {
    case IC_KICK: return GB_KICK;
    case IC_SNARE: case IC_CLAP: return GB_SNARE;
    case IC_HATS: case IC_CYMBAL: return GB_CYMBAL;
    case IC_SYN: case IC_SAW: return GB_SYNTH;
    case IC_SAMPLE: return GB_SAMPLE;
    default: return GB_PERC;
    }
}

static const uint8_t TRK_DEF[NPART] = {8, 9, 10, 11, 13, 0};

/* SPDX-License-Identifier: MIT
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

#ifndef FELUCCA_ENGINE_H
#define FELUCCA_ENGINE_H
#include <stdint.h>

#ifndef NVOICE
#error "engine.h: the host defines NVOICE (voices per part) first"
#endif
#ifndef NPART
#error "engine.h: the host defines NPART (parts) first"
#endif

#ifndef NELEM
#define NELEM(a) (sizeof(a) / sizeof((a)[0]))
#endif

enum {
    F_INT, F_PCT, F_BIPCT, F_TIME, F_LFOHZ, F_CUTOFF, F_DB, F_SEMI, F_ENUM, F_BPM, F_NOTE,
    F_ONOFF, F_OCT, F_STEPS
};

typedef struct {
    const char *label;
    uint8_t fmt;
    int16_t min, max, def;
    const char *const *names;
    const char *unit;
} param_desc_t;

typedef struct {
    uint8_t note, vel, gate, active;
    uint8_t stage;
    uint8_t side;
    int32_t env;
    int32_t env_out;
    int32_t pitch16, pitch_cur;
    int32_t gstep;
    int32_t fine;
    uint32_t ph[3];
    int32_t s[8];
    uint32_t age;
    uint8_t mvel;
    int16_t mrnd;
} voice_t;

typedef struct {
    uint32_t inc;
    int32_t pitch16;
    int32_t amp0, amp1;
    int32_t cutoff;
    int32_t shape;
    int32_t envq15;
    int32_t fine;
} vmod_t;

typedef struct {
    const char *name;
    int8_t e[8];
    uint8_t env[4];
    int8_t fenv;
    uint8_t mono;

    uint8_t fx[4];
    uint8_t pat;

} preset_t;
#define FX(d, c, dl, r) .fx = {(d) + 1, (c) + 1, (dl) + 1, (r) + 1}
#define PAT(n) .pat = (n)

struct track;
typedef struct {
    const char *name;
    const char *page_title[2];
    param_desc_t edit[8];
    const preset_t *presets;
    uint8_t npresets;
    uint8_t knob[4];
    uint8_t poly;
    uint8_t sampled;

    uint8_t keep;
    uint8_t oneshot;
    void (*note_on)(struct track *t, voice_t *v);
    void (*render)(struct track *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m);

    int32_t (*amp)(struct track *t, voice_t *v, int32_t adsr);

    const param_desc_t *(*desc)(const struct track *t, uint32_t k);

    void (*block)(struct track *t);

    int32_t (*keys)(const struct track *t, uint32_t k);

    uint8_t ownenv;
    int (*done)(struct track *t, voice_t *v);

    uint16_t mem;
    void (*post)(struct track *t, int32_t *out, uint32_t n);
} engine_t;

#endif

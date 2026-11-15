/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

#pragma once
#include <stdint.h>
#include "fm1_time.h"
#include "fm1_gpio.h"
#include "fm1_led_anim.h"

#ifndef FM1_INPUT_IDLE
#define FM1_INPUT_IDLE() ((void)0)
#endif
#ifndef FM1_INPUT_NOW
#define FM1_INPUT_NOW() fm1_ticks()
#endif
#ifndef FM1_LED_US
#define FM1_LED_US 40u
#endif
#define FM1_DEB_PRESS 2u
#define FM1_DEB_RELEASE 8u
#define FM1_INPUT_LAT 1
#define FM1_SETTLE_US 10u
#ifndef FM1_LED_DIM_NS
#define FM1_LED_DIM_NS 3200u
#endif
#ifndef FM1_LED_DIM_LO_NS
#define FM1_LED_DIM_LO_NS 1600u
#endif
#ifndef FM1_LED_DIM_DIV
#define FM1_LED_DIM_DIV 1u
#endif
#define FM1_LED_BREATH_FRAMES 1024u
#ifndef FM1_LED_BREATH_PK
#define FM1_LED_BREATH_PK 154u
#endif
#ifndef FM1_LED_BREATH_PK_LO
#define FM1_LED_BREATH_PK_LO 77u
#endif
#ifndef FM1_LED_MID_N
#define FM1_LED_MID_N 8u
#endif
#ifndef FM1_LED_MID_N_LO
#define FM1_LED_MID_N_LO 12u
#endif
#ifndef FM1_LED_TICK_US
#define FM1_LED_TICK_US 100u
#endif
#ifndef FM1_LED_TRACE
#define FM1_LED_TRACE(rowmask) ((void)0)
#endif
#ifndef FM1_SR_LATCH_TRACE
#define FM1_SR_LATCH_TRACE() ((void)0)
#endif
#define FM1_REST_FRAMES 900u
#define FM1_ENC_GO 20u
#define FM1_NCOL 11u
#define FM1_NKEY 41u
#define FM1_NENC 7u

static const int8_t FM1_KEYMAP[6][FM1_NCOL] = {
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { 5, 11,  4, 10,  3,  9,  2,  8, -1, -1, -1},
    {34, 35, 36, 37, 38, 40, 39, 13,  7,  6, 12},
    {23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
    { 0,  1, 15, 14, 17, 16, 19, 18, 20, 21, 22},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
};

static const uint8_t FM1_ENC[FM1_NENC][4] = {
    {0, 0, 1, 0}, {2, 0, 3, 0}, {8, 1, 9, 1}, {8, 0, 9, 0}, {6, 0, 7, 0}, {4, 0, 5, 0}, {0, 5, 1, 5},
};
enum { FM1_BTN_OCT_DOWN = 0, FM1_BTN_OCT_UP = 1 };

static volatile struct {
    uint32_t notes;
    uint32_t buttons;
    uint32_t pressed, released;
    uint32_t notes_pressed;
    uint8_t raw[FM1_NCOL];
    uint8_t cnt[FM1_NKEY];
    uint32_t note_t0[27];
    uint8_t enc_prev[FM1_NENC], enc_last[FM1_NENC];
    uint8_t enc_rest[FM1_NENC];
    uint16_t enc_still[FM1_NENC];
    int8_t enc_sub[FM1_NENC];
    int16_t enc_steps[FM1_NENC];
    int8_t enc_dir[FM1_NENC];
    uint32_t enc_dfr[FM1_NENC];
    uint32_t frames;
} fm1_in;
static uint8_t fm1_led[FM1_NCOL];
static uint8_t fm1_led_dim[FM1_NCOL];
static uint8_t fm1_led_breath[FM1_NCOL];
static uint8_t fm1_led_mid[FM1_NCOL];

#define FM1_GAP_BINS 8u
static volatile struct {
    uint32_t last, gap_max, gap_hist[FM1_GAP_BINS];
    uint32_t on[FM1_NCOL], on_max[FM1_NCOL];
    uint32_t cost_sum, cost_max, ticks;
    uint32_t enc_moves[FM1_NENC], enc_lost[FM1_NENC];
    uint32_t press_n, press_sum, press_max;

    uint32_t kb_n, kb_sum, kb_max, dac_sum, dac_max;
    uint32_t dim_n, dim_sum;
} fm1_in_stat;

static void fm1__led_lines(uint32_t rowmask)
{
    FM1_LED_TRACE(rowmask);
    uint32_t a = FM1_PR(FM1_PA, FM1_OUT) & ~((1u << 9) | (1u << 10));
    uint32_t h = FM1_PR(FM1_PH, FM1_OUT) & ~((1u << 6) | (1u << 9));
    FM1_PR(FM1_PA, FM1_OUT) = a | (rowmask & 2u) << 8 | (rowmask & 4u) << 8;
    FM1_PR(FM1_PH, FM1_OUT) = h | (rowmask & 8u) << 3 | (rowmask & 16u) << 5;
}

static void fm1__sr_bits(uint32_t w, uint32_t i0, uint32_t i1)
{
    uint32_t i;

    for (i = i0; i < i1; i++) {
        if (w & (0x8000u >> i))
            FM1_PR(FM1_PA, FM1_OUT) |= 1u << 4;
        else
            FM1_PR(FM1_PA, FM1_OUT) &= ~(1u << 4);
        FM1_PR(FM1_PA, FM1_OUT) |= 1u << 3;
        FM1_PR(FM1_PA, FM1_OUT) &= ~(1u << 3);
    }
}
static void fm1__sr_latch(void)
{
    FM1_SR_LATCH_TRACE();
    FM1_PR(FM1_PA, FM1_OUT) |= 1u << 1;
    FM1_PR(FM1_PA, FM1_OUT) &= ~(1u << 1);
}
static void fm1__sr_word(uint32_t w)
{
    fm1__sr_bits(w, 0, 16u);
    fm1__sr_latch();
}

static uint32_t fm1__rows(void)
{
    uint32_t a = FM1_PR(FM1_PA, FM1_IN), b = FM1_PR(FM1_PB, FM1_IN);
    return (~((a & 1u) | ((a >> 4) & 0x1Eu) | ((b >> 2) & 0x20u))) & 0x3Fu;
}

static void fm1__wait(uint32_t us)
{
    uint32_t t0 = fm1_ticks(), span = us * FM1_TICKS_PER_US;
    while ((uint32_t)(fm1_ticks() - t0) < span)
        FM1_INPUT_IDLE();
}

static void fm1_input_init(void)
{
    static const uint8_t LEDP[4][2] = {{FM1_PH, 6}, {FM1_PH, 9}, {FM1_PA, 9}, {FM1_PA, 10}};
    const uint32_t rows_a = (1u << 0) | (1u << 5) | (1u << 6) | (1u << 7) | (1u << 8);
    const uint32_t sr = (1u << 1) | (1u << 3) | (1u << 4), row_b = 1u << 7;
    uint32_t i;
    for (i = 0; i < 4u; i++) {
        uint32_t p = LEDP[i][0], m = 1u << LEDP[i][1];
        FM1_PR(p, FM1_DIE) |= m;
        FM1_PR(p, FM1_OUT) &= ~m;
        FM1_PR(p, FM1_DIR) &= ~m;
        FM1_PR(p, FM1_HD0) |= m;
        FM1_PR(p, FM1_HD) |= m;
    }
    FM1_PR(FM1_PA, FM1_DIE) |= rows_a;
    FM1_PR(FM1_PA, FM1_DIR) |= rows_a;
    FM1_PR(FM1_PA, FM1_PD) &= ~rows_a;
    FM1_PR(FM1_PA, FM1_PU) |= rows_a;
    FM1_PR(FM1_PB, FM1_DIE) |= row_b;
    FM1_PR(FM1_PB, FM1_DIR) |= row_b;
    FM1_PR(FM1_PB, FM1_PD) &= ~row_b;
    FM1_PR(FM1_PB, FM1_PU) |= row_b;
    FM1_PR(FM1_PA, FM1_DIE) |= sr;
    FM1_PR(FM1_PA, FM1_PU) &= ~sr;
    FM1_PR(FM1_PA, FM1_PD) &= ~sr;
    FM1_PR(FM1_PA, FM1_OUT) &= ~sr;
    FM1_PR(FM1_PA, FM1_DIR) &= ~sr;
    fm1__sr_word(0xFFFFu);
    for (i = 0; i < FM1_NENC; i++)
        fm1_in.enc_prev[i] = fm1_in.enc_last[i] = 0xFF;
}

static void fm1__key(uint32_t id, uint32_t closed)
{
    volatile uint8_t *c = &fm1_in.cnt[id];
    uint32_t note = id >= 14u, bit = note ? 1u << (id - 14u) : 1u << id;
    uint32_t on = ((note ? fm1_in.notes : fm1_in.buttons) & bit) != 0u;
    if (closed == on) {
        *c = 0;
        return;
    }
    if (!on && note && *c == 0u)
        fm1_in.note_t0[id - 14u] = FM1_INPUT_NOW();
    if (++*c < (on ? FM1_DEB_RELEASE : FM1_DEB_PRESS))
        return;
    *c = 0;
    if (note) {
        if (!on) {
            uint32_t d = FM1_INPUT_NOW() - fm1_in.note_t0[id - 14u];
            fm1_in.notes_pressed |= bit;
            fm1_in_stat.press_n++;
            fm1_in_stat.press_sum += d;
            if (d > fm1_in_stat.press_max)
                fm1_in_stat.press_max = d;
        }
        fm1_in.notes ^= bit;
    } else {
        fm1_in.buttons ^= bit;
        if (!on)
            fm1_in.pressed |= bit;
        else
            fm1_in.released |= bit;
    }
}

static void fm1__keys(uint32_t p)
{
    uint32_t r, raw = fm1_in.raw[p];
    for (r = 1; r < 5u; r++)
        if (FM1_KEYMAP[r][p] >= 0)
            fm1__key((uint32_t)FM1_KEYMAP[r][p], (raw >> r) & 1u);
}

static void fm1__frame(void);

static void fm1_input_scan(void)
{
    uint32_t p;
    for (p = 0; p < FM1_NCOL; p++) {
        fm1__led_lines(0);
        fm1__sr_word(0xFFFFu ^ (1u << p) ^ (p < 2u ? 1u << (11u + p) : 0u));
        fm1__wait(FM1_SETTLE_US);
        fm1_in.raw[p] = (uint8_t)fm1__rows();
        fm1__keys(p);
        fm1__led_lines(fm1_led[p]);
        fm1__wait(FM1_LED_US);
    }
    fm1__led_lines(0);
    fm1__frame();
}

static void fm1__frame(void)
{
    uint32_t e;
    for (e = 0; e < FM1_NENC; e++) {
        const uint8_t *m = FM1_ENC[e];
        uint32_t cur = ((fm1_in.raw[m[0]] >> m[1]) & 1u) << 1 | ((fm1_in.raw[m[2]] >> m[3]) & 1u);
        uint32_t idx;
        volatile int8_t *sub = &fm1_in.enc_sub[e];
        if (cur != fm1_in.enc_last[e]) {
            fm1_in.enc_last[e] = (uint8_t)cur;
            fm1_in.enc_still[e] = 0;
        }
        if (fm1_in.enc_prev[e] == 0xFF) {
            fm1_in.enc_prev[e] = (uint8_t)cur;
            fm1_in.enc_rest[e] = (uint8_t)cur;
        }
        if (fm1_in.enc_still[e] < 0xFFFFu && ++fm1_in.enc_still[e] == FM1_REST_FRAMES &&
            cur != fm1_in.enc_rest[e]) {

            fm1_in.enc_rest[e] = (uint8_t)cur;
            *sub = 0;
        }
        if (cur == fm1_in.enc_prev[e])
            continue;
        idx = (uint32_t)fm1_in.enc_prev[e] << 2 | cur;
        if ((0x4182u >> idx) & 1u) {
            (*sub)++;
            fm1_in_stat.enc_moves[e]++;
        } else if ((0x2814u >> idx) & 1u) {
            (*sub)--;
            fm1_in_stat.enc_moves[e]++;
        } else {
            int32_t d = *sub > 0 ? 1 : *sub < 0 ? -1 :
                        (uint32_t)(fm1_in.frames - fm1_in.enc_dfr[e]) <= FM1_ENC_GO ? fm1_in.enc_dir[e] : 0;
            fm1_in_stat.enc_lost[e]++;
            *sub = (int8_t)(*sub + 2 * d);
        }
        fm1_in.enc_prev[e] = (uint8_t)cur;
        if (*sub > 100 || *sub < -100)
            *sub = 0;
        if (cur == fm1_in.enc_rest[e]) {
            int32_t n = *sub < 0 ? -*sub : *sub;
            n = n >= 2 ? (n + 2) / 4 : 0;
            fm1_in.enc_steps[e] = (int16_t)(fm1_in.enc_steps[e] + (*sub < 0 ? -n : n));
            if (n) {
                fm1_in.enc_dir[e] = (int8_t)(*sub < 0 ? -1 : 1);
                fm1_in.enc_dfr[e] = fm1_in.frames;
            }
            *sub = 0;
        }
    }
    fm1_in.frames++;
}

#define FM1__DIM_T(ns) (((ns) * FM1_TICKS_PER_US + 500u) / 1000u)
static uint8_t fm1__tick_col, fm1__dim_k = 16u;
static uint16_t fm1__dim_t = FM1__DIM_T(FM1_LED_DIM_NS);

#define FM1__BR_G(ns) ((ns) * 65536u / (FM1_LED_TICK_US * 1000u * FM1_LED_DIM_DIV))
#define FM1__BR(pk, ns) {(pk), FM1__BR_G(ns), (1u << 24) / FM1__BR_G(ns), (1u << 30) / (65536u - FM1__BR_G(ns))}
static const uint32_t FM1__BR_K[2][4] = {FM1__BR(FM1_LED_BREATH_PK, FM1_LED_DIM_NS),
                                         FM1__BR(FM1_LED_BREATH_PK_LO, FM1_LED_DIM_LO_NS)};
static uint8_t fm1__br_sel;
static uint8_t fm1__mid_n = FM1_LED_MID_N, fm1__mid_ph;

static void fm1_led_dim_level(uint32_t lo)
{
    fm1__dim_t = (uint16_t)(lo ? FM1__DIM_T(FM1_LED_DIM_LO_NS) : FM1__DIM_T(FM1_LED_DIM_NS));
    fm1__br_sel = lo != 0u;
    fm1__mid_n = (uint8_t)(lo ? FM1_LED_MID_N_LO : FM1_LED_MID_N);
}

#define FM1__LIT(c) (fm1_led[c] | (fm1__br_lit ? fm1_led_breath[c] & ~fm1_led_dim[c] : 0u) | \
                     (!fm1__mid_ph ? fm1_led_mid[c] : 0u))
#if FM1_LED_DIM_DIV > 1
static uint8_t fm1__dim_ph;
#endif
static uint16_t fm1__br_ph;
static uint16_t fm1__br_lv, fm1__br_acc;
static uint8_t fm1__br_on, fm1__br_d, fm1__br_lit;
static void fm1__breath_frame(void)
{
    const uint32_t *q = FM1__BR_K[fm1__br_sel];
    uint32_t c, any = 0, x, d;
    for (c = 0; c < FM1_NCOL; c++)
        any |= fm1_led_breath[c];
    x = !any ? 0u : fm1__br_on ? (fm1__br_ph + 1u) & (FM1_LED_BREATH_FRAMES - 1u) : FM1_LED_BREATH_FRAMES / 2u;
    fm1__br_on = any != 0u;
    fm1__br_ph = (uint16_t)x;
    d = x & 7u;
    fm1__br_d = (uint8_t)(((d & 1u) << 7) | ((d & 2u) << 5) | ((d & 4u) << 3));
    x = x * 512u / FM1_LED_BREATH_FRAMES;
    x = x & 256u ? 511u - x : x;
    x = x * x * (768u - 2u * x) >> 16;
    x = x * x * q[0] >> 8;
    if (x <= q[1]) {
        fm1__br_lv = (uint16_t)(x * q[2] >> 16);
        fm1__br_lit = 0;
    } else {
        x = fm1__br_acc + ((x - q[1]) * q[3] >> 14);
        fm1__br_lit = x >= 65536u;
        fm1__br_acc = (uint16_t)x;
        fm1__br_lv = 256u;
    }
}

static uint16_t fm1__an_f;
static uint8_t fm1__an_glow, fm1__an_idle, fm1__an_cb[FM1_NKEY], fm1__an_acc[FM1_NKEY];

static __attribute__((noinline)) void fm1__anim_frame(void)
{
    uint32_t f = fm1__an_f - 1u, i, idle = fm1__an_idle;
    uint8_t l[FM1_NCOL] = {0}, d[FM1_NCOL] = {0};
    if (fm1_in.notes | fm1_in.buttons) {
        if (idle) {
            fm1__an_f = 0;
            return;
        }
        f = FM1_ANIM_FRAMES;
    }
    for (i = 0; i < FM1_NKEY; i++) {
        uint32_t q = idle ? fm1_idle_level(f, i, fm1__an_glow) : fm1_anim_level(f, i, fm1__an_glow);
        uint32_t cb = fm1__an_cb[i], a = fm1__an_acc[i] + (q & 0x7Fu);
        uint32_t m = 1u << (cb & 7u);
        if (a >= FM1_ANIM_FULL) {
            a -= FM1_ANIM_FULL;
            l[cb >> 3] |= (uint8_t)m;
        }
        if (q & FM1_ANIM_GLOW)
            d[cb >> 3] |= (uint8_t)m;
        fm1__an_acc[i] = (uint8_t)a;
    }
    if (idle)
        f = (f + 1u) & (FM1_IDLE_FRAMES - 1u);
    for (i = 0; i < FM1_NCOL; i++) {
        fm1_led_dim[i] = d[i];
        fm1_led[i] = !idle && f >= FM1_ANIM_FRAMES ? 0u : l[i];
    }
    fm1__an_f = (uint16_t)(idle ? f + 1u : f >= FM1_ANIM_FRAMES ? 0u : f + 2u);

    fm1__led_lines(FM1__LIT(0u));
}

static void fm1__anim_begin(uint32_t glow, uint32_t idle)
{
    uint32_t i, r, c;
    for (i = 0; i < FM1_NKEY; i++)
        for (r = 1; r < 5u; r++)
            for (c = 0; c < FM1_NCOL; c++)
                if (FM1_KEYMAP[r][c] == (int8_t)i)
                    fm1__an_cb[i] = (uint8_t)(c << 3 | r);
    for (i = 0; i < FM1_NKEY; i++)
        fm1__an_acc[i] = FM1_ANIM_FULL / 2u;
    for (c = 0; c < FM1_NCOL; c++)
        fm1_led[c] = fm1_led_dim[c] = fm1_led_breath[c] = fm1_led_mid[c] = 0;
    fm1__an_glow = glow != 0u;
    fm1__an_idle = idle != 0u;
    fm1__an_f = 1;
}
static void fm1_led_anim_start(uint32_t glow) { fm1__anim_begin(glow, 0); }
static int fm1_led_anim_on(void) { return fm1__an_f != 0u; }

static void fm1_led_idle_start(uint32_t glow) { fm1__anim_begin(glow, 1); }
static int fm1_led_idle_on(void) { return fm1__an_f != 0u && fm1__an_idle; }
static void fm1_led_anim_stop(void) { fm1__an_f = 0; }
static void fm1_input_tick(void)
{
    uint32_t p = fm1__tick_col, n = p + 1u == FM1_NCOL ? 0u : p + 1u;
    uint32_t t0 = fm1_ticks(), g = t0 - fm1_in_stat.last, b = 0;
    uint32_t w = 0xFFFFu ^ (1u << n) ^ (n < 2u ? 1u << (11u + n) : 0u), lit, dim = 0, k = fm1__dim_k;
    uint32_t br = 0, kb = 0;
    fm1__led_lines(0);
    fm1_in_stat.last = t0;
    while (b < FM1_GAP_BINS - 1u && g >= (150u * FM1_TICKS_PER_US << b))
        b++;
    fm1_in_stat.gap_hist[b]++;
    if (g > fm1_in_stat.gap_max)
        fm1_in_stat.gap_max = g;
    fm1_in_stat.on[p] += g;
    if (g > fm1_in_stat.on_max[p])
        fm1_in_stat.on_max[p] = g;
    fm1_in.raw[p] = (uint8_t)fm1__rows();
    if (p == 0u)
        fm1__breath_frame();
    lit = FM1__LIT(p);
#if FM1_LED_DIM_DIV > 1
    if (p == 0u)
        fm1__dim_ph = (uint8_t)(fm1__dim_ph + 1u >= FM1_LED_DIM_DIV ? 0u : fm1__dim_ph + 1u);
    if (!fm1__dim_ph)
#endif
    {
        dim = fm1_led_dim[p] & ~lit;
        if (fm1__br_on) {
            kb = (k * fm1__br_lv + fm1__br_d) >> 8;
            br = kb ? fm1_led_breath[p] & ~(lit | dim) : 0u;
        }
    }
    if (dim | br) {
        uint32_t t1, d, T = fm1__dim_t;
        t1 = fm1_ticks();
        fm1__led_lines(lit | dim | br);
        if (fm1__br_on) {
            fm1__sr_bits(w, 0, kb);
            fm1__led_lines(lit | dim);
            fm1__sr_bits(w, kb, k);
        } else {
            fm1__sr_bits(w, 0, k);
        }
        d = fm1_ticks() - t1;
        while (k == 16u && d < T)
            d = fm1_ticks() - t1;
        fm1__led_lines(0);
        fm1__dim_k = (uint8_t)(d > T ? (k ? k - 1u : 0u) : d < T && k < 16u ? k + 1u : k);
        fm1_in_stat.dim_n++;
        fm1_in_stat.dim_sum += d;
    } else {
        k = 0;
    }
    fm1__sr_bits(w, k, 16u);
    fm1__sr_latch();
    if (n == 0u)
        fm1__mid_ph = (uint8_t)(fm1__mid_ph + 1u >= fm1__mid_n ? 0u : fm1__mid_ph + 1u);
    fm1__led_lines(FM1__LIT(n));
    fm1__tick_col = (uint8_t)n;
    fm1__keys(p);
    if (n == 0u) {
        fm1__frame();
        if (fm1__an_f)
            fm1__anim_frame();
    }
    g = fm1_ticks() - t0;
    fm1_in_stat.cost_sum += g;
    fm1_in_stat.ticks++;
    if (g > fm1_in_stat.cost_max)
        fm1_in_stat.cost_max = g;
}

static inline uint32_t fm1__lock(void)
{
    __asm__ volatile("cli" ::: "memory");
    return 0;
}
static inline void fm1__unlock(uint32_t v)
{
    (void)v;
    __asm__ volatile("csync\n\tsti" ::: "memory");
}

static int32_t fm1_enc_take(uint32_t e)
{
    uint32_t k = fm1__lock();
    int32_t s = fm1_in.enc_steps[e];
    fm1_in.enc_steps[e] = 0;
    fm1__unlock(k);
    return s;
}

static uint32_t fm1_input_edges(uint32_t *released)
{
    uint32_t k = fm1__lock();
    uint32_t p = fm1_in.pressed;
    if (released)
        *released = fm1_in.released;
    fm1_in.pressed = fm1_in.released = 0;
    fm1__unlock(k);
    return p;
}

static uint32_t fm1_input_note_edges(void)
{
    uint32_t k = fm1__lock();
    uint32_t p = fm1_in.notes_pressed;
    fm1_in.notes_pressed = 0;
    fm1__unlock(k);
    return p;
}

static void fm1_led_key(uint32_t id, int on)
{
    uint32_t p, r;
    for (p = 0; p < FM1_NCOL; p++)
        for (r = 1; r < 5u; r++)
            if (FM1_KEYMAP[r][p] == (int8_t)id) {
                if (on)
                    fm1_led[p] |= (uint8_t)(1u << r);
                else
                    fm1_led[p] &= (uint8_t)~(1u << r);
            }
}

/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

#pragma once
#include <stdint.h>

#define FM1_ANIM_FRAMES 640u
#define FM1_ANIM_SWEEP 512u
#define FM1_ANIM_H0 (-24)
#define FM1_ANIM_H1 608
#define FM1_ANIM_BTN0 352u
#define FM1_ANIM_BTN_PK 496u
#define FM1_ANIM_FULL 64u
#define FM1_ANIM_GLOW 0x80u
#define FM1_ANIM_NBTN 14u
#define FM1_ANIM_NKEY 27u

static const uint8_t FM1_ANIM_X[FM1_ANIM_NKEY] = {0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 14, 15, 16, 17, 18, 19,
                                                  20, 22, 23, 24, 25, 26, 28, 29, 30};
static const uint16_t FM1_ANIM_POW[16] = {1024, 981, 939, 899, 861, 825, 790, 756,
                                          724, 693, 664, 636, 609, 583, 558, 535};

static uint32_t fm1_anim_bright(int32_t e)
{
    return e >= 176 ? 0u : (uint32_t)FM1_ANIM_POW[e & 15] >> (e >> 4);
}

static uint32_t fm1_anim_q(uint32_t b, uint32_t floor)
{
    uint32_t l = (b + 8u) >> 4;
    if (l >= 4u)
        return (l > FM1_ANIM_FULL ? FM1_ANIM_FULL : l) | FM1_ANIM_GLOW;
    return b >= 20u || floor ? FM1_ANIM_GLOW : 0u;
}

static uint32_t fm1_anim_level(uint32_t f, uint32_t id, uint32_t glow)
{
    int32_t e, d;
    if (f >= FM1_ANIM_FRAMES)
        return glow ? FM1_ANIM_GLOW : 0u;
    if (id < FM1_ANIM_NBTN) {
        if (f < FM1_ANIM_BTN0)
            return 0;
        e = f < FM1_ANIM_BTN_PK ? 96 - (int32_t)(f - FM1_ANIM_BTN0) * 64 / (int32_t)(FM1_ANIM_BTN_PK - FM1_ANIM_BTN0)
                                : 32 + (int32_t)(f - FM1_ANIM_BTN_PK) * 48 / (int32_t)(FM1_ANIM_FRAMES - FM1_ANIM_BTN_PK);
        return fm1_anim_q(fm1_anim_bright(e), glow && f >= FM1_ANIM_BTN_PK);
    }
    if (id >= FM1_ANIM_NBTN + FM1_ANIM_NKEY)
        return 0;
    d = FM1_ANIM_H0 + (int32_t)((uint32_t)(FM1_ANIM_H1 - FM1_ANIM_H0) * (f < FM1_ANIM_SWEEP ? f : FM1_ANIM_SWEEP) /
                                FM1_ANIM_SWEEP) - 16 * (int32_t)FM1_ANIM_X[id - FM1_ANIM_NBTN];
    e = d >= 0 ? (d * 3) >> 2 : (-d * 5) >> 1;
    return fm1_anim_q(fm1_anim_bright(e), glow && d > 0);
}

#define FM1_IDLE_FRAMES 8192u
#define FM1_IDLE_E0 24
static uint32_t fm1_idle_level(uint32_t f, uint32_t id, uint32_t glow)
{
    uint32_t x = f & (FM1_IDLE_FRAMES - 1u);
    int32_t d;
    x = (x & (FM1_IDLE_FRAMES / 2u) ? FM1_IDLE_FRAMES - 1u - x : x) >> 4;
    x = x * x * (768u - 2u * x) >> 16;
    if (id < FM1_ANIM_NBTN)
        return fm1_anim_q(fm1_anim_bright(96 - (int32_t)(x * 48u >> 8)), glow);
    if (id >= FM1_ANIM_NBTN + FM1_ANIM_NKEY)
        return 0;
    d = (int32_t)(x * 480u >> 8) - 16 * (int32_t)FM1_ANIM_X[id - FM1_ANIM_NBTN];
    return fm1_anim_q(fm1_anim_bright(FM1_IDLE_E0 + (d < 0 ? -d : d)), glow);
}

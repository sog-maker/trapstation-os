/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

#pragma once
#include <stdint.h>

#define FM1_T4_CON (*(volatile uint32_t *)0x10800u)
#define FM1_T4_CNT (*(volatile uint32_t *)0x10804u)
#define FM1_T4_PRD (*(volatile uint32_t *)0x10808u)
#define FM1_TICKS_PER_US 24u

static inline void fm1_time_init(void)
{
    if ((FM1_T4_CON & 0xFu) == ((2u << 2) | 1u))
        return;
    FM1_T4_CON = 0;
    FM1_T4_PRD = 0xFFFFFFFFu;
    FM1_T4_CNT = 0;
    FM1_T4_CON = (2u << 2) | 1u | (1u << 14);
}

static inline uint32_t fm1_ticks(void) { return FM1_T4_CNT; }

static inline uint32_t fm1_micros(void) { return FM1_T4_CNT / FM1_TICKS_PER_US; }

static inline void fm1_delay_us(uint32_t us)
{
    uint32_t start = FM1_T4_CNT, span = us * FM1_TICKS_PER_US;
    while ((uint32_t)(FM1_T4_CNT - start) < span)
        ;
}

static inline void fm1_delay_ms(uint32_t ms)
{
    while (ms--)
        fm1_delay_us(1000u);
}

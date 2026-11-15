/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

#pragma once
#include <stdint.h>
#include "fm1_cc.h"
#include "fm1_irq.h"

#define FM1_T5_CON (*(volatile uint32_t *)0x10900u)
#define FM1_T5_CNT (*(volatile uint32_t *)0x10904u)
#define FM1_T5_PRD (*(volatile uint32_t *)0x10908u)

FM1_INLINE void fm1_timer5_start(void (*isr)(void), uint32_t prio)
{
    FM1_T5_CON = 0x4000u;
    FM1_T5_CNT = 0;
    FM1_T5_PRD = 600u;
    fm1_irq_attach(FM1_IRQ_TIMER5, isr, prio);
    FM1_T5_CON = 0x4019u;
}

FM1_INLINE void fm1_timer5_ack(void) { FM1_T5_CON |= 0x4000u; }

/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

#pragma once
#include <stdint.h>
#include "fm1_cc.h"

#define FM1_P33_CON (*(volatile uint32_t *)0x13E08u)
#define FM1_P33_DAT (*(volatile uint32_t *)0x13E0Cu)
#define FM1_RST_SRC (*(volatile uint32_t *)0x100C0u)
#define FM1_BOOT_STATE ((volatile uint8_t *)0x01C7FD80u)
#define FM1_PWR_CON (*(volatile uint32_t *)0x10000u)
#define FM1_UPDATA_PARM ((volatile uint8_t *)0x01C7FD88u)
#define FM1_UPDATA_PARM_LEN 112u

static uint32_t fm1_p33_timeouts;

#ifndef FM1_CPU1_USED
#define FM1_CPU1_USED 0
#endif
#if FM1_CPU1_USED
#define FM1_C1_CON (*(volatile uint32_t *)0x1EEE004u)
#define FM1__DBG_WR_EN (*(volatile uint32_t *)0x1EEE240u)
#define FM1__DBG_MSG_CLR (*(volatile uint32_t *)0x1EEE248u)
#define FM1__DBG_EN (*(volatile uint32_t *)0x1EEE340u)
#define FM1__DBG_BUS 0x003F0030u
FM1_INLINE void fm1_cpu1_halt(void)
{
    uint32_t locked = !(FM1__DBG_WR_EN & 1u), en, bus = FM1__DBG_BUS;
    volatile uint32_t n;
    __asm__("" : "+r"(bus));
    if (locked)
        FM1__DBG_WR_EN = 0xE7u;
    en = FM1__DBG_EN;
    FM1__DBG_EN = en & ~bus;
    __asm__ volatile("csync" ::: "memory");
    FM1_C1_CON |= 0x2u;
    FM1_C1_CON &= ~0x8u;
    __asm__ volatile("csync" ::: "memory");
    for (n = 0; n < 256u; n++)
        ;
    FM1__DBG_MSG_CLR = FM1__DBG_BUS;
    __asm__ volatile("csync" ::: "memory");
    FM1__DBG_EN = en;
    if (locked && (FM1__DBG_WR_EN & 1u))
        FM1__DBG_WR_EN = 0xE7u;
    __asm__ volatile("csync" ::: "memory");
}
#define FM1_RESET_PREP() fm1_cpu1_halt()
#else
#define FM1_RESET_PREP() ((void)0)
#endif

static uint8_t fm1__p33_xfer(uint8_t b)
{
    uint32_t n = 100000u;
    FM1_P33_DAT = b;
    FM1_P33_CON |= 1u << 4;
    (void)FM1_P33_CON;
    while (FM1_P33_CON & (1u << 1))
        if (--n == 0) {
            fm1_p33_timeouts++;
            break;
        }
    return (uint8_t)FM1_P33_DAT;
}

static void fm1__p33_cs(uint32_t a)
{
    if (a & 0x8000u) {
        FM1_P33_CON |= (1u << 0) | (1u << 8);
    } else {
        FM1_P33_CON &= ~(1u << 8);
        FM1_P33_CON |= 1u << 0;
    }
}

static void fm1__p33_op(uint32_t op, uint32_t a, uint8_t d)
{
    fm1__p33_cs(a);
    fm1__p33_xfer((uint8_t)((op << 5) | ((a >> 8) & 3u)));
    fm1__p33_xfer((uint8_t)a);
    fm1__p33_xfer(d);
    FM1_P33_CON &= ~((1u << 0) | (1u << 8));
}

static uint8_t fm1_p33_read(uint32_t a)
{
    uint8_t v;
    fm1__p33_cs(a);
    fm1__p33_xfer((uint8_t)(0x80u | ((a >> 8) & 3u)));
    fm1__p33_xfer((uint8_t)a);
    v = fm1__p33_xfer(0x5E);
    FM1_P33_CON &= ~((1u << 0) | (1u << 8));
    return v;
}
static void fm1_p33_write(uint32_t a, uint8_t d) { fm1__p33_op(0, a, d); }
static void fm1_p33_or(uint32_t a, uint8_t d) { fm1__p33_op(1, a, d); }
static void fm1_p33_and(uint32_t a, uint8_t d) { fm1__p33_op(2, a, d); }

#define FM1_P3_WDT_CON 0x80u
#define FM1_P3_VLD_KEEP 0x17u
#define FM1_P3_RST_SRC 0x12u
#define FM1_P3_PR_PWR 0xA0u

static void fm1_wdt_arm(uint8_t t)
{
    fm1_p33_write(FM1_P3_WDT_CON, 0);
    fm1_p33_and(FM1_P3_VLD_KEEP, (uint8_t)~0x40u);
    fm1_p33_write(FM1_P3_WDT_CON, (uint8_t)((t & 0x0Fu) | 0x40u));
    fm1_p33_or(FM1_P3_WDT_CON, 0x10);
}
static void fm1_wdt_feed(void) { fm1_p33_or(FM1_P3_WDT_CON, 0x40); }
static void fm1_wdt_stop(void) { fm1_p33_write(FM1_P3_WDT_CON, 0); }

static struct {
    uint8_t p3_rst;
    uint32_t rst_src;
    uint8_t wdt_con;
} fm1_boot;

static void fm1_reset_reason(void)
{
    fm1_boot.p3_rst = fm1_p33_read(FM1_P3_RST_SRC);
    fm1_boot.rst_src = FM1_RST_SRC;
    fm1_boot.wdt_con = fm1_p33_read(FM1_P3_WDT_CON);
}

static void fm1_reboot(void)
{
    __asm__ volatile("cli");
    FM1_RESET_PREP();
    fm1_p33_or(FM1_P3_PR_PWR, 0x10);
    for (;;)
        ;
}

static void fm1_enter_uboot(void)
{
    static const char k[16] = "usb_update_mode";
    uint32_t i;
    __asm__ volatile("cli");
    FM1_RESET_PREP();

    if (!(*(volatile uint32_t *)0x1EEE240u & 1u))
        *(volatile uint32_t *)0x1EEE240u = 0xE7u;
    *(volatile uint32_t *)0x1EEE348u = 0;
    for (i = 0; i < 16u; i++)
        FM1_BOOT_STATE[i] = (uint8_t)k[i];
    fm1_p33_or(FM1_P3_PR_PWR, 0x10);
    for (;;)
        ;
}

FM1_INLINE void fm1_core_reset(void)
{
    FM1_RESET_PREP();
    FM1_PWR_CON |= 0x10u;
    for (;;)
        ;
}

FM1_INLINE void fm1_enter_update(const uint8_t *parm)
{
    uint32_t i;
    __asm__ volatile("cli");
    FM1_RESET_PREP();
    if (!(*(volatile uint32_t *)0x1EEE240u & 1u))
        *(volatile uint32_t *)0x1EEE240u = 0xE7u;
    *(volatile uint32_t *)0x1EEE348u = 0;
    for (i = 0; i < FM1_UPDATA_PARM_LEN; i++)
        FM1_UPDATA_PARM[i] = parm[i];
    fm1_core_reset();
}

FM1_INLINE void fm1_updata_parm_clear(void)
{
    uint32_t i;
    for (i = 0; i < FM1_UPDATA_PARM_LEN; i++)
        FM1_UPDATA_PARM[i] = 0;
}

FM1_INLINE void fm1_mailbox_clear(void)
{
    uint32_t *s;
    for (s = (uint32_t *)0x01C7FD80u; s < (uint32_t *)0x01C7FE00u; s++)
        *s = 0;
}

static int fm1_mem_readable(uint32_t a, uint32_t n)
{
    return (a >= 0x01C00000u && a + n <= 0x01C80000u && a + n >= a) ||
           (a >= 0x02000000u && a + n <= 0x02100000u && a + n >= a);
}
FM1_INLINE uint8_t fm1_peek8(uint32_t a) { return *(const volatile uint8_t *)a; }

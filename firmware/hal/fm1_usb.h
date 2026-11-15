/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

#pragma once
#include <stdint.h>
#include "fm1_cc.h"

#define FM1_USB_CON0 (*(volatile uint32_t *)0x11800u)
#define FM1_USB_CON1 (*(volatile uint32_t *)0x11804u)
#define FM1_USB_EP_CNT(n) (*(volatile uint32_t *)(0x11808u + 4u * (n)))
#define FM1_USB_EP0_ADR (*(volatile uint32_t *)0x11818u)
#define FM1_USB_EP_TADR(n) (*(volatile uint32_t *)(0x1181Cu + 8u * ((n) - 1u)))
#define FM1_USB_EP_RADR(n) (*(volatile uint32_t *)(0x11820u + 8u * ((n) - 1u)))
#define FM1_USB_EP4_CNT (*(volatile uint32_t *)0x11834u)
#define FM1_USB_EP4_TADR (*(volatile uint32_t *)0x11838u)
#define FM1_USB_EP4_RADR (*(volatile uint32_t *)0x1183Cu)
#define FM1_USB_IO_CON0 (*(volatile uint32_t *)0x51000u)
#define FM1_CLK_CON1 (*(volatile uint32_t *)0x10010u)

FM1_INLINE void fm1_usb_reset(void)
{
    FM1_USB_CON0 = 0;
    FM1_USB_CON1 = 0;
    FM1_USB_IO_CON0 = 0x0Cu;
}

FM1_INLINE void fm1_usb_attach(void *ep0)
{
    uint32_t n;
    FM1_CLK_CON1 &= ~3u;
    FM1_USB_EP0_ADR = (uint32_t)(uintptr_t)ep0;
    for (n = 1; n <= 3u; n++)
        FM1_USB_EP_RADR(n) = (uint32_t)(uintptr_t)ep0;
    FM1_USB_EP4_RADR = (uint32_t)(uintptr_t)ep0;
    FM1_USB_IO_CON0 |= (1u << 10) | (1u << 9);
    FM1_USB_IO_CON0 &= ~(1u << 11);
    FM1_USB_IO_CON0 |= 1u << 12;
    FM1_USB_CON0 &= ~0x4035u;
    FM1_USB_CON1 = 0;
    FM1_USB_CON0 |= 1u;
    FM1_USB_IO_CON0 &= ~0xF0u;
    FM1_USB_IO_CON0 |= 0x40u;
    FM1_USB_CON0 |= 0x3Cu;
}

FM1_INLINE void fm1_usb_off(void)
{
    FM1_USB_CON0 = 0;
    FM1_USB_IO_CON0 = 0x0Cu;
}

FM1_INLINE uint32_t fm1_usb_sie_on(void) { return FM1_USB_CON0 & 4u; }
FM1_INLINE void fm1_usb_sie_wr_start(uint32_t r, uint32_t v)
{
    FM1_USB_CON1 = ((r << 8) & 0xFFFF00u) | (v & 0xFFu);
    __asm__ volatile("csync" ::: "memory");
}
FM1_INLINE void fm1_usb_sie_rd_start(uint32_t r)
{
    FM1_USB_CON1 = ((r << 8) & 0xFFBF00u) | 0x4000u;
    __asm__ volatile("csync" ::: "memory");
}
FM1_INLINE uint32_t fm1_usb_sie_done(void) { return FM1_USB_CON1 & 0x8000u; }
FM1_INLINE uint32_t fm1_usb_sie_data(void) { return FM1_USB_CON1 & 0xFFu; }

FM1_INLINE void fm1_usb_ep0_buf(void *p) { FM1_USB_EP0_ADR = (uint32_t)(uintptr_t)p; }
FM1_INLINE void fm1_usb_ep_txbuf(uint32_t ep, void *p) { FM1_USB_EP_TADR(ep) = (uint32_t)(uintptr_t)p; }
FM1_INLINE void fm1_usb_ep_rxbuf(uint32_t ep, void *p) { FM1_USB_EP_RADR(ep) = (uint32_t)(uintptr_t)p; }
FM1_INLINE void fm1_usb_ep0_send(void *p, uint32_t n)
{
    __asm__ volatile("csync" ::: "memory");
    FM1_USB_EP0_ADR = (uint32_t)(uintptr_t)p;
    FM1_USB_EP_CNT(0) = n;
}
FM1_INLINE void fm1_usb_ep_send(uint32_t ep, void *p, uint32_t n)
{
    __asm__ volatile("csync" ::: "memory");
    FM1_USB_EP_TADR(ep) = (uint32_t)(uintptr_t)p;
    FM1_USB_EP_CNT(ep) = n;
}

FM1_INLINE void fm1_usb_ep4_txbuf(void *p) { FM1_USB_EP4_TADR = (uint32_t)(uintptr_t)p; }
FM1_INLINE void fm1_usb_ep4_send(void *p, uint32_t n)
{
    __asm__ volatile("csync" ::: "memory");
    FM1_USB_EP4_TADR = (uint32_t)(uintptr_t)p;
    FM1_USB_EP4_CNT = n;
    __asm__ volatile("csync" ::: "memory");
}
FM1_INLINE void fm1_usb_rx_sync(void) { __asm__ volatile("ssync" ::: "memory"); }

FM1_INLINE void fm1_usb_ep_enable(uint32_t eps) { FM1_USB_CON0 &= ~(eps << 19); }

FM1_INLINE uint32_t fm1_usb_sof_take(void)
{
    if (!(FM1_USB_CON0 & (1u << 13)))
        return 0;
    FM1_USB_CON0 |= 1u << 12;
    return 1;
}

/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

#include "../hal/fm1_usb.h"
#ifndef FELUCCA_CDC
#define FELUCCA_CDC 0
#endif
#ifndef FELUCCA_UAC
#define FELUCCA_UAC 0
#endif
#ifndef FELUCCA_UAC_48K
#define FELUCCA_UAC_48K 1
#endif
#ifndef FELUCCA_UAC_AS_FIRST
#define FELUCCA_UAC_AS_FIRST 1
#endif
#if !FELUCCA_UAC
#undef FELUCCA_UAC_48K
#define FELUCCA_UAC_48K 0
#undef FELUCCA_UAC_AS_FIRST
#define FELUCCA_UAC_AS_FIRST 0
#endif
enum { S_FADDR = 0, S_POWER = 1, S_INTRTX1 = 2, S_INTRTX2 = 3, S_INTRRX1 = 4, S_INTRRX2 = 5, S_INTRUSB = 6,
       S_INTRTX1E = 7, S_INTRTX2E = 8, S_INTRRX1E = 9, S_INTRRX2E = 10, S_INTRUSBE = 11, S_FRAME1 = 12,
       S_FRAME2 = 13, S_INDEX = 14,
       S_TXMAXP = 16, S_CSR0 = 17, S_TXCSR1 = 17, S_TXCSR2 = 18, S_RXMAXP = 19, S_RXCSR1 = 20, S_RXCSR2 = 21,
       S_COUNT0 = 22, S_RXCOUNT1 = 22, S_RXCOUNT2 = 23 };

static uint8_t ep0buf[64 + 4] __attribute__((aligned(4)));
static uint8_t ep1tx[64] __attribute__((aligned(4)));
static uint8_t ep1rx[64 + 4] __attribute__((aligned(4)));
#if FELUCCA_CDC
static uint8_t ep2tx[8] __attribute__((aligned(4)));
static uint8_t ep3tx[64] __attribute__((aligned(4)));
static uint8_t ep3rx[64 + 4] __attribute__((aligned(4)));

#define CI_N 256u
#define CO_N 8192u
static uint8_t cdc_in[CI_N], cdc_out[CO_N];
static volatile uint32_t ci_w, ci_r, co_w, co_r;
static struct {
    uint8_t line[7];
    volatile uint8_t dtr;
    uint8_t e0_rx;
    uint8_t rx_pend;
    uint32_t rx_pkts, tx_pkts;
} cdc = {{0x00, 0xC2, 0x01, 0x00, 0, 0, 8}};
#endif

#if FELUCCA_UAC

#define UA_N 512u
#define UA_PRIME 104u
#define UA_LO 80u
#define UA_HI 128u
#define UAC_RATE 44100u
#if FELUCCA_UAC_48K

#define UA_PRIME48 113u
#define UA_LO48 87u
#define UA_HI48 139u
#define UA_REND48 ((HALF_FRAMES * 160u + 146u) / 147u)
#define UA_MAXF 49u
_Static_assert(UA_HI48 + UA_REND48 + UA_MAXF < UA_N, "the ring holds the band, a render and a packet (48 kHz)");
#else
#define UA_MAXF 46u
#endif
#define UAC_MAXP (UA_MAXF * 4u)
_Static_assert(UA_HI + HALF_FRAMES + UA_MAXF < UA_N, "the ring holds the band, a render and a packet");
static uint32_t ua_ring[UA_N];
static volatile uint32_t ua_w, ua_r;
static uint32_t ep4tx[UA_MAXF] __attribute__((aligned(4)));
static struct {
    volatile uint8_t alt;
    volatile uint8_t r48;
    uint8_t ring48;
    volatile uint8_t go;
    volatile uint8_t flowing;
    uint8_t queued;
    uint8_t feed;
    uint8_t wait;
    uint8_t e0_rx;
    volatile uint32_t fill_min;
    uint32_t acc;
    uint32_t last;
    uint32_t starts, pkts, frames;
    uint32_t underruns, overruns, missed;
    uint32_t stalls;
    uint32_t adj_up, adj_down;
    uint32_t fill_lo, fill_hi;
    uint32_t rate_sets;
} uac;
#if FELUCCA_UAC_48K
#include "uac_fir.h"

#define RS_H 32u
_Static_assert(RS_H >= UAC_FIR_N && (RS_H & (RS_H - 1u)) == 0, "the history holds a window");
static struct {
    int16_t l[2u * RS_H], r[2u * RS_H];
    uint32_t w;
    uint32_t ph;
#if FELUCCA_UAC_TONE
    uint32_t tone;
#endif
} rs;
#endif
#endif

static struct {
    uint8_t up, config, pend_addr, has_pend_addr, e0_tx, e0_zlp;
    const uint8_t *e0_src;
    uint16_t e0_left;
    uint32_t resets, setups, rx_pkts, tx_pkts, sof_seen, timeouts, no_sof;
    uint32_t suspends, max_gap, retries, frame_stalls;
    uint16_t frame, frame_same;
    uint8_t detached;
    volatile uint8_t suspended;
    uint8_t last_setup[8];
    uint8_t sysex[16];
    uint8_t sx_len, sx_on;
    uint8_t rx_pend;
    volatile uint8_t uboot_req;
    volatile uint8_t ota_req;
} usb;

#if FELUCCA_OTA

static uint8_t sx_frame[640];
static uint32_t sx_pos;
static volatile uint32_t sx_frame_len;
static volatile uint8_t sx_ready, sx_collect, sx_busy;
#define SXQ 512u
static uint32_t sx_out_q[SXQ];
static volatile uint32_t so_w, so_r;
#endif

#define MQ 64u
static uint32_t midi_in_q[MQ], midi_out_q[MQ];
static uint32_t midi_in_ms[MQ];
static uint8_t midi_in_source[MQ];
static volatile uint32_t fm1_ms;
static volatile uint32_t mi_w, mi_r, mo_w, mo_r;
static volatile uint8_t midi_in_overflow;

static int midi_enqueue(uint32_t pkt, uint32_t source)
{
    uint32_t at = mi_w % MQ;
    if (midi_in_overflow)
        return 0;
    if (mi_w - mi_r >= MQ) {
        midi_in_overflow = 1;
        return 0;
    }
    midi_in_q[at] = pkt;
    midi_in_ms[at] = fm1_ms;
    midi_in_source[at] = (uint8_t)source;
    RING_PUBLISH();
    mi_w++;
    return 1;
}

static void midi_out_event(uint32_t pkt)
{
    if (usb.config && mo_w - mo_r < MQ) {
        midi_out_q[mo_w % MQ] = pkt;
        RING_PUBLISH();
        mo_w++;
    }
}

#ifndef FELUCCA_USB_LAYOUT
#define FELUCCA_USB_LAYOUT 0
#endif
#ifndef FELUCCA_CDC_DEFAULT
#define FELUCCA_CDC_DEFAULT 1
#endif
#ifndef FELUCCA_USB_PID
#define FELUCCA_USB_PID 0x0001
#endif
#if FELUCCA_USB_LAYOUT < 0 || FELUCCA_USB_LAYOUT > 3
#error "FELUCCA_USB_LAYOUT is 0..3"
#endif

#define D_CFG(len, nif) 9, 2, (len) & 0xFF, (len) >> 8, (nif), 1, 0, 0x80, 50
#define D_IAD(first, n, cls, sub, proto) 8, 0x0B, (first), (n), (cls), (sub), (proto), 0
#if FELUCCA_UAC_48K
#define D_FMT 14, 0x24, 2, 1, 2, 2, 16, 2, 0x44, 0xAC, 0x00, 0x80, 0xBB, 0x00,
#define FMT_LEN 14
#elif FELUCCA_UAC
#define D_FMT 11, 0x24, 2, 1, 2, 2, 16, 1, UAC_RATE & 0xFF, (UAC_RATE >> 8) & 0xFF, UAC_RATE >> 16,
#define FMT_LEN 11
#else
#define FMT_LEN 0
#endif
#if FELUCCA_UAC

#if FELUCCA_UAC_AS_FIRST
#define AC_COLL(a) (a) + 2, (a) + 1
#else
#define AC_COLL(a) (a) + 1, (a) + 2
#endif
#define D_AC(a)                                                                                      \
    9, 4, (a), 0, 0, 1, 1, 0, 0,                                                                     \
    10, 0x24, 1, 0x00, 0x01, 31, 0, 2, AC_COLL(a),           \
    12, 0x24, 2, 1, 0x03, 0x06, 0, 2, 0x03, 0x00, 0, 0,           \
    9, 0x24, 3, 2, 0x01, 0x01, 0, 1, 0,
#define D_AS(s)                                                                                      \
    9, 4, (s), 0, 0, 1, 2, 0, 0,                                \
    9, 4, (s), 1, 1, 1, 2, 0, 0,                                                  \
    7, 0x24, 1, 2, 1, 0x01, 0x00,                              \
    D_FMT                                                           \
    9, 5, 0x84, 0x05, UAC_MAXP & 0xFF, UAC_MAXP >> 8, 1, 0, 0,       \
    7, 0x25, 1, 0x01, 0, 0, 0,
#else
#define D_AC(a) 9, 4, (a), 0, 0, 1, 1, 0, 0, 9, 0x24, 1, 0x00, 0x01, 9, 0, 1, (a) + 1,
#define D_AS(s)
#endif
#define D_MIDI(m)                                                                                    \
    9, 4, (m), 0, 2, 1, 3, 0, 0,                                                                     \
    7, 0x24, 1, 0x00, 0x01, 0x41, 0x00,                                                              \
    6, 0x24, 2, 1, 1, 0,                                                                             \
    6, 0x24, 2, 2, 2, 0,                                                                             \
    9, 0x24, 3, 1, 3, 1, 2, 1, 0,                                                                    \
    9, 0x24, 3, 2, 4, 1, 1, 1, 0,                                                                    \
    9, 5, 0x01, 2, 64, 0, 0, 0, 0,                                                                   \
    5, 0x25, 1, 1, 1,                                                                                \
    9, 5, 0x81, 2, 64, 0, 0, 0, 0,                                                                   \
    5, 0x25, 1, 1, 3,
#define D_AUDIO(a) D_AC(a) D_MIDI((a) + 1) D_AS((a) + 2)
#define AUD_NIF (2 + FELUCCA_UAC)
#define AUD_LEN (92 + (63 + FMT_LEN) * FELUCCA_UAC)
#define D_CDC(c)                                                                                     \
    D_IAD((c), 2, 2, 2, 1),                                                            \
    9, 4, (c), 0, 1, 2, 2, 1, 0,                                                      \
    5, 0x24, 0x00, 0x10, 0x01,                                                          \
    5, 0x24, 0x01, 0x00, (c) + 1,                                      \
    4, 0x24, 0x02, 0x02,                                                   \
    5, 0x24, 0x06, (c), (c) + 1,                                                              \
    7, 5, 0x82, 3, 8, 0, 16,                                          \
    9, 4, (c) + 1, 0, 2, 0x0A, 0, 0, 0,                                                        \
    7, 5, 0x03, 2, 64, 0, 0,                                                           \
    7, 5, 0x83, 2, 64, 0, 0,
#define CDC_LEN 66
#define BCD_LO(cdc)                                                                                  \
    (0x10 * (FELUCCA_UAC + FELUCCA_UAC_48K) + 0x08 * FELUCCA_UAC_AS_FIRST + ((cdc) ? 0x01 + 2 * FELUCCA_USB_LAYOUT : 0))
#define D_DEV(bcdusb, cls, sub, proto, cdc)                                                          \
    18, 1, (bcdusb) & 0xFF, (bcdusb) >> 8, (cls), (sub), (proto), 64, 0x09, 0x12, FELUCCA_USB_PID & 0xFF, \
    FELUCCA_USB_PID >> 8, BCD_LO(cdc), 0x03, 1, 2, 0, 1

#define CFG_LEN_PLAIN (9 + AUD_LEN)
static const uint8_t DEV_DESC_PLAIN[18] = {D_DEV(0x0110, 0, 0, 0, 0)};
static const uint8_t CFG_DESC_PLAIN[] = {D_CFG(CFG_LEN_PLAIN, AUD_NIF), D_AUDIO(0)};
typedef char cfg_plain_len_ok[sizeof CFG_DESC_PLAIN == CFG_LEN_PLAIN ? 1 : -1];
#if FELUCCA_CDC
#define CFG_LEN (9 + 8 + AUD_LEN + CDC_LEN)
#define USB_NIF (AUD_NIF + 2)
#if FELUCCA_USB_LAYOUT == 1
#define AUD_IF_CDC 2u
#define CDC_IF_CDC 0u
#else
#define AUD_IF_CDC 0u
#define CDC_IF_CDC AUD_NIF
#endif
#if FELUCCA_USB_LAYOUT == 2
static const uint8_t DEV_DESC[18] = {D_DEV(0x0200, 0x00, 0x00, 0x01, 1)};
#elif FELUCCA_USB_LAYOUT == 3
static const uint8_t DEV_DESC[18] = {D_DEV(0x0200, 0x00, 0x00, 0x00, 1)};
#else
static const uint8_t DEV_DESC[18] = {D_DEV(0x0200, 0xEF, 0x02, 0x01, 1)};
#endif
static const uint8_t CFG_DESC[] = {
    D_CFG(CFG_LEN, USB_NIF),
#if FELUCCA_USB_LAYOUT == 1
    D_CDC(CDC_IF_CDC)
#endif
    D_IAD(AUD_IF_CDC, AUD_NIF, 1, 1, 0),
    D_AUDIO(AUD_IF_CDC)
#if FELUCCA_USB_LAYOUT != 1
    D_CDC(CDC_IF_CDC)
#endif
};
typedef char cfg_len_ok[sizeof CFG_DESC == CFG_LEN ? 1 : -1];
static uint8_t usb_cdc_on = FELUCCA_CDC_DEFAULT != 0;
#define USB_CDC_ON usb_cdc_on
#define AUD_IF (usb_cdc_on ? AUD_IF_CDC : 0u)
#else
#define DEV_DESC DEV_DESC_PLAIN
#define CFG_DESC CFG_DESC_PLAIN
#define USB_CDC_ON 0
#define AUD_IF 0u
#endif
#define UAC_AS_IF (AUD_IF + 2u)
static const uint8_t STR0[4] = {4, 3, 0x09, 0x04};

static const uint8_t STR1[] = {40, 3, 'S', 0, 'c', 0, 'h', 0, 'l', 0, 'e', 0, 'n', 0, 's', 0, 'o', 0, 'g', 0, ' ', 0,
                               'A', 0, 'u', 0, 'd', 0, 'i', 0, 'o', 0, ' ', 0, 'L', 0, 'a', 0, 'b', 0};
#ifdef FELUCCA_LOADER
static const uint8_t STR2[] = {48, 3, 'T', 0, 'R', 0, 'A', 0, 'P', 0, 'S', 0, 'T', 0, 'A', 0, 'T', 0, 'I', 0, 'O', 0,
                               'N', 0, ' ', 0, 'F', 0, 'M', 0, '-', 0, '1', 0, ' ', 0, 'U', 0, 'p', 0, 'd', 0,
                               'a', 0, 't', 0, 'e', 0};
#else
static const uint8_t STR2[] = {34, 3, 'T', 0, 'R', 0, 'A', 0, 'P', 0, 'S', 0, 'T', 0, 'A', 0, 'T', 0, 'I', 0, 'O', 0,
                               'N', 0, ' ', 0, 'F', 0, 'M', 0, '-', 0, '1', 0};
#endif

static int get_desc(uint32_t wvalue, const uint8_t **d, uint16_t *l)
{
    switch (wvalue >> 8) {
    case 1:
        *d = USB_CDC_ON ? DEV_DESC : DEV_DESC_PLAIN;
        *l = 18;
        return 1;
    case 2:
        *d = USB_CDC_ON ? CFG_DESC : CFG_DESC_PLAIN;
        *l = (uint16_t)(USB_CDC_ON ? sizeof CFG_DESC : sizeof CFG_DESC_PLAIN);
        return 1;
    case 3:
        switch (wvalue & 0xFFu) {
        case 0:
            *d = STR0;
            *l = sizeof STR0;
            return 1;
        case 1:
            *d = STR1;
            *l = sizeof STR1;
            return 1;
        case 2:
            *d = STR2;
            *l = sizeof STR2;
            return 1;
        default:
            return 0;
        }
    default:
        return 0;
    }
}

static void sie_wr(uint32_t r, uint32_t v)
{
    uint32_t n = 20000;
    if (!fm1_usb_sie_on())
        return;
    fm1_usb_sie_wr_start(r, v);
    while (!fm1_usb_sie_done() && --n)
        ;
    if (n)
        usb.timeouts = 0;
    else if (++usb.timeouts > 50u)
        usb.up = 0;
}

static uint32_t sie_rd(uint32_t r)
{
    uint32_t n = 20000;
    if (!fm1_usb_sie_on())
        return 0;
    fm1_usb_sie_rd_start(r);
    while (!fm1_usb_sie_done())
        if (!--n) {
            if (++usb.timeouts > 50u)
                usb.up = 0;
            return 0;
        }
    usb.timeouts = 0;
    return fm1_usb_sie_data();
}

#if FELUCCA_UAC
static void uac_ep4_reset(void)
{
    sie_wr(S_INDEX, 4);
    sie_wr(S_TXCSR1, 0x48);
    sie_wr(S_TXCSR2, 0);
    sie_wr(S_TXCSR1, 0);
    sie_wr(S_TXCSR2, 0x40);
}

static void uac_stream(uint32_t alt)
{
    uac.go = 0;
    uac.flowing = 0;
    uac.queued = 0;
    uac.wait = 0;
    RING_PUBLISH();
    uac.alt = (uint8_t)alt;
    uac.acc = 0;
    uac.last = 0;
    if (alt) {
        uac.starts++;
        uac.fill_lo = 0xFFFFFFFFu;
        uac.fill_hi = 0;
    }
}

static void uac_rate_set(uint32_t hz)
{
    uint8_t r48 = FELUCCA_UAC_48K && hz >= (UAC_RATE + 48000u) / 2u;
    if (r48 == uac.r48)
        return;
    uac.go = 0;
    RING_PUBLISH();
    uac.r48 = r48;
    uac.acc = 0;
    uac.pkts = uac.frames = 0;
}
#endif

static void ep1_config(void)
{
    fm1_usb_ep_txbuf(1, ep1tx);
    fm1_usb_ep_rxbuf(1, ep1rx);
    sie_wr(S_INDEX, 1);
    sie_wr(S_TXMAXP, 0xFF);
    sie_wr(S_TXCSR1, 0x48);
    sie_wr(S_TXCSR2, 0);
    sie_wr(S_RXMAXP, 0xFF);
    sie_wr(S_RXCSR1, 0x90);
    sie_wr(S_RXCSR2, 0);
    sie_wr(S_INTRRX1E, 0x02);
    fm1_usb_ep_enable(1u << 1);
#if FELUCCA_CDC
    if (!usb_cdc_on)
        goto no_cdc;
    fm1_usb_ep_txbuf(2, ep2tx);
    sie_wr(S_INDEX, 2);
    sie_wr(S_TXMAXP, 0xFF);
    sie_wr(S_TXCSR1, 0x48);
    sie_wr(S_TXCSR2, 0);
    fm1_usb_ep_txbuf(3, ep3tx);
    fm1_usb_ep_rxbuf(3, ep3rx);
    sie_wr(S_INDEX, 3);
    sie_wr(S_TXMAXP, 0xFF);
    sie_wr(S_TXCSR1, 0x48);
    sie_wr(S_TXCSR2, 0);
    sie_wr(S_RXMAXP, 0xFF);
    sie_wr(S_RXCSR1, 0x90);
    sie_wr(S_RXCSR2, 0);
    sie_wr(S_INTRRX1E, 0x0A);
    fm1_usb_ep_enable((1u << 2) | (1u << 3));
    cdc.rx_pend = 1;
no_cdc:;
#endif
#if FELUCCA_UAC
    fm1_usb_ep4_txbuf(ep4tx);
    sie_wr(S_INDEX, 4);
    sie_wr(S_TXMAXP, 0xFF);
    uac_ep4_reset();
    fm1_usb_ep_enable(1u << 4);
    uac_stream(0);
#endif
}

static void e0_chunk(void)
{
    uint32_t n = usb.e0_left > 64u ? 64u : usb.e0_left, i;
    int last;
    for (i = 0; i < n; i++)
        ep0buf[i] = usb.e0_src[i];
    fm1_usb_ep0_send(ep0buf, n);
    usb.e0_src += n;
    usb.e0_left = (uint16_t)(usb.e0_left - n);
    last = usb.e0_left == 0 && !(n == 64u && usb.e0_zlp);
    if (usb.e0_left == 0 && n == 64u)
        usb.e0_zlp = 0;
    sie_wr(S_INDEX, 0);
    sie_wr(S_CSR0, last ? 0x0A : 0x02);
    if (last)
        usb.e0_tx = 0;
}

static void e0_send(const uint8_t *p, uint16_t len, uint16_t wlen)
{
    if (len > wlen)
        len = wlen;
    usb.e0_src = p;
    usb.e0_left = len;
    usb.e0_zlp = (len < wlen) && (len % 64u == 0);
    usb.e0_tx = 1;
    sie_wr(S_INDEX, 0);
    sie_wr(S_CSR0, 0x40);
    e0_chunk();
}

static void ep0_service(void)
{
    static const uint8_t zero2[2];
    uint32_t csr, i;
    uint16_t wvalue, wlength;
    uint8_t *s = usb.last_setup;
    if (usb.has_pend_addr) {
        sie_wr(S_FADDR, usb.pend_addr);
        usb.has_pend_addr = 0;
    }
    sie_wr(S_INDEX, 0);
    csr = sie_rd(S_CSR0);
    if (csr & 0x04u) {
        sie_wr(S_CSR0, 0);
        usb.e0_tx = 0;
#if FELUCCA_CDC
        cdc.e0_rx = 0;
#endif
#if FELUCCA_UAC
        uac.e0_rx = 0;
#endif
        return;
    }
    if (csr & 0x10u) {
        sie_wr(S_CSR0, 0x80);
        usb.e0_tx = 0;
#if FELUCCA_CDC
        cdc.e0_rx = 0;
#endif
#if FELUCCA_UAC
        uac.e0_rx = 0;
#endif
    }
    if (usb.e0_tx) {
        if (!(csr & 0x02u))
            e0_chunk();
        return;
    }
    if (!(csr & 0x01u))
        return;
    fm1_usb_rx_sync();
#if FELUCCA_CDC
    if (cdc.e0_rx) {
        uint32_t n = sie_rd(S_COUNT0);
        for (i = 0; i < 7u && i < n; i++)
            cdc.line[i] = ep0buf[i];
        cdc.e0_rx = 0;
        goto ack;
    }
#endif
#if FELUCCA_UAC
    if (uac.e0_rx) {
        uac.rate_sets++;
        if (sie_rd(S_COUNT0) >= 3u)
            uac_rate_set(ep0buf[0] | (uint32_t)ep0buf[1] << 8 | (uint32_t)ep0buf[2] << 16);
        uac.e0_rx = 0;
        goto ack;
    }
#endif
    for (i = 0; i < 8u; i++)
        s[i] = ep0buf[i];
    usb.setups++;
    wvalue = (uint16_t)(s[2] | s[3] << 8);
    wlength = (uint16_t)(s[6] | s[7] << 8);
    switch ((uint32_t)s[0] << 8 | s[1]) {
    case 0x0005:
        usb.pend_addr = s[2] & 0x7Fu;
        usb.has_pend_addr = 1;
        goto ack;
    case 0x8006: {
        const uint8_t *d;
        uint16_t l;
        if (get_desc(wvalue, &d, &l)) {
            if (!wlength)
                goto ack;
            e0_send(d, l, wlength);
            return;
        }
        goto stall;
    }
    case 0x0009:
        if (s[2] > 1u)
            goto stall;
        usb.config = s[2];
        if (usb.config == 1u)
            ep1_config();
        else
            sie_wr(S_INTRRX1E, 0);
#if FELUCCA_UAC
        if (usb.config != 1u)
            uac_stream(0);
        uac_rate_set(UAC_RATE);
#endif
        goto ack;
    case 0x8008:
        e0_send(&usb.config, 1, wlength);
        return;
    case 0x8000:
    case 0x8100:
    case 0x8200:
        e0_send(zero2, 2, wlength);
        return;
    case 0x010B:
#if FELUCCA_UAC
        if (s[4] == UAC_AS_IF && wvalue <= 1u && usb.config) {
            uac_ep4_reset();
            uac_stream(wvalue);
            goto ack;
        }
#endif
        if (wvalue == 0)
            goto ack;
        goto stall;
    case 0x810A:
#if FELUCCA_UAC
        if (s[4] == UAC_AS_IF) {
            static uint8_t alt;
            alt = uac.alt;
            e0_send(&alt, 1, wlength);
            return;
        }
#endif
        e0_send(zero2, 1, wlength);
        return;
    case 0x0201: {
        uint32_t ep = s[4] & 0x0Fu, last = USB_CDC_ON ? 3u : 1u;
#if FELUCCA_UAC
        if (wvalue == 0 && s[4] == 0x84u)
            goto ack;
#endif
        if (wvalue != 0 || ep > last)
            goto stall;
        if (ep) {
            sie_wr(S_INDEX, ep);
            if (s[4] & 0x80u)
                sie_wr(S_TXCSR1, 0x40);
            else
                sie_wr(S_RXCSR1, 0x80);
        }
        goto ack;
    }
#if FELUCCA_CDC
    case 0x2120:
        if (!usb_cdc_on)
            goto stall;
        if (wlength) {
            cdc.e0_rx = 1;
            sie_wr(S_INDEX, 0);
            sie_wr(S_CSR0, 0x40);
            return;
        }
        goto ack;
    case 0xA121:
        if (!usb_cdc_on)
            goto stall;
        e0_send(cdc.line, 7, wlength);
        return;
    case 0x2122:
        if (!usb_cdc_on)
            goto stall;
        cdc.dtr = s[2] & 1u;
        goto ack;
    case 0x2123:
        if (!usb_cdc_on)
            goto stall;
        goto ack;
#endif
#if FELUCCA_UAC
    case 0x2201:
        if (s[4] != 0x84u || s[3] != 1u)
            goto stall;
        if (wlength) {
            uac.e0_rx = 1;
            sie_wr(S_INDEX, 0);
            sie_wr(S_CSR0, 0x40);
            return;
        }
        goto ack;
    case 0xA281:
    case 0xA282:
    case 0xA283: {
        static const uint8_t RATE[3] = {UAC_RATE & 0xFF, (UAC_RATE >> 8) & 0xFF, UAC_RATE >> 16};
#if FELUCCA_UAC_48K
        static const uint8_t RATE48[3] = {48000u & 0xFF, (48000u >> 8) & 0xFF, 48000u >> 16};
#endif
        if (s[4] != 0x84u || s[3] != 1u)
            goto stall;
#if FELUCCA_UAC_48K
        if (s[1] == 0x83u || (s[1] == 0x81u && uac.r48)) {
            e0_send(RATE48, 3, wlength);
            return;
        }
#endif
        e0_send(RATE, 3, wlength);
        return;
    }
#endif
    default:
        goto stall;
    }
ack:
    sie_wr(S_INDEX, 0);
    sie_wr(S_CSR0, 0x48);
    return;
stall:
    sie_wr(S_INDEX, 0);
    sie_wr(S_CSR0, 0x60);
}

static void sysex_byte(uint8_t b)
{
    static const uint8_t UBOOT_KEY[6] = {0xF0, 0x22, 0x24, 0x35, 0x7D, 0xF7};
    if (b >= 0xF8u)
        return;
    if ((b & 0x80u) && b != 0xF0u && b != 0xF7u) {
        usb.sx_on = 0;
#if FELUCCA_OTA
        sx_collect = 0;
#endif
        return;
    }
    if (b == 0xF0) {
        usb.sx_on = 1;
        usb.sx_len = 0;
#if FELUCCA_OTA
        sx_collect = !sx_ready;
        sx_pos = 0;
#endif
    }
    if (!usb.sx_on)
        return;
#if FELUCCA_OTA
    if (sx_collect && b != 0xF0 && b != 0xF7) {
        if (sx_pos < sizeof sx_frame)
            sx_frame[sx_pos++] = b;
        else
            sx_collect = 0;
    }
#endif
    if (usb.sx_len < sizeof usb.sysex)
        usb.sysex[usb.sx_len++] = b;
    if (b == 0xF7) {
        uint32_t i, ok = usb.sx_len == 6u;
        for (i = 0; ok && i < 6u; i++)
            ok = usb.sysex[i] == UBOOT_KEY[i];
        if (ok)
            usb.uboot_req = 1;
#if FELUCCA_OTA
        else if (usb.sx_len == 6u && usb.sysex[1] == 0x22 && usb.sysex[2] == 0x24 && usb.sysex[3] == 0x35 &&
                 usb.sysex[4] == 0x7F)
            usb.ota_req = 1;
        else if (sx_collect && sx_pos) {
            sx_frame_len = sx_pos;
            RING_PUBLISH();
            sx_ready = 1;
        }
        sx_collect = 0;
#endif
        usb.sx_on = 0;
    }
}

static void midi_in_event(uint32_t pkt)
{
    uint32_t cin = pkt & 15u, st = (pkt >> 8) & 0xFFu;
    if (cin >= 4u && cin <= 7u) {
        uint32_t k, nb = cin == 4u || cin == 7u ? 3u : cin == 6u ? 2u : 1u;
        for (k = 0; k < nb; k++)
            sysex_byte((uint8_t)(pkt >> (8u * (k + 1u))));
        return;
    }
    if (st >= 0x80u && st < 0xF8u) {
        usb.sx_on = 0;
#if FELUCCA_OTA
        sx_collect = 0;
#endif
    }
    if (cin == 0xFu && (st == 0xF8u || st == 0xFAu || st == 0xFBu || st == 0xFCu)) {
        midi_enqueue(0xFu | st << 8, 1u);
        return;
    }
    if (cin >= 8u && cin <= 0xEu && (st >> 4) == cin && !((pkt >> 16) & 0x80u) &&
        ((cin == 0xCu || cin == 0xDu) || !(pkt & 0x80000000u))) {
        midi_enqueue(pkt, 1u);
    }
}

#define EP1_ROOM (16u + 8u)
static int ep1_take(const uint8_t *b, uint32_t n)
{
    uint32_t i;
#ifndef FELUCCA_LOADER
    if (MQ - (mi_w - mi_r) < EP1_ROOM)
        return 0;
#endif
    for (i = 0; i + 3u < n; i += 4u)
        midi_in_event((uint32_t)b[i] | (uint32_t)b[i + 1] << 8 | (uint32_t)b[i + 2] << 16 | (uint32_t)b[i + 3] << 24);
    return 1;
}

static void ep1_rx(void)
{
    uint32_t csr, n;
    sie_wr(S_INDEX, 1);
    csr = sie_rd(S_RXCSR1) | (sie_rd(S_RXCSR2) << 8);
    if (!(csr & 1u)) {
        usb.rx_pend = 0;
        return;
    }
    n = sie_rd(S_RXCOUNT1) | (sie_rd(S_RXCOUNT2) << 8);
    if (n > 64u)
        n = 64u;
    fm1_usb_rx_sync();
    if (!ep1_take(ep1rx, n))
        return;
    usb.rx_pend = 0;
    usb.rx_pkts++;
    csr = (csr & ~0x164u) | 0x10u;
    sie_wr(S_RXCSR1, csr & 0xFFu);
    sie_wr(S_RXCSR2, csr >> 8);
}

#if FELUCCA_OTA

static uint32_t ota_now_ms(void);
static void ota_idle(void);

static int ota_wire_send(const uint8_t *p, uint32_t n)
{
    uint32_t i = 0, t0 = ota_now_ms();
    sx_busy = 1;
    while (i < n) {
        uint32_t k = n - i >= 3u ? 3u : n - i, pkt;
        uint32_t cin = k == 3u && i + 3u < n ? 4u : k == 3u ? 7u : 4u + k;
        pkt = cin | (uint32_t)p[i] << 8 | (k > 1u ? (uint32_t)p[i + 1] << 16 : 0u) |
              (k > 2u ? (uint32_t)p[i + 2] << 24 : 0u);
        while (so_w - so_r >= SXQ) {
            if (!*(volatile uint8_t *)&usb.config || ota_now_ms() - t0 > 200u) {
                sx_busy = 0;
                return -1;
            }
            ota_idle();
        }
        sx_out_q[so_w % SXQ] = pkt;
        RING_PUBLISH();
        so_w++;
        i += k;
    }
    sx_busy = 0;
    return 0;
}
static int ota_frame_get(const uint8_t **p, uint32_t *n)
{
    if (!sx_ready)
        return 0;
    RING_PUBLISH();
    *p = sx_frame;
    *n = sx_frame_len;
    return 1;
}
static void ota_frame_done(void)
{
    RING_PUBLISH();
    sx_ready = 0;
}
#endif

static void ep1_tx(void)
{
    uint32_t csr, n = 0;
#if FELUCCA_OTA
    if (mo_w == mo_r && so_w == so_r)
        return;
#else
    if (mo_w == mo_r)
        return;
#endif
    sie_wr(S_INDEX, 1);
    csr = sie_rd(S_TXCSR1);
    if (csr & 0x01u)
        return;
    if (csr & 0x80u)
        sie_wr(S_TXCSR1, csr & ~0x80u);
#if FELUCCA_OTA
    while (so_r != so_w && n < 64u) {
        uint32_t pkt = sx_out_q[so_r % SXQ];
        ep1tx[n] = (uint8_t)pkt;
        ep1tx[n + 1] = (uint8_t)(pkt >> 8);
        ep1tx[n + 2] = (uint8_t)(pkt >> 16);
        ep1tx[n + 3] = (uint8_t)(pkt >> 24);
        n += 4u;
        so_r++;
    }
    while (!sx_busy && so_r == so_w && mo_r != mo_w && n < 64u) {
#else
    while (mo_r != mo_w && n < 64u) {
#endif
        uint32_t pkt;
        RING_PUBLISH();
        pkt = midi_out_q[mo_r % MQ];
        ep1tx[n] = (uint8_t)pkt;
        ep1tx[n + 1] = (uint8_t)(pkt >> 8);
        ep1tx[n + 2] = (uint8_t)(pkt >> 16);
        ep1tx[n + 3] = (uint8_t)(pkt >> 24);
        n += 4u;
        RING_PUBLISH();
        mo_r++;
    }
    if (!n)
        return;
    fm1_usb_ep_send(1, ep1tx, n);
    sie_wr(S_TXCSR1, sie_rd(S_TXCSR1) | 0x01u);
    usb.tx_pkts++;
}

#if FELUCCA_CDC
static void ep3_rx(void)
{
    uint32_t csr, n, i;
    sie_wr(S_INDEX, 3);
    csr = sie_rd(S_RXCSR1) | (sie_rd(S_RXCSR2) << 8);
    if (!(csr & 1u)) {
        cdc.rx_pend = 0;
        return;
    }
    n = sie_rd(S_RXCOUNT1) | (sie_rd(S_RXCOUNT2) << 8);
    if (n > 64u)
        n = 64u;
    if (CI_N - (ci_w - ci_r) < n)
        return;
    cdc.rx_pend = 0;
    fm1_usb_rx_sync();
    for (i = 0; i < n; i++)
        cdc_in[(ci_w + i) % CI_N] = ep3rx[i];
    RING_PUBLISH();
    ci_w += n;
    cdc.rx_pkts++;
    csr = (csr & ~0x164u) | 0x10u;
    sie_wr(S_RXCSR1, csr & 0xFFu);
    sie_wr(S_RXCSR2, csr >> 8);
}

static void ep3_tx(void)
{
    uint32_t csr, n = 0;
    if (co_w == co_r)
        return;
    sie_wr(S_INDEX, 3);
    csr = sie_rd(S_TXCSR1);
    if (csr & 0x01u)
        return;
    if (csr & 0x80u)
        sie_wr(S_TXCSR1, csr & ~0x80u);
    while (co_r != co_w && n < 63u)
        ep3tx[n++] = cdc_out[co_r++ % CO_N];
    fm1_usb_ep_send(3, ep3tx, n);
    sie_wr(S_TXCSR1, sie_rd(S_TXCSR1) | 0x01u);
    cdc.tx_pkts++;
}
#endif

#if FELUCCA_UAC

static __attribute__((noinline)) void uac_render_start(void)
{
    uint32_t fill;
    uac.feed = uac.alt && usb.config && uac.flowing;
    if (!uac.feed) {
        uac.go = 0;
        return;
    }
    if (!uac.go || uac.ring48 != uac.r48) {
        uint32_t r = ua_r, i, prime = UA_PRIME;
        uac.go = 0;
#if FELUCCA_UAC_48K
        uac.ring48 = uac.r48;
        if (uac.ring48) {
            prime = UA_PRIME48;
            for (i = 0; i < 2u * RS_H; i++)
                rs.l[i] = rs.r[i] = 0;
            rs.w = rs.ph = 0;
        }
#endif
        for (i = 0; i < prime; i++)
            ua_ring[(r + i) & (UA_N - 1u)] = 0;
        RING_PUBLISH();
        ua_w = r + prime;
        uac.fill_min = prime;
        RING_PUBLISH();
        uac.go = 1;
        return;
    }
    fill = ua_w - ua_r;
    uac.fill_min = fill;
    if (fill < uac.fill_lo)
        uac.fill_lo = fill;
    if (fill > uac.fill_hi)
        uac.fill_hi = fill;
}

static inline int32_t uac_s16(int32_t v)
{
    return v > 32767 ? 32767 : v < -32768 ? -32768 : v;
}

#if FELUCCA_UAC_48K

static __attribute__((noinline)) void uac_tap48(const int32_t *out, uint32_t n)
{
    uint32_t w = ua_w, i, j, k = 0, h = rs.w, ph = rs.ph;
    if (UA_N - (w - ua_r) < n + n / 8u + 2u) {
        uac.overruns++;
        return;
    }
    for (i = 0; i < n; i++) {
        uint32_t s = h & (RS_H - 1u);
        int16_t l = (int16_t)uac_s16(out[2u * i]), r = (int16_t)uac_s16(out[2u * i + 1u]);
        rs.l[s] = rs.l[s + RS_H] = l;
        rs.r[s] = rs.r[s + RS_H] = r;
        h++;
        s = (h - UAC_FIR_N) & (RS_H - 1u);
        do {
            const int16_t *x = &rs.l[s];
            int64_t al = 0, ar = 0;
            int32_t yl, yr;
            if (ph < UAC_FIR_L / 2u) {
                const int32_t *c = UAC_FIR[ph];
                for (j = UAC_FIR_N; j; j--, x++) {
                    int32_t t = *c++;
                    al += (int64_t)t * x[0];
                    ar += (int64_t)t * x[2u * RS_H];
                }
            } else {
                const int32_t *c = &UAC_FIR[UAC_FIR_L - 1u - ph][UAC_FIR_N - 1u];
                for (j = UAC_FIR_N; j; j--, x++) {
                    int32_t t = *c--;
                    al += (int64_t)t * x[0];
                    ar += (int64_t)t * x[2u * RS_H];
                }
            }
            yl = uac_s16((int32_t)((al + (1 << (UAC_FIR_Q - 1))) >> UAC_FIR_Q));
            yr = uac_s16((int32_t)((ar + (1 << (UAC_FIR_Q - 1))) >> UAC_FIR_Q));
#if FELUCCA_UAC_TONE
            {
                uint32_t t = rs.tone, q = t < 50u ? t : 100u - t, q2 = t < 50u ? 50u - t : t - 50u;
                rs.tone = t == 99u ? 0u : t + 1u;
                yl = (int32_t)q * 1000 - 25000;
                yr = (int32_t)q2 * 1000 - 25000;
            }
#endif
            ua_ring[(w + k++) & (UA_N - 1u)] = (uint32_t)(uint16_t)yl | (uint32_t)(uint16_t)yr << 16;
            ph += UAC_FIR_M;
        } while (ph < UAC_FIR_L);
        ph -= UAC_FIR_L;
    }
    rs.w = h;
    rs.ph = ph;
    RING_PUBLISH();
    ua_w = w + k;
}
#endif

static __attribute__((noinline)) void uac_tap(const int32_t *out, uint32_t n)
{
    uint32_t w = ua_w, i;
    if (!uac.feed)
        return;
#if FELUCCA_UAC_48K
    if (uac.ring48) {
        uac_tap48(out, n);
        return;
    }
#endif
    if (UA_N - (w - ua_r) < n) {
        uac.overruns++;
        return;
    }
#if FELUCCA_UAC_TONE
    {

        static uint32_t ph;
        for (i = 0; i < n; i++, ph = ph == 99u ? 0u : ph + 1u) {
            uint32_t q = ph < 50u ? ph : 100u - ph, q2 = ph < 50u ? 50u - ph : ph - 50u;
            ua_ring[(w + i) & (UA_N - 1u)] = (uint32_t)(uint16_t)(int16_t)((int32_t)q * 1000 - 25000) |
                                             (uint32_t)(uint16_t)(int16_t)((int32_t)q2 * 1000 - 25000) << 16;
        }
        (void)out;
    }
#else
    for (i = 0; i < n; i++)
        ua_ring[(w + i) & (UA_N - 1u)] = (uint32_t)(uint16_t)uac_s16(out[2u * i]) |
                                         (uint32_t)(uint16_t)uac_s16(out[2u * i + 1u]) << 16;
#endif
    RING_PUBLISH();
    ua_w = w + n;
}

static uint32_t uac_packet(uint32_t *d)
{
    uint32_t n = 44u, r, fill, k, i, lo = UA_LO, hi = UA_HI;
#if FELUCCA_UAC_48K
    if (uac.r48) {
        n = 48u;
        lo = UA_LO48;
        hi = UA_HI48;
    } else
#endif
    {
        uac.acc += UAC_RATE % 1000u;
        if (uac.acc >= 1000u) {
            uac.acc -= 1000u;
            n++;
        }
    }
    if (!uac.go || uac.ring48 != uac.r48) {
        for (i = 0; i < n; i++)
            d[i] = 0;
        return n;
    }
    if (uac.fill_min > hi) {
        n++;
        uac.adj_up++;
    } else if (uac.fill_min < lo) {
        n--;
        uac.adj_down++;
    }
    r = ua_r;
    fill = ua_w - r;
    RING_PUBLISH();
    k = fill < n ? fill : n;
    for (i = 0; i < k; i++)
        d[i] = ua_ring[(r + i) & (UA_N - 1u)];
    if (k < n) {
        uint32_t last = k ? d[k - 1u] : uac.last;
        uac.underruns++;
        for (; i < n; i++)
            d[i] = last;
    }
    uac.last = d[n - 1u];
    RING_PUBLISH();
    ua_r = r + k;
    uac.pkts++;
    uac.frames += n;
    return n;
}

static void uac_service(void)
{
    uint32_t csr, n;
    if (!usb.up || !usb.config || !uac.alt)
        return;
    sie_wr(S_INDEX, 4);
    csr = sie_rd(S_TXCSR1);
    if (csr & 0x01u) {
        if (uac.wait < 255u && ++uac.wait == 40u && uac.flowing) {
            uac.flowing = 0;
            uac.stalls++;
        }
        return;
    }
    if (uac.queued) {
        if (csr & 0x04u)
            uac.missed++;
        uac.flowing = 1;
    }
    uac.queued = 1;
    uac.wait = 0;
    n = uac_packet(ep4tx);
    fm1_usb_ep4_send(ep4tx, n * 4u);
    sie_wr(S_TXCSR1, 0x01);
}
#endif

static void usb_poll(void)
{
    uint32_t iu, it, ir;
    if (!usb.up)
        return;
    if (fm1_usb_sof_take()) {

        usb.sof_seen++;
        usb.no_sof = 0;
    } else if (++usb.no_sof > usb.max_gap) {
        usb.max_gap = usb.no_sof;
    }
    if (usb.config) {
        static uint32_t tick;
        if (++tick >= 20u) {
            uint16_t f;
            tick = 0;
            f = (uint16_t)(sie_rd(S_FRAME1) | (sie_rd(S_FRAME2) & 7u) << 8);
            if (f != usb.frame) {
                usb.frame = f;
                usb.frame_same = 0;
                usb.suspended = 0;
            } else if (++usb.frame_same >= 10u && !usb.suspended) {
                usb.suspended = 1;
                usb.frame_stalls++;
            }
        }
    }
    iu = sie_rd(S_INTRUSB);
#ifdef FELUCCA_LOADER
    it = sie_rd(S_INTRTX1) | sie_rd(S_INTRTX2) << 8;
    ir = sie_rd(S_INTRRX1) | sie_rd(S_INTRRX2) << 8;
#else
    it = sie_rd(S_INTRTX1);
    ir = sie_rd(S_INTRRX1);
#endif
    if (iu & 0x01u) {
        usb.suspends++;
        usb.suspended = 1;
    }
    if (iu & 0x02u)
        usb.suspended = 0;
    if (iu & 0x04u) {
        usb.resets++;
        sie_wr(S_FADDR, 0);
        usb.config = 0;
        usb.suspended = 0;
        mo_r = mo_w;
        usb.sx_on = 0;
#if FELUCCA_OTA
        so_r = so_w;
        sx_collect = 0;
#endif
        usb.e0_tx = 0;
        usb.has_pend_addr = 0;
        usb.rx_pend = 0;
#if FELUCCA_CDC
        cdc.dtr = 0;
        cdc.e0_rx = 0;
        cdc.rx_pend = 0;
#endif
#if FELUCCA_UAC
        uac.e0_rx = 0;
        uac_stream(0);
        uac_rate_set(UAC_RATE);
#endif
        fm1_usb_ep0_buf(ep0buf);
        sie_wr(S_INTRUSBE, 0x07);
        sie_wr(S_INTRTX1E, 0x01);
        sie_wr(S_INTRRX1E, 0);
    }
    if (it & 0x01u)
        ep0_service();
    if (ir & 0x02u)
        usb.rx_pend = 1;
    if (usb.rx_pend)
        ep1_rx();
    if (usb.config)
        ep1_tx();
#if FELUCCA_CDC
    if (usb.config && usb_cdc_on) {
        if (ir & 0x08u)
            cdc.rx_pend = 1;
        if (cdc.rx_pend)
            ep3_rx();
        if (cdc.dtr)
            ep3_tx();
    }
#endif
#if FELUCCA_UAC
    uac_service();
#endif
}

static void usb_start(void)
{
    usb.timeouts = 0;
    fm1_usb_reset();
    fm1_delay_ms(25);
    fm1_usb_attach(ep0buf);
    sie_wr(S_POWER, 0x60);
    sie_wr(S_INTRUSBE, 0x07);
    sie_wr(S_INTRTX1E, 0x01);
    sie_wr(S_INTRTX2E, 0);
    sie_wr(S_INTRRX1E, 0);
    sie_wr(S_INTRRX2E, 0);
    fm1_usb_ep_enable(0x1Fu);
    usb.up = usb.timeouts < 3u;
}

static void usb_retry(uint32_t now_ms)
{
    static uint32_t t;
    if (usb.up || usb.detached || now_ms - t < 1000u)
        return;
    t = now_ms;
    usb.retries++;
    usb_start();
}

static void usb_detach(void)
{
    usb.detached = 1;
    usb.up = 0;
    fm1_usb_off();
}

#if FELUCCA_CDC

static __attribute__((unused)) void usb_cdc_switch(uint32_t on)
{
    on = on != 0;
    if (on == usb_cdc_on)
        return;
    if (usb.up) {
        usb.up = 0;
        RING_PUBLISH();
        fm1_usb_off();
    }
    usb.config = 0;
    cdc.dtr = 0;
    cdc.rx_pend = 0;
    usb_cdc_on = (uint8_t)on;
}
#endif

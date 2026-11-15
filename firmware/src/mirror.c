/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Blair (blairysbeatlab), BBL JAM BOX */

#define MIR_CMD 0x70u
#define MIR_N 32768u
#define MIR_BODY 200u
#define MIR_ALIVE_MS 3000u
#define MIR_WAIT_MS 500u
#define MIR_LAG 3072u
static uint8_t mir_buf[MIR_N] __attribute__((section(".pool")));
static uint32_t mir_w, mir_r, mir_t;
static uint8_t mir_on, mir_seq, mir_skip, mir_drew, mir_ser;
static uint8_t mir_msg[8 + MIR_BODY + 1], mir_wire[2 + (8 + MIR_BODY + 1) * 8 / 7 + 2];

static int mir_send(void)
{
    uint32_t n = mir_w - mir_r, i, s, w;
#if FELUCCA_CDC
    if (mir_ser) {
        uint32_t room = CO_N - (co_w - co_r);
        if (!cdc.dtr) {
            mir_on = 0;
            return 0;
        }
        n = n < room ? n : room;
        for (i = 0; i < n; i++)
            cdc_out[(co_w + i) % CO_N] = mir_buf[(mir_r + i) % MIR_N];
        RING_PUBLISH();
        co_w += n;
        mir_r += n;
        return n != 0u;
    }
#endif
    if (!n || SXQ - (so_w - so_r) < (sizeof mir_wire + 2u) / 3u + 1u)
        return 0;
    if (n > MIR_BODY)
        n = MIR_BODY;
    mir_msg[0] = 0;
    mir_msg[1] = 0x59;
    mir_msg[2] = (uint8_t)MIR_CMD;
    mir_msg[3] = (uint8_t)(n + 1u);
    mir_msg[4] = mir_msg[5] = 0;
    mir_msg[6] = mir_seq++;
    s = mir_msg[6];
    for (i = 0; i < n; i++) {
        mir_msg[7 + i] = mir_buf[(mir_r + i) % MIR_N];
        s += mir_msg[7 + i];
    }
    mir_msg[7 + n] = (uint8_t)~s;
    mir_wire[0] = 0xF0;
    w = 1u + ota_pack7(mir_msg, 8u + n, mir_wire + 1);
    mir_wire[w++] = 0xF7;
    if (ota_wire_send(mir_wire, w) < 0) {
        mir_on = 0;
        return 0;
    }
    mir_r += n;
    return 1;
}

static void mir_put(uint32_t b)
{
    uint32_t t0 = fm1_ms;
    while (mir_on && mir_w - mir_r >= MIR_N && !mir_send()) {
        if (fm1_ms - t0 > MIR_WAIT_MS)
            mir_on = 0;
        fm1_wdt_feed();
    }
    if (!mir_on)
        return;
    mir_buf[mir_w++ % MIR_N] = (uint8_t)b;
}
static inline uint32_t mir_c8(uint32_t v)
{
    return (v >> 8 & 0xE0u) | (v >> 6 & 0x1Cu) | (v >> 3 & 0x03u);
}
static void mir_head(uint32_t t, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    mir_drew = 1;
    mir_put(t);
    mir_put(x);
    mir_put(y);
    mir_put(w & 0xFFu);
    mir_put(h & 0xFFu);
}
static void mir_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t c)
{
    if (!mir_on || mir_skip)
        return;
    mir_head(1, x, y, w, h);
    mir_put(mir_c8(c));
}
static void mir_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *px)
{
    uint32_t i, n = w * h, c, run = 0, last = 0x100u;
    if (!mir_on || mir_skip || !n || x >= 240u || y >= 240u)
        return;
    mir_head(2, x, y, w, h);
    for (i = 0; i < n; i++) {
        c = mir_c8((uint32_t)(px[i] >> 8 | px[i] << 8) & 0xFFFFu);
        if (c == last && run < 255u) {
            run++;
            continue;
        }
        if (run) {
            mir_put(run);
            mir_put(last);
        }
        last = c;
        run = 1;
    }
    mir_put(run);
    mir_put(last);
}

static void mir_start(uint32_t ser)
{
    mir_on = 1;
    mir_ser = (uint8_t)ser;
    mir_skip = 0;
    mir_w = mir_r = 0;
    mir_put(3);
    if (ser) {
        mir_put('B');
        mir_put('J');
        mir_put('B');
    }
    ui.force = 1;
}
static void mir_cmd(const uint8_t *body, uint32_t n)
{
    if (n < 1u || !body[0]) {
        mir_on = 0;
        return;
    }
    mir_t = fm1_ms | 1u;
    if (!mir_on || mir_ser)
        mir_start(0);
}
#if FELUCCA_CDC
static void mir_serial(uint32_t c)
{
    if (c == 'm') {
        mir_on = 0;
        return;
    }
    if (c != 'S' && c != 'M')
        return;
    mir_t = fm1_ms | 1u;
    if (c == 'S' || !mir_on || !mir_ser)
        mir_start(1);
}
#endif
static void mir_pump(void)
{
    if (mir_on && (int32_t)(fm1_ms - mir_t) > (int32_t)MIR_ALIVE_MS)
        mir_on = 0;
    while (mir_on && mir_send())
        ;
    if (mir_on && mir_skip && mir_w == mir_r) {
        mir_skip = 0;
        ui.force = 1;
    }
}
static void mir_mark(void)
{
    if (!mir_on || !mir_drew || mir_skip)
        return;
    mir_drew = 0;
    mir_put(4);
    if (mir_w - mir_r > MIR_LAG)
        mir_skip = 1;
}

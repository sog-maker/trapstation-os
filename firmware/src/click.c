/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

enum { CLICK_OFF, CLICK_REC, CLICK_ON };
static volatile uint8_t click_mode;
static volatile uint8_t click_lvl = 1;
static volatile uint8_t cin_bars;
static volatile uint8_t click_req;

#define CLICK_K_DOWN 8195
#define CLICK_K_BEAT 6154
#define CLICK_AMP 16384
static const uint16_t CLICK_GAIN[3] = {1024, 2048, 4096};

static struct { int32_t s, c, k, env, g; } clk;

static __attribute__((noinline)) void click_render(int32_t *out, uint32_t n)
{
    uint32_t i;
    int32_t s, c, k, env, g;
    if (click_req) {
        uint32_t down = click_req == 2u;
        clk.s = 0;
        clk.c = CLICK_AMP;
        clk.k = down ? CLICK_K_DOWN : CLICK_K_BEAT;
        clk.env = 32767;
        clk.g = CLICK_GAIN[click_lvl % 3u] >> (down ? 0 : 1);
        click_req = 0;
    }
    if (clk.env <= 0)
        return;
    s = clk.s; c = clk.c; k = clk.k; env = clk.env;
    g = (int32_t)(((uint32_t)clk.g * song.master_q12) >> 12);
    for (i = 0; i < n; i++) {
        int32_t v, l, r;
        s += (k * c) >> 15;
        c -= (k * s) >> 15;
        v = (((s * env) >> 15) * g) >> 12;
        env -= (env >> 8) + 1;
        if (env < 0)
            env = 0;
        l = out[2u * i] + v;
        r = out[2u * i + 1u] + v;
        out[2u * i] = l > 32767 ? 32767 : l < -32767 ? -32767 : l;
        out[2u * i + 1u] = r > 32767 ? 32767 : r < -32767 ? -32767 : r;
    }
    clk.s = s; clk.c = c; clk.env = env;
}

/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION contributors */

static uint32_t tr_s = 0x9E3779B9u;
static uint8_t tr_mix=100;
static uint32_t tr_raw(uint32_t n)
{
    tr_s ^= tr_s << 13;
    tr_s ^= tr_s >> 17;
    tr_s ^= tr_s << 5;
    return n ? tr_s % n : 0u;
}
static uint32_t tr_rnd(uint32_t n) {
    uint32_t v=tr_raw(n);
    return tr_mix<100u && tr_raw(100u)>=tr_mix ? 0u : v;
}
static int tr_pct(uint32_t p) { return tr_raw(100u) < p*tr_mix/100u; }
static uint32_t tr_pick(const uint8_t *at, const uint8_t *w, uint32_t n)
{
    uint32_t sum = 0, i, r;
    for (i = 0; i < n; i++)
        sum += w[i];
    r = tr_rnd(sum);
    for (i = 0; i + 1u < n && r >= w[i]; i++)
        r -= w[i];
    return at[i];
}
static void tr_seed_with(uint32_t s)
{
    tr_s = s ? s : 0x9E3779B9u;
    tr_rnd(1u);
}
static uint32_t tr_seed(void)
{
    uint32_t s = rng() ^ (fm1_ms * 2654435761u) ^ (uint32_t)fm1_ticks();
    {uint32_t gi=gen_index(cur_page());tr_mix=gi<4u?gen_amount[gi]:100u;}
    tr_seed_with(s);
    return s;
}

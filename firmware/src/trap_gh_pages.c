/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION contributors */

static void gh_pad(int32_t x, int32_t y, int32_t w, int32_t h, int on, const char *s, uint32_t sz)
{
    uint16_t c = on ? GH_RED : GH_P30;
    gh_sbox(x, y, w, h, 2, c, on ? GH_PINK : GH_P62, GH_BG);
    if (s)
        gh_t(x + w / 2, y + h / 2 + (int32_t)gh_f(sz)->asc / 2 - 1, sz, s, GH_WHITE, c, GA_M);
}
static int32_t gh_r(const gb_view_t *v, uint32_t k) { return (v->mask >> k) & 1u ? v->ratio[k] : 0; }
static int16_t gh_ys[240];

static uint8_t gh_mark;
static void gh_dot(int32_t x, int32_t y, uint16_t under)
{
    gh_disc(x, y, 3, GH_RED, under);
    cv_rect(x - 1, y, 2, 1, GH_WHITE);
    cv_rect(x, y - 1, 1, 2, GH_WHITE);
}
static void gh_marker(const gb_view_t *v, int32_t x0, int32_t n)
{
    int32_t i, k;
    if (!gh_mark || !v->play || !v->brd || n < 2)
        return;
    i = (int32_t)(v->brd - 1u) * (n - 1) / 959;
    for (k = 3; k >= 1; k--)
        if (i - 4 * k >= 0)
            gh_disc(x0 + i - 4 * k, gh_ys[i - 4 * k], 1, gh_mixc(GH_DARK, GH_RED, (uint32_t)(256 - 60 * k)), GH_DARK);
    gh_dot(x0 + i, gh_ys[i], GH_DARK);
}
static void gh_mstrip(const gb_view_t *v, int32_t x0, int32_t y, int32_t w)
{
    if (!gh_mark || !v->play || !v->brd)
        return;
    cv_rect(x0, y, w, 1, GH_LBL);
    gh_dot(x0 + (int32_t)(v->brd - 1u) * (w - 1) / 959, y, GH_DARK);
}

static void gh_path_dot(const gb_view_t *v, const int16_t *x, const int16_t *y, uint32_t n)
{
    uint32_t total = 0, len[8], i, at;
    if (!gh_mark || !v->play || !v->brd || n < 2u || n > 9u)
        return;
    for (i = 0; i + 1u < n; i++) {
        int32_t dx = x[i + 1u] - x[i], dy = y[i + 1u] - y[i];
        len[i] = (uint32_t)((dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy));
        total += len[i];
    }
    at = (v->brd - 1u) * total / 960u;
    for (i = 0; i + 1u < n; i++) {
        if (at <= len[i] || i + 2u == n) {
            uint32_t den = len[i] ? len[i] : 1u;
            gh_dot(x[i] + (x[i + 1u] - x[i]) * (int32_t)at / (int32_t)den,
                   y[i] + (y[i + 1u] - y[i]) * (int32_t)at / (int32_t)den, GH_DARK);
            return;
        }
        at -= len[i];
    }
}
static void gh_edge_dot(const gb_view_t *v, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    int16_t x[2] = {x0, x1}, y[2] = {y0, y1};
    gh_path_dot(v, x, y, 2);
}

#include "trap_final_ui.c"

enum { GC_DRUM, GC_VERB, GC_LFO, GC_DJ, GC_COMB };
static void gh_preview(const gb_view_t *v, const char *cap, uint32_t kind, int32_t a, int32_t b, int32_t c)
{
    int32_t i, n = 226, x0 = GX(3), mid = GY(60), amp = 52;
    gh_grid(4, 34, 232, 142, 16, 8);
    for (i = 0; i < n; i++) {
        int32_t y = mid, e;
        switch (kind) {
        case GC_DRUM: {
            int32_t tau = 14 + b * 40 / 1000;
            e = 256 * tau / (tau + i * 2);
            y = mid - tl_sin((uint32_t)(i * (30 + a * 30 / 1000) + 768)) * amp / 256 * e / 256;
            break;
        }
        case GC_VERB: {
            int32_t tau = 14 + a * 60 / 1000, s = tl_sin((uint32_t)(i * (300 - b * 120 / 1000)));
            e = 256 * tau / (tau + i * 3);
            y = GY(102) - ((256 + s) * e / 512) * (GY(102) - GY(14)) / 256;
            break;
        }
        case GC_LFO: {
            uint32_t ph = (uint32_t)(i * (6 + a * 26 / 1000));
            int32_t s = tl_sin(ph);
            if (b == 1)
                s = (int32_t)(ph & 1023u) < 512 ? -256 + (int32_t)(ph & 511u) : 256 - (int32_t)(ph & 511u);
            else if (b == 2)
                s = 256 - (int32_t)(ph & 1023u) / 2;
            else if (b == 3)
                s = (ph & 512u) ? -200 : 200;
            else if (b == 4)
                s = tl_sin((ph & ~127u) * 37u) * 4 / 5;
            y = mid - s * amp / 256;
            break;
        }
        case GC_DJ: {
            int32_t f = a < -1 ? 226 + a * 3 : a > 1 ? a * 3 : 260, u = (i - f) * 4, d;
            d = u <= -64 ? 0 : u >= 64 ? 256 : (u + 64) * (u + 64) / 32 - (u > 0 ? u * u / 16 : 0);
            d = clamp(d, 0, 256);
            if (a > 1)
                d = 256 - d;
            y = GY(30) + d * (GY(100) - GY(30)) / 256;
            if (v->play && v->brd)
                y += tl_sin((uint32_t)(i * 22 + v->brd * 4)) * 7 / 256;
            break;
        }
        case GC_COMB: {
            int32_t p = 16 + (1000 - a) * 8 / 1000, ph = (i % p) * 1024 / p, s = tl_sin((uint32_t)ph);
            s = s > 0 ? s : 0;
            e = 256 * (8 + b * 24 / 1000) / (8 + b * 24 / 1000 + i / p * 2);
            y = GY(100) - s * e / 256 * (GY(100) - GY(14)) / 256;
            break;
        }
        }
        gh_ys[i] = (int16_t)clamp(y, 36, 174);
    }
    (void)c;
    gh_curve(x0, n, gh_ys, GH_RED, 2);
    if (kind == GC_DJ && v->play && v->brd) {
        int32_t at = (int32_t)(v->brd - 1u) * (n - 1) / 959;
        gh_dot(x0 + at, gh_ys[at], GH_DARK);
    } else gh_marker(v, x0, n);
    gh_t(GX(7), GY(14) + 1, 10, cap, GH_BG, GH_DARK, GA_S);
}

static void gh_amp(const gb_view_t *v)
{
    int32_t k, lit = gh_r(v, 0) * 10 / 1000;
    gh_module("MASTER / AMP");
    cv_rrect(12, 62, 56, 108, 3, GH_DARK, GH_CARD);
    gh_t(40, 76, 9, "LEVEL", GH_BG, GH_DARK, GA_M);
    for (k = 0; k < 10; k++)
        cv_rrect(21, 157 - 7 * k, 37, 5, 1, k >= lit ? GH_LBL : k >= 8 ? GH_HEX(0xdc7179) : GH_HEX(0xca4551), GH_DARK);
    gh_disc(116, 103, 24, GH_LBL, GH_CARD);
    gh_disc(116, 103, 19, GH_DARK, GH_LBL);
    {
        int32_t px, py;
        gh_pt(116, 103, 17, -135 + gh_r(v, 0) * 270 / 1000, &px, &py);
        gh_aaline(116, 103, px, py, 2, GH_WHITE, GH_DARK);
    }
    gh_t(116, 144, 9, "AMP", GH_INK, GH_CARD, GA_M);
    {
        char a[12];
        str_cpy(a, v->val[0], sizeof a);
        str_cpy(a + str_len(a), "%", sizeof a - str_len(a));
        gh_t(116, 153, 13, a, GH_INK, GH_CARD, GA_M);
    }
    gh_disc(195, 103, 24, GH_LBL, GH_CARD);
    gh_disc(195, 103, 19, GH_DARK, GH_LBL);
    {
        int32_t px, py;
        gh_pt(195, 103, 17, -135 + gh_r(v, 1) * 270 / 1000, &px, &py);
        gh_aaline(195, 103, px, py, 2, GH_WHITE, GH_DARK);
    }
    gh_t(195, 144, 9, "PAN", GH_INK, GH_CARD, GA_M);
    gh_t(195, 153, 13, v->val[1], GH_INK, GH_CARD, GA_M);
}

static void gh_sends(const gb_view_t *v)
{
    int r = ui.hot_t && ui.hot_col % 4u == 1u;
    char b[24];
    cv_rect(GX(15), GY(18), GX(205) - GX(15), GY(58) - GY(18), r ? GH_P30 : GH_RED);
    gh_t(GX(22), GY(31), 10, "01", r ? GH_GREY : GH_WHITE, r ? GH_P30 : GH_RED, GA_S);
    str_cpy(b, "TRACK > DELAY", sizeof b);
    gh_t(GX(44), GY(31), 10, b, GH_WHITE, r ? GH_P30 : GH_RED, GA_S);
    cv_rect(GX(15), GY(59), GX(205) - GX(15), GY(99) - GY(59), r ? GH_RED : GH_P30);
    gh_t(GX(22), GY(72), 10, "02", r ? GH_WHITE : GH_GREY, r ? GH_RED : GH_P30, GA_S);
    str_cpy(b, "TRACK > REVERB", sizeof b);
    gh_t(GX(44), GY(72), 10, b, GH_WHITE, r ? GH_RED : GH_P30, GA_S);
}

static void gh_delay(const gb_view_t *v)
{
    int32_t cx, cy, fb = gh_r(v, 1), col = v->echo_ph * 16 / 12;
    for (cy = 0; cy < 6; cy++)
        for (cx = 0; cx < 16; cx++) {
            int32_t x = GXT(cx * 138), y = GYT(cy * 188);
            int32_t age = (col - cx + 16) % 16, lit = v->play && age < 1 + fb * 5 / 1000 && cy >= age;
            uint16_t c = !lit ? GH_LBL : age == 0 ? GH_RED : gh_mixc(GH_RED, GH_LBL, (uint32_t)(age * 48));
            cv_rrect(x, y, GXT(cx * 138 + 110) - x, GYT(cy * 188 + 150) - y, 1, c, GH_BG);
        }
}

static void gh_perf(const gb_view_t *v)
{
    static const char *const FD[4] = {"1/16", "1/8", "1/4", "BAR"}, *const RP[4] = {"REV", "STUT", "STOP", "TAPE"};
    static const uint8_t RB[4] = {3, 5, 4, 0xFF};
    uint32_t k;
    char b[4] = {'M', 0, 0, 0};
    for (k = 0; k < 16u; k++) {
        int32_t c = (int32_t)(k & 3u), r = (int32_t)(k >> 2), x = GXT(c * 555), y = GYT(r * 285);
        int32_t w = GXT(c * 555 + 510) - x, h = GYT(r * 285 + 250) - y, on;
        const char *s = b;
        if (k < 8u) {
            b[1] = (char)('1' + k);
            on = v->mac_held[k] || v->mac_n[k];
        } else if (k < 12u) {
            s = FD[k - 8u];
            on = v->fade == k - 7u;
        } else {
            s = RP[k - 12u];
            on = RB[k - 12u] != 0xFFu && ((v->rpt >> RB[k - 12u]) & 1u);
        }
        gh_pad(x, y, w, h, on, s, 10);
    }
}

static void gh_banks(const gb_view_t *v)
{
    uint32_t i, b = (uint32_t)v->cur[0] % 5u, s;
    int16_t *vp;
    const param_desc_t *d = page_desc(cur_page(), 1, &vp);
    s = d ? (uint32_t)(v->cur[1] - d->min) % 8u : 0u;
    for (i = 0; i < 5u; i++) {
        char n[2] = {(char)('A' + i), 0};
        uint16_t c = i == b ? GH_RED : GH_DARK;
        cv_rrect(4 + 47 * (int32_t)i, 34, 44, 23, 3, c, GH_BG);
        gh_t(26 + 47 * (int32_t)i, 50, 11, n, GH_WHITE, c, GA_M);
    }
    for (i = 0; i < 8u; i++) {
        int32_t x = 4 + 59 * (int32_t)(i & 3u), y = 63 + 58 * (int32_t)(i >> 2);
        uint16_t c = i == s ? GH_RED : GH_DARK;
        char n[4] = {'0', (char)('1' + i), 0, 0};
        cv_rrect(x, y, 55, 53, 4, c, GH_BG);
        gh_t(x + 6, y + 14, 9, n, GH_BG, c, GA_S);
        if (d && d->names)
            gh_t(x + 27, y + 36, 10, d->names[i], GH_WHITE, c, GA_M);
    }
}

static void gh_steps(const gb_view_t *v)
{
    uint32_t i;
    for (i = 0; i < 16u; i++) {
        int32_t c = (int32_t)(i & 7u), r = (int32_t)(i >> 3), x = GXT(c * 277), y = GYT(110 + r * 490);
        int32_t w = GXT(c * 277 + 240) - x, h = GYT(110 + r * 490 + 400) - y;
        char n[4];
        fmt_int(n, (int32_t)v->sq_at + (int32_t)i + 1);
        if (i >= v->sq_len && v->sq_len)
            gh_sbox(x, y, w, h, 2, GH_BG, GH_GREY, GH_BG);
        else
            gh_pad(x, y, w, h, (v->sq_hit >> i) & 1u, n, 10);
        if (v->sq_pos == i + 1u)
            cv_rect(x, y + h + 2, w, 2, GH_WHITE);
    }
}

static void gh_clock(const gb_view_t *v)
{
    char b[12];
    uint32_t k;
    fmt_int(b, v->bpm);
    str_cpy(b + str_len(b), ".0", sizeof b - str_len(b));
    gh_t(GX(110), GY(69), 43, b, GH_WHITE, GH_BG, GA_M);
    gh_t(GX(211), GY(84), 9, "BPM", GH_GREY, GH_BG, GA_E);
    for (k = 0; k < 4u; k++) {
        int32_t x = GX(55 * (int32_t)k), w = GX(55 * (int32_t)k + 51) - x;
        char n[2] = {(char)('1' + k), 0};
        gh_pad(x, GY(93), w, GY(113) - GY(93), v->play ? v->clk_beat == k : k == 0u, n, 10);
    }
}

static void gh_scenes(const gb_view_t *v)
{
    static const char *const RN[5] = {"INTRO", "VERSE", "HOOK", "OUTRO", "LIVE"};
    uint32_t r, k;
    for (k = 0; k <= NTRK; k++) {
        char n[3] = {'T', (char)('1' + k), 0};
        gh_t(66 + 25 * (int32_t)k, 45, 9, k < NTRK ? n : "FX", GH_INK, GH_BG, GA_M);
    }
    for (r = 0; r < 5u; r++) {
        int32_t y = 51 + 25 * (int32_t)r, pick = r < 4u && (int32_t)r == (int32_t)ph_scene;
        uint32_t m = r < 4u ? v->sc_m[r] : ((~(uint32_t)v->tmute) & 63u) | ((v->sc_live & 2u) ? 0u : 64u);
        cv_rrect(4, y, 47, 22, 2, pick ? GH_RED : GH_CARD, GH_BG);
        gh_t(27, y + 15, 9, RN[r], pick ? GH_WHITE : GH_INK, pick ? GH_RED : GH_CARD, GA_M);
        for (k = 0; k <= NTRK; k++) {
            int on = (m >> k) & 1u;
            uint16_t c = !on ? GH_CARD : k < NTRK ? GH_TRK(k) : GH_P62;
            gh_sbox(55 + 25 * (int32_t)k, y, 22, 22, 2, c, pick ? GH_INK : GH_GREY, GH_BG);
        }
    }
}

static void gh_sequencer(const gb_view_t *v)
{
    uint32_t r, i;
    gh_grid(4, 34, 232, 144, 16, 8);
    for (r = 0; r < NTRK; r++) {
        int32_t y = 37 + 21 * (int32_t)r;
        char n[3] = {'T', (char)('1' + r), 0};
        gh_t(7, y + 12, 9, n, GH_WHITE, GH_DARK, GA_S);
        for (i = 0; i < 16u; i++) {
            int32_t x = 30 + (int32_t)(i * 127u / 10u), hit = (v->ov_hit[r] >> i) & 1u, in = i < v->ov_len[r];
            uint16_t c = hit ? GH_TRK(r) : GH_LBL;
            if (!in)
                c = GH_HEX(0x2c2c2c);
            if (!(i & 3u))
                gh_sbox(x, y, 10, 17, 1, c, GH_P62, GH_DARK);
            else
                cv_rrect(x, y, 10, 17, 1, c, GH_DARK);
        }
    }
    if (v->ov_pos[v->sel % NTRK]) {
        int32_t x = 30 + (int32_t)((v->ov_pos[v->sel % NTRK] - 1u) * 127u / 10u) + 5;
        cv_rect(x, 34, 1, 133, GH_WHITE);
    }
}

static void gh_roll(const gb_view_t *v)
{
    uint32_t i, q, lo = 127, hi = 0, notes = 0;
    int32_t x0 = GX(17), y0 = GY(0), w = GX(220) - x0, h = GY(90) - y0, rows;
    for (i = 0; i < 16u; i++)
        for (q = 0; q < v->pr_n[i]; q++) {
            notes++;
            lo = v->pr_note[i][q] < lo ? v->pr_note[i][q] : lo;
            hi = v->pr_note[i][q] > hi ? v->pr_note[i][q] : hi;
        }
    if (lo > hi && v->pr_ln)
        lo = hi = v->pr_lv[0];
    if (lo > hi)
        lo = 60, hi = 67;
    for (i = 0; i < v->pr_ln; i++) {
        lo = v->pr_lv[i] < lo ? v->pr_lv[i] : lo;
        hi = v->pr_lv[i] > hi ? v->pr_lv[i] : hi;
    }
    if (hi - lo < 7u)
        hi = lo + 7u;
    rows = (int32_t)(hi - lo + 1u);
    gh_grid(x0, y0, w, h, 16, 8);
    for (i = 0; i < 8u; i++) {
        uint32_t n = hi - (uint32_t)(((int32_t)i * rows) / 8), pc = n % 12u, nb = hi - (uint32_t)(((int32_t)(i + 1u) * rows) / 8);
        int bk = pc == 1u || pc == 3u || pc == 6u || pc == 8u || pc == 10u, lit = 0;
        for (q = 0; q < v->pr_ln; q++)
            lit |= v->pr_lv[q] <= n && (int32_t)v->pr_lv[q] > (int32_t)nb;
        cv_rect(4, GYT((int32_t)i * 112), GX(14) - 4, GYT((int32_t)i * 112 + 100) - GYT((int32_t)i * 112),
                lit ? GH_RED : bk ? GH_GREY : GH_BG);
    }
    for (i = 0; i < 16u; i++) {
        int32_t x = x0 + 3 + (int32_t)i * (w - 6) / 16, nw = (w - 6) / 16 - 2, sel = v->pr_sel == i + 1u;
        if (i >= v->pr_len)
            continue;
        for (q = 0; q < v->pr_n[i]; q++) {
            int32_t y = y0 + h - 4 - ((int32_t)(v->pr_note[i][q] - lo) * (h - 10)) / rows - 6;
            uint32_t end = i + 1u;
            while (end < v->pr_len && ((v->pr_tie >> end) & 1u)) end++;
            int32_t width = nw + (int32_t)(end - i - 1u) * (w - 6) / 16;
            cv_rect(x, y, width, 6, sel ? GH_WHITE : GH_RED);
        }
        {
            int32_t vh = v->pr_n[i] ? 14 + (int32_t)(i % 4u == 0u) * 8 : 6, yb = GY(112);
            cv_rect(x + 2, yb - vh, nw - 2 > 3 ? 6 : nw, vh, v->pr_n[i] ? (i % 4u == 0u ? GH_RED : GH_GREY) : GH_LBL);
        }
    }
    if (!notes)
        gh_t(x0 + w / 2, y0 + h / 2, 9, "NO NOTES / THIS TRACK", GH_GREY, GH_DARK, GA_M);
    if (v->pr_pos) {
        int32_t x = x0 + 3 + (int32_t)(v->pr_pos - 1u) * (w - 6) / 16 + 3;
        cv_rect(x, y0, 1, h, GH_WHITE);
    }
}

static void gh_mix(const gb_view_t *v)
{
    const page_t *pg = cur_page();
    uint32_t s;
    for (s = 0; s < 2u; s++) {
        uint32_t k = (uint32_t)pg->id[s * 2u] % NTRK;
        const track_t *t = &trk[k];
        int32_t ox = (int32_t)s * 112, x = GX(ox), w = GX(ox + 106) - x, lvl = t->p[P_LEVEL] & 127, pan = t->p[P_PAN], i;
        char n[10];
        int32_t fy, px, py;
        cv_rect(x, GY(0), w, GY(114) - GY(0), GH_P30);
        str_cpy(n, "TRACK 1", sizeof n);
        n[6] = (char)('1' + k);
        gh_t(GX(ox + 7), GY(14), 10, n, k == v->sel % NTRK ? GH_RED : GH_WHITE, GH_P30, GA_S);
        for (i = 0; i < 10; i++)
            cv_rect(GX(ox + 8), GY(92 - 7 * i), GX(ox + 20) - GX(ox + 8), 6, i < lvl * 10 / 128 ? (i >= 6 ? GH_RED : GH_GREY) : GH_LBL);
        cv_rect(GX(ox + 46) - 2, GY(23), 4, GY(92) - GY(23), GH_P62);
        fy = GY(92) - lvl * (GY(92) - GY(23)) / 127 - 6;
        gh_sbox(GX(ox + 35), fy, GX(ox + 58) - GX(ox + 35), 11, 1, GH_BG, GH_WHITE, GH_P30);
        gh_disc(GX(ox + 81), GY(43), 15, GH_P62, GH_P30);
        gh_disc(GX(ox + 81), GY(43), 14, GH_DARK, GH_P62);
        gh_pt(GX(ox + 81), GY(43), 13, pan * 135 / 64, &px, &py);
        gh_aaline(GX(ox + 81), GY(43), px, py, 1, GH_WHITE, GH_DARK);
        gh_sbox(GX(ox + 63), GY(76), GX(ox + 100) - GX(ox + 63), GY(100) - GY(76), 2, t->p[P_MUTE] ? GH_RED : GH_P30,
                t->p[P_MUTE] ? GH_PINK : GH_P62, GH_P30);
        gh_t(GX(ox + 81) + 1, GY(91), 10, "MUTE", GH_WHITE, t->p[P_MUTE] ? GH_RED : GH_P30, GA_M);
    }
}

static int gh_run(const gb_view_t *v, uint32_t kind) { return v->an_k == kind && v->an_f >= 1u; }
static void gh_motion_frame(const gb_view_t *v, const char *head)
{
    int ready = v->an_f >= 17u;
    cv_rrect(4, 34, 232, 21, 3, GH_DARK, GH_CARD);
    cv_rect(4, 34, 3, 21, GH_RED);
    gh_tc(13, 48, 10, ready ? "READY / RESULT AVAILABLE" : head, GH_WHITE, GH_DARK, 200);
    cv_rrect(220, 41, 7, 7, 2, ready || (v->an_f & 1u) ? GH_RED : GH_WHITE, GH_DARK);
    cv_rrect(4, 183, 232, 32, 3, ready ? GH_RED : GH_LBL, GH_BG);
    gh_t(120, 203, 11, ready ? "DONE / PREVIEW RESULT" : "WORKING / ACTIVITY", GH_WHITE, ready ? GH_RED : GH_LBL, GA_M);
}
static void gh_make_btns(void)
{
    cv_rrect(4, 183, 72, 32, 3, GH_DARK, GH_BG);
    gh_t(40, 203, 11, "UNDO", GH_WHITE, GH_DARK, GA_M);
    cv_rrect(80, 183, 156, 32, 3, GH_RED, GH_BG);
    gh_t(158, 203, 12, "K4  MAKE", GH_WHITE, GH_RED, GA_M);
}
static uint32_t gh_hits(uint32_t k)
{
    uint32_t i, m = 0;
    for (i = 0; i < 16u; i++)
        if (trk[k % NTRK].step[i].time == ST_NOTE && trk[k % NTRK].step[i].n)
            m |= 1u << i;
    return m;
}
static void gh_gen_drums(const gb_view_t *v)
{
    static const char *const RN[6] = {"BD", "SD", "CH", "OH", "PC", "BS"};
    static const uint8_t RC[6] = {2, 3, 4, 5, 5, 5};
    uint32_t r, i, run = gh_run(v, TL_AN_TRAP) && v->an_f <= 16u, col = (uint32_t)v->an_f - 1u;
    gh_module("DRUM GENERATOR");
    for (r = 0; r < 6u; r++) {
        uint32_t m = gh_hits(r);
        gh_t(12, 74 + 17 * (int32_t)r, 9, RN[r], GH_INK, GH_CARD, GA_S);
        for (i = 0; i < 16u; i++) {
            int32_t x = 47 + (int32_t)(i * 113u / 10u), y = 63 + 17 * (int32_t)r, hit = (m >> i) & 1u;
            uint16_t c = hit ? GH_TRK(RC[r]) : GH_BG;
            if (run && i == col)
                c = hit ? GH_RED : GH_PINK;
            cv_rrect(x, y, 9, 13, 1, c, GH_CARD);
            if (run && i == col)
                cv_rect(x + 3, y + 5, 3, 3, GH_WHITE);
        }
    }
    if (gh_run(v, TL_AN_TRAP))
        gh_motion_frame(v, "DRUM STEP SCAN");
}
static void gh_gen_hats(const gb_view_t *v)
{
    static const char *const RN[3] = {"CLOSED", "OPEN", "ROLL"};
    uint32_t r, i, k = v->hg_trk < NTRK ? v->hg_trk : 3u;
    uint32_t run = gh_run(v, TL_AN_HAT) && v->an_f <= 16u, f = v->an_f;
    gh_module("HIHAT GENERATOR");
    for (r = 0; r < 3u; r++) {
        gh_t(12, 74 + 32 * (int32_t)r, 8, RN[r], GH_INK, GH_CARD, GA_S);
        for (i = 0; i < 16u; i++) {
            const step_t *s = &trk[k].step[i];
            int on = s->time == ST_NOTE && s->n &&
                     (r == 0u ? !(s->flags & SF_ACCENT) : r == 1u ? (s->flags & SF_ACCENT) != 0 : step_ratchet(s) > 1u);
            int pulse = run && (i * 7u + r * 5u + f * 3u) % 11u == 0u;
            cv_rrect(47 + (int32_t)(i * 113u / 10u), 63 + 32 * (int32_t)r, 9, 28, 1, pulse ? GH_WHITE : on ? GH_TRK(2 + r) : GH_BG, GH_CARD);
        }
    }
    if (gh_run(v, TL_AN_HAT))
        gh_motion_frame(v, "HIHAT PULSES");
}
static void gh_notes_panel(int32_t y, int32_t h, const char *cap, uint32_t k)
{
    uint32_t i, lo = 127, hi = 0;
    const track_t *t = &trk[k % NTRK];
    cv_rrect(12, y, 216, h, 3, GH_DARK, GH_CARD);
    for (i = 1; i < 8u; i++)
        cv_rect(12 + (int32_t)i * 27, y + 4, 1, h - 8, GH_G2);
    for (i = 0; i < 16u; i++)
        if (t->step[i].time == ST_NOTE && t->step[i].n) {
            lo = t->step[i].note[0] < lo ? t->step[i].note[0] : lo;
            hi = t->step[i].note[0] > hi ? t->step[i].note[0] : hi;
        }
    if (hi < lo + 6u)
        hi = lo + 6u;
    for (i = 0; i < 16u; i++)
        if (t->step[i].time == ST_NOTE && t->step[i].n) {
            int32_t yy = y + h - 12 - (int32_t)(t->step[i].note[0] - lo) * (h - 34) / (int32_t)(hi - lo);
            cv_rrect(18 + (int32_t)i * 13, yy, 11, 7, 2, GH_RED, GH_DARK);
        }
    if (cap)
        gh_t(18, y + 14, 9, cap, GH_BG, GH_DARK, GA_S);
}
static void gh_gen_bass(const gb_view_t *v)
{
    gh_module("BASSLINE GENERATOR");
    gh_notes_panel(63, 106, "BASS / PATTERN PREVIEW", song.sel);
    if (gh_run(v, TL_AN_BASS)) {
        const track_t *t = TSEL;
        uint32_t i, lo = 127, hi = 0, upto = v->an_f >= 17u ? 0u : v->an_f;
        int32_t px = -1, py = 0;
        for (i = 0; i < 16u; i++)
            if (t->step[i].time == ST_NOTE && t->step[i].n) {
                lo = t->step[i].note[0] < lo ? t->step[i].note[0] : lo;
                hi = t->step[i].note[0] > hi ? t->step[i].note[0] : hi;
            }
        if (hi < lo + 6u)
            hi = lo + 6u;
        for (i = 0; i < upto && i < 16u; i++)
            if (t->step[i].time == ST_NOTE && t->step[i].n) {
                int32_t x = 23 + (int32_t)i * 13, y = 63 + 106 - 12 - (int32_t)(t->step[i].note[0] - lo) * (106 - 34) / (int32_t)(hi - lo) + 3;
                if (px >= 0)
                    gh_aaline(px, py, x, y, 2, GH_RED, GH_DARK);
                px = x;
                py = y;
            }
        if (px >= 0) {
            gh_disc(px, py, 3, GH_RED, GH_DARK);
            cv_rect(px, py, 1, 1, GH_WHITE);
        }
        gh_motion_frame(v, "BASS CONTOUR");
    }
}
static void gh_gen_chords(const gb_view_t *v)
{
    uint32_t run = gh_run(v, TL_AN_JAZZ) && v->an_f <= 16u;
    uint32_t b, i;
    const track_t *t = TSEL;
    gh_module("CHORD GENERATOR");
    for (b = 0; b < 4u; b++) {
        const step_t *s = 0;
        for (i = 0; i < 4u && !s; i++)
            if (t->step[b * 4u + i].time == ST_NOTE && t->step[b * 4u + i].n)
                s = &t->step[b * 4u + i];
        cv_rrect(12 + 55 * (int32_t)b, 63, 51, 29, 3, b == 0u ? GH_RED : GH_DARK, GH_CARD);
        gh_t(37 + 55 * (int32_t)b, 82, 11, s ? TL_NOTE[s->note[0] % 12u] : "\x97", GH_WHITE, b == 0u ? GH_RED : GH_DARK, GA_M);
    }
    cv_rrect(12, 99, 216, 70, 3, GH_DARK, GH_CARD);
    for (i = 0; i < 16u; i++) {
        const step_t *s = &t->step[i];
        uint32_t q;
        for (q = 0; s->time == ST_NOTE && q < s->n && q < 4u; q++)
            cv_rect(18 + (int32_t)i * 13, 158 - (int32_t)q * 13, 11, 3,
                    run && i == (uint32_t)v->an_f - 1u ? GH_WHITE : q == 0u ? GH_RED : GH_BG);
    }
    if (gh_run(v, TL_AN_JAZZ))
        gh_motion_frame(v, "CHORD VOICING");
}

static void gh_keys(const gb_view_t *v, int arp)
{
    uint32_t mask = scale_mask(TSEL), root = (uint32_t)TSEL->p[P_ROOT] % 12u, k;
    static const int8_t AY[12] = {70, 58, 46, 34, 70, 58, 78, 66, 102, 90, 78, 66};
    (void)v;

    for (k = 0; k < 12u; k++) {
        int32_t c = (int32_t)(k % 6u), r = (int32_t)(k / 6u), x = GXT(c * 368);
        int32_t y = arp ? GY(AY[k]) : GY(28 + 32 * r), w = GXT(c * 368 + 330) - x, h = GY(55) - GY(28);
        gh_pad(x, y, w, h, (mask >> ((k + 12u - root) % 12u)) & 1u, TL_NOTE[k], 10);
    }
}

static void gh_chordgen(void)
{
    uint32_t i, q;
    const track_t *t = TSEL;
    gh_grid(4, 34, 232, 142, 16, 8);
    for (i = 0; i < 8u; i++) {
        const step_t *s = &t->step[i * 2u];
        for (q = 0; s->time == ST_NOTE && q < s->n && q < 3u; q++) {
            int32_t y = GY(91 - (int32_t)((s->note[q] % 24u) * 3u)) ;
            cv_rect(GX(5 + 27 * (int32_t)i), y, GX(26 + 27 * (int32_t)i) - GX(5 + 27 * (int32_t)i), 6, q == 0u ? GH_RED : GH_BG);
        }
    }
    gh_t(GX(6), GY(12), 10, "PROGRESSION / PREVIEW", GH_BG, GH_DARK, GA_S);
}

static void gh_chordvar(const gb_view_t *v)
{
    uint32_t b, r;
    char h[20];
    const track_t *t = TSEL;
    (void)h;
    for (r = 0; r < 2u; r++) {
        gh_t(GX(0), GY(32 + 46 * (int32_t)r), 10, r ? "VARIANT" : "SOURCE", GH_GREY, GH_BG, GA_S);
        for (b = 0; b < 4u; b++) {
            const step_t *s = &t->step[(b * 4u) % NSTEP];
            int32_t x = GX(57 + 40 * (int32_t)b);
            gh_pad(x, GY(19 + 46 * (int32_t)r), GX(93 + 40 * (int32_t)b) - x, GY(46) - GY(19), r == 1u && b == (uint32_t)(v->cur[0] - 1) % 4u,
                   s->time == ST_NOTE && s->n ? TL_NOTE[s->note[0] % 12u] : "\x97", 10);
        }
    }
    cv_rect(GX(58), GY(53), GX(214) - GX(58), 1, GH_P62);
}

static void gh_euclid(const gb_view_t *v)
{
    int32_t len = clamp(v->cur[0], 1, 64), hits = clamp(v->cur[1], 0, len), rot = v->cur[2], i;
    char b[20];
    gh_module("EUCLID / RHYTHM TOOL");
    cv_rrect(12, 62, 216, 107, 3, GH_DARK, GH_CARD);
    for (i = 0; i < len && i < 32; i++) {
        int32_t px, py, hit = ((((i - rot) % len + len) % len) * hits) % len < hits;
        gh_pt(62, 110, 35, i * 360 / (len < 32 ? len : 32), &px, &py);
        if (hit)
            gh_sbox(px - 4, py - 4, 8, 8, 2, GH_RED, GH_WHITE, GH_DARK);
        else
            cv_rrect(px - 4, py - 4, 8, 8, 2, GH_P62, GH_DARK);
    }
    fmt_int(b, hits);
    str_cpy(b + str_len(b), "/", sizeof b - str_len(b));
    fmt_int(b + str_len(b), len);
    gh_t(62, 115, 16, b, GH_WHITE, GH_DARK, GA_M);
    gh_t(122, 87, 9, "EVENLY SPACED", GH_BG, GH_DARK, GA_S);
    fmt_int(b, hits);
    str_cpy(b + str_len(b), " HITS", sizeof b - str_len(b));
    gh_t(122, 112, 18, b, GH_WHITE, GH_DARK, GA_S);
    fmt_int(b, len);
    str_cpy(b + str_len(b), " STEPS / ROT ", sizeof b - str_len(b));
    fmt_int(b + str_len(b), rot);
    gh_t(122, 131, 9, b, GH_BG, GH_DARK, GA_S);
    for (i = 0; i < 16; i++) {
        int32_t hit = i < len && ((((i - rot) % len + len) % len) * hits) % len < hits;
        cv_rrect(17 + i * 133 / 10, 153, 10, 9, 1, hit ? GH_RED : GH_P62, GH_DARK);
    }
}

static void gh_knobs4(const gb_view_t *v)
{
    uint32_t k;
    for (k = 0; k < 4u; k++) {
        if (!((v->mask >> k) & 1u))
            continue;
        gh_knob(36 + 56 * (int32_t)k, 146, 17, v->ratio[k], 1, GH_CARD);
        gh_t(36 + 56 * (int32_t)k, 175, 9, v->lab[k], GH_INK, GH_CARD, GA_M);
    }
}
static void gh_disp(int32_t y, int32_t h)
{
    int32_t k;
    cv_rrect(12, y, 216, h, 3, GH_DARK, GH_CARD);
    for (k = 1; k < 8; k++)
        cv_rect(12 + k * 27, y + 4, 1, h - 8, GH_G2);
    for (k = 1; k < 4; k++)
        cv_rect(16, y + k * h / 4, 208, 1, GH_G2);
}
static void gh_env(const gb_view_t *v, const char *head)
{
    int32_t x0 = 19, x1 = 220, yb = 111, yt = 71, a = x0 + 10 + gh_r(v, 0) * 50 / 1000, d = a + 10 + gh_r(v, 1) * 60 / 1000;
    int32_t s = yb - gh_r(v, 2) * (yb - yt) / 1000, r = x1 - 6 - gh_r(v, 3) * 40 / 1000;
    gh_module(head);
    gh_disp(62, 56);
    cv_line_t(x0, yb, a, yt, GH_RED, 2);
    cv_line_t(a, yt, d, s, GH_RED, 2);
    cv_line_t(d, s, r, s, GH_RED, 2);
    cv_line_t(r, s, x1, yb, GH_RED, 2);
    cv_rect(x0 - 2, yb - 2, 4, 4, GH_WHITE);
    cv_rect(a - 2, yt - 2, 4, 4, GH_WHITE);
    cv_rect(d - 2, s - 2, 4, 4, GH_WHITE);
    cv_rect(r - 2, s - 2, 4, 4, GH_WHITE);
    cv_rect(x1 - 2, yb - 2, 4, 4, GH_WHITE);
    {int16_t px[5]={(int16_t)x0,(int16_t)a,(int16_t)d,(int16_t)r,(int16_t)x1},py[5]={(int16_t)yb,(int16_t)yt,(int16_t)s,(int16_t)s,(int16_t)yb};gh_path_dot(v,px,py,5);}
    gh_knobs4(v);
}
static void gh_two(const gb_view_t *v, const char *head, const char *line)
{
    uint32_t k;
    gh_module(head);
    cv_rrect(12, 62, 216, 26, 3, GH_DARK, GH_CARD);
    gh_tc(120 - text_w(gh_f(10), line) / 2, 80, 10, line, GH_WHITE, GH_DARK, 208);
    if(str_eq(head,"ENVELOPE ROUTING")) {
        cv_rrect(18,65,43,19,2,GH_LBL,GH_DARK);gh_t(39,78,8,"ENV",GH_WHITE,GH_LBL,GA_M);
        cv_rect(63,70,95,14,GH_DARK);cv_line_t(64,75,164,75,GH_RED,1);
        cv_rrect(165,65,53,19,2,GH_LBL,GH_DARK);gh_t(191,78,8,v->val[0],GH_WHITE,GH_LBL,GA_M);
        if (v->cur[0] && v->cur[1])
            gh_edge_dot(v,64,75,164,75);
    } else gh_mstrip(v,20,84,200);
    for (k = 0; k < 2u; k++) {
        gh_knob(70 + 102 * (int32_t)k, 133, 27, gh_r(v, k), 1, GH_CARD);
        gh_t(70 + 102 * (int32_t)k, 172, 9, v->lab[k], GH_INK, GH_CARD, GA_M);
    }
}
static void gh_lfo(const gb_view_t *v, const char *head, uint32_t kind)
{
    int32_t i;
    gh_module(head);
    gh_disp(62, 48);
    if (kind == 0u) {
        for (i = 0; i < 200; i++)
            gh_ys[i] = (int16_t)(86 - tl_sin((uint32_t)(i * (4 + gh_r(v, 0) * 12 / 1000))) * 18 / 256);
        gh_curve(20, 200, gh_ys, GH_RED, 2);
        gh_t(18, 74, 8, v->val[1], GH_BG, GH_DARK, GA_S);
    } else if (kind == 1u) {
        for (i = 0; i < 16; i++)
            cv_rect(20 + i * 13, 90 - (i % 4 == 0 ? 14 : 8), 5, i % 4 == 0 ? 14 : 8, i % 4 == 0 ? GH_RED : GH_GREY);
        gh_t(18, 74, 8, v->cur[0] ? "SYNC ON / NOTE TRIGGER" : "SYNC OFF / NOTE TRIGGER", GH_BG, GH_DARK, GA_S);
    } else {
        int32_t cut = 20 + gh_r(v, 0) * 180 / 1000;
        for (i = 0; i < 200; i++) {
            int32_t y = 20 + i < cut ? 80 : 80 + (20 + i - cut) * 2;
            gh_ys[i] = (int16_t)(y > 104 ? 104 : y);
        }
        gh_curve(20, 200, gh_ys, GH_RED, 2);
        gh_t(18, 74, 8, "RESPONSE PREVIEW", GH_BG, GH_DARK, GA_S);
    }
    if (kind == 1u) {
        if(gh_mark&&v->play){uint32_t col=v->sig_step%16u;gh_dot(22+(int32_t)col*13,90-(col%4u==0u?14:8),GH_DARK);}
    }
    else gh_marker(v, 20, 200);
    gh_knobs4(v);
}

static void gh_synth(const gb_view_t *v, const char *head, uint32_t kind)
{
    uint32_t k;
    int32_t i;
    gh_module(head);
    gh_disp(63, 76);
    if (kind == 1u) {
        static const uint8_t ED[8] = {9, 3, 11, 13, 15, 17, 21, 25};
        uint32_t e = ED[(uint32_t)v->cur[0] & 7u];
        if (e & 1u) cv_line_t(59, 90, 59, 109, GH_RED, 1);
        if (e & 8u) cv_line_t(167, 90, 167, 109, GH_RED, 1);
        if (e & 2u) cv_line_t(167, 90, 59, 109, GH_RED, 1);
        if (e & 4u) cv_line_t(59, 90, 167, 109, GH_RED, 1);
        if(e&16u)cv_line_t(151,80,76,80,GH_RED,1);
        cv_line_t(59, 128, 112, 135, GH_GREY, 1);
        cv_line_t(167, 128, 112, 135, GH_GREY, 1);
        gh_sbox(43, 70, 33, 19, 3, GH_LBL, GH_RED, GH_DARK);
        gh_t(59, 83, 10, "C", GH_WHITE, GH_LBL, GA_M);
        gh_sbox(151, 70, 33, 19, 3, GH_LBL, GH_RED, GH_DARK);
        gh_t(167, 83, 10, "D", GH_WHITE, GH_LBL, GA_M);
        gh_sbox(43, 109, 33, 19, 3, GH_P62, GH_RED, GH_DARK);
        gh_t(59, 122, 10, "A", GH_WHITE, GH_P62, GA_M);
        gh_sbox(151, 109, 33, 19, 3, GH_P62, GH_RED, GH_DARK);
        gh_t(167, 122, 10, "B", GH_WHITE, GH_P62, GA_M);
        if(e&1u)gh_edge_dot(v,59,90,59,109);
        if(e&2u)gh_edge_dot(v,167,90,59,109);
        if(e&4u)gh_edge_dot(v,59,90,167,109);
        if(e&8u)gh_edge_dot(v,167,90,167,109);
        if(e&16u)gh_edge_dot(v,151,80,76,80);
        gh_edge_dot(v,59,128,112,135);gh_edge_dot(v,167,128,112,135);
    } else if (kind == 2u) {
        for (k = 0; k < 4u; k++) {
            int32_t x = 19 + 50 * (int32_t)k;
            cv_line_t(x, 95, x + 42, 71, GH_RED, 1);
            cv_line_t(x + 42, 71, x + 42, 95, GH_RED, 1);
            cv_line_t(x, 128, x, 108, GH_BG, 1);
            cv_line_t(x, 108, x + 21, 108, GH_BG, 1);
            cv_line_t(x + 21, 108, x + 21, 128, GH_BG, 1);
            cv_line_t(x + 21, 128, x + 42, 128, GH_BG, 1);
        }

    } else if (kind == 3u) {
        int32_t a = 26 + gh_r(v, 0) * 40 / 1000, d = a + 30 + gh_r(v, 1) * 120 / 1000;
        cv_line_t(20, 133, a, 70, GH_RED, 1);
        int32_t endx = str_eq(v->val[1], "HOLD") ? 218 : (d > 218 ? 218 : d);
        int32_t endy = str_eq(v->val[1], "HOLD") ? 70 : 133;
        cv_line_t(a, 70, endx, endy, GH_RED, 1);
        cv_rect(18, 131, 4, 4, GH_WHITE);
        cv_rect(a - 2, 68, 4, 4, GH_WHITE);
        cv_rect(endx - 2, endy - 2, 4, 4, GH_WHITE);
        {int16_t px[3]={20,(int16_t)a,(int16_t)endx},py[3]={133,70,(int16_t)endy};gh_path_dot(v,px,py,3);}
    } else if (kind == 4u) {
        cv_line_t(20, 101, 220, 101 - 5 - gh_r(v, 0) * 25 / 1000, GH_RED, 1);
        cv_line_t(20, 101, 220, 101 + 5 + gh_r(v, 0) * 25 / 1000, GH_BG, 1);
        gh_edge_dot(v,20,101,220,(int16_t)(101-5-gh_r(v,0)*25/1000));
        gh_edge_dot(v,20,101,220,(int16_t)(101+5+gh_r(v,0)*25/1000));
    } else if (kind == 5u) {
        int32_t st = 4 + gh_r(v, 0) * 8 / 1000;
        for (i = 0; i < 200; i++)
            gh_ys[i] = (int16_t)(118 - (i * st / 200) * 40 / st);
        gh_curve(20, 200, gh_ys, GH_RED, 1);
        gh_marker(v, 20, 200);
    } else if (kind == 6u) {
        for (i = 0; i < 200; i++) {
            int32_t tau = 40 + gh_r(v, 1) * 120 / 1000, e = 256 * tau / (tau + i * 2);
            int32_t s = tl_sin((uint32_t)(i * (10 + gh_r(v, 0) / 100)));
            gh_ys[i] = (int16_t)clamp(101 - s * 28 / 256 * e / 256, 66, 136);
        }
        gh_curve(20, 200, gh_ys, GH_RED, 1);
        gh_marker(v, 20, 200);
    } else {
        for (i = 0; i < 200; i++) {
            int32_t s = tl_sin((uint32_t)(i * (24 + gh_r(v, 0) / 50)));
            gh_ys[i] = (int16_t)clamp(101 - s * 28 / 256, 66, 136);
        }
        gh_curve(20, 200, gh_ys, GH_RED, 1);
        gh_marker(v, 20, 200);
    }
    for (k = 0; k < 4u; k++) {
        int32_t x = 12 + 55 * (int32_t)k;
        if (!((v->mask >> k) & 1u))
            continue;
        cv_rrect(x, 146, 51, 24, 3, GH_BG, GH_CARD);
        gh_tc(x + 5, 157, 8, v->lab[k], GH_LBL, GH_BG, 44);
        cv_rect(x + 5, 164, 40, 2, GH_GREY);
        cv_rect(x + 5, 164, v->ratio[k] * 40 / 1000, 2, GH_RED);
    }
}

static void gh_sampler(const gb_view_t *v)
{
    int32_t x0 = GX(0), w = GX(220) - x0, y0 = GY(0), h = GY(84) - y0, i, mid = y0 + h / 2;
    gh_grid(x0, y0, w, h, 16, 6);
    if (v->wv != 2u) {
        gh_t(120, y0 + h / 2 + 6, 16, "NO SAMPLE", GH_GREY, GH_DARK, GA_M);
    } else {
        const uint8_t *pk = smp_peaks(v->wkey & 63u);
        for (i = 0; i < w && pk; i++) {
            int32_t col = i * (int32_t)GB_WAVE / w, a = (int32_t)pk[col] * (h / 2 - 4) / 255;
            int in = col >= v->wa && col <= v->wb;
            cv_rect(x0 + i, mid - a, 1, 2 * a + 1, in ? GH_RED : GH_GREY);
        }
        {
            int32_t a = x0 + v->wa * w / (int32_t)GB_WAVE, b = x0 + v->wb * w / (int32_t)GB_WAVE;
            cv_rect(a, y0, 1, h, GH_RED);
            cv_rect(b, y0, 1, h, GH_RED);
            cv_rect(a, y0, b - a, 1, GH_RED);
            cv_rect(a, y0 + h - 1, b - a, 1, GH_RED);
            gh_t(a + 3, y0 + 13, 10, "SLICE", GH_WHITE, GH_DARK, GA_S);
        }
        for (i = 0; v->sl_n && i <= (int32_t)v->sl_n; i++)
            cv_rect(x0 + (int32_t)v->sl_c[i] * w / (int32_t)GB_WAVE, y0, 1, h, GH_WHITE);
        for (i = 0; v->sl_pg && i < (int32_t)v->sl_pn; i++) {
            int32_t x = x0 + (int32_t)v->sl_p[i] * w / (int32_t)GB_WAVE;
            cv_rect(x - 1, y0, 3, 6, GH_WHITE);
            cv_rect(x - 1, y0 + h - 6, 3, 6, GH_WHITE);
        }
        if (v->sl_hd)
            cv_rect(x0 + (int32_t)(v->sl_hd - 1u) * w / (int32_t)GB_WAVE - 1, y0, 2, h, GH_WHITE);
    }
    for (i = 0; i < 16; i++) {
        char n[4];
        int32_t x = GXT(i * 138), ww = GXT(i * 138 + 120) - x;
        fmt_int(n, i + 1);
        gh_pad(x, GY(91), ww, GY(111) - GY(91),
               v->sl_n ? i + 1 == (int32_t)v->sl_last : v->wv == 2u && i == v->wa * 16 / (int32_t)GB_WAVE, n, 9);
    }
}

static void gh_stage(const gb_view_t *v)
{
    uint32_t i, k;
    char b[16];
    for (i = 0; i < v->stg_n && i < NTRK; i++) {
        uint32_t ch = v->stg_trk[i];
        int32_t x = 4 + (int32_t)(i & 1u) * 118, y = 35 + (int32_t)(i >> 1) * 18, pick = v->stg_tgt == ch;
        int mute = (v->stg_mute >> ch) & 1u;
        uint16_t c = pick ? GH_RED : GH_DARK;
        sg_chname(b, ch, v);
        cv_rrect(x, y, 114, 16, 2, c, GH_BG);
        gh_t(x + 5, y + 11, 8, b, mute ? GH_GREY : GH_WHITE, c, GA_S);
        if (mute)
            gh_t(x + 108, y + 11, 8, "M", GH_GREY, c, GA_E);
        else
            for (k = 0; k < 6u; k++)
                cv_rect(x + 80 + 4 * (int32_t)k, y + 5, 2, 7, k < v->stg_lvl[ch] ? (pick ? GH_WHITE : GH_RED) : GH_LBL);
        if ((v->stg_chfx >> ch) & 1u)
            cv_rect(x + 4, y + 13, 106, 1, GH_WHITE);
    }
    if (!v->stg_n)
        gh_t(120, 80, 7, "KEIN PATTERN / KANAL GEW\xc4HLT", GH_LBL, GH_BG, GA_M);
    {
        int pick = v->stg_tgt >= NTRK;
        uint16_t c = pick ? GH_RED : GH_DARK;
        cv_rrect(4, 90, 232, 16, 2, c, GH_BG);
        gh_t(9, 101, 8, "MASTER", GH_WHITE, c, GA_S);
        for (k = 0; k < 6u; k++)
            cv_rect(202 + 4 * (int32_t)k, 95, 2, 7, k < v->stg_lvl[NTRK] ? (pick ? GH_WHITE : GH_RED) : GH_LBL);
        if ((v->stg_chfx >> NTRK) & 1u)
            cv_rect(8, 103, 224, 1, GH_WHITE);
    }
    for (k = 0; k < SG_N; k++) {
        int32_t x = 4 + 59 * (int32_t)(k & 3u), y = 111 + 33 * (int32_t)(k >> 2), on = (v->stg_on >> k) & 1u;
        uint16_t c = on ? GH_RED : GH_DARK;
        if ((v->stg_lat >> k) & 1u)
            gh_sbox(x, y, 55, 29, 2, c, GH_WHITE, GH_BG);
        else
            cv_rrect(x, y, 55, 29, 2, c, GH_BG);
        b[0] = (char)('1' + k);
        b[1] = ' ';
        str_cpy(b + 2, SG_NAME[k], sizeof b - 2u);
        gh_t(x + 4, y + 10, 8, b, GH_WHITE, c, GA_S);
        if (on) {
            char t[12];
            t[0] = (v->stg_lat >> k) & 1u ? 'L' : 'H';
            t[1] = ' ';
            t[2] = '>';
            t[3] = ' ';
            sg_chname(t + 4, v->stg_ch[k], v);
            t[6] = 0;
            if (v->stg_ch[k] >= NTRK)
                str_cpy(t + 4, "MST", 4);
            gh_t(x + 4, y + 21, 7, t, GH_WHITE, c, GA_S);
            if (v->stg_ramp[k])
                cv_rect(x + 4, y + 25, (int32_t)v->stg_ramp[k] * 47 / 100, 1, GH_WHITE);
        } else {
            fmt_int(b, (int32_t)k + 9);
            str_cpy(b + str_len(b), " LATCH", sizeof b - str_len(b));
            gh_t(x + 4, y + 21, 7, b, GH_GREY, c, GA_S);
        }
    }
    for (k = 0; k < 4u; k++) {
        int32_t x = 4 + 59 * (int32_t)k;
        cv_rrect(x, 183, 55, 32, 2, GH_CARD, GH_BG);
        gh_tc(x + 4, 192, 7, v->lab[k], GH_LBL, GH_CARD, 48);
        gh_disc(x + 11, 204, 7, GH_DARK, GH_CARD);
        {
            int32_t px, py;
            gh_pt(x + 11, 204, 5, -135 + v->ratio[k] * 270 / 1000, &px, &py);
            gh_aaline(x + 11, 204, px, py, 2, GH_RED, GH_DARK);
        }
        gh_tc(x + 22, 209, 9, v->val[k], GH_INK, GH_CARD, 32);
    }
}

static void gh_rowlist(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t n, uint32_t pick, const char *const *s,
                       uint32_t first)
{
    uint32_t i;
    for (i = 0; i < n; i++) {
        int32_t yy = y + (int32_t)i * (h + 1);
        uint16_t c = i == pick ? GH_RED : GH_P30;
        char d[4];
        d[0] = (char)('0' + (first + i + 1u) / 10u);
        d[1] = (char)('0' + (first + i + 1u) % 10u);
        d[2] = 0;
        cv_rect(x, yy, w, h, c);
        gh_t(x + 7, yy + h / 2 + 4, 10, d, i == pick ? GH_WHITE : GH_GREY, c, GA_S);
        gh_tc(x + 30, yy + h / 2 + 4, 10, s[i], GH_WHITE, c, w - 34);
    }
}

static void gh_menu(const gb_view_t *v)
{
    const char *t[NPAGES];
    uint32_t i, n = 0, pick = 0, first, count, fam = (uint32_t)v->menu_f - 1u;
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].fam == fam && page_visible(i)) {
            if (i == ui.page) pick = n;
            t[n++] = PAGES[i].title;
        }
    first = pick > 3u ? pick - 3u : 0u;
    if (n > 7u && first + 7u > n) first = n - 7u;
    count = n - first > 7u ? 7u : n - first;
    cv_rect(4, 34, 232, 144, GH_BG);
    cv_rect(GX(0), GY(0), GX(65) - GX(0), GY(114) - GY(0), GH_P30);
    gh_t(GX(5), GY(15), 10, "SELECT", GH_GREY, GH_P30, GA_S);
    gh_t(GX(5), GY(32), 10, "PAGES", GH_BG, GH_P30, GA_S);
    gh_rowlist(GX(67), GY(0), GX(220) - GX(67), GY(15) - GY(0), count, pick - first, t + first, first);

}

static void gh_start(const gb_view_t *v)
{
    static const char *const OPT[3] = {"NEW PROJECT", "LOAD KIT", "RESUME"};
    uint32_t i, k;
    for (i = 0; i < 3u; i++) {
        int32_t y = GY(3 + 35 * (int32_t)i), h = GY(37 + 35 * (int32_t)i) - y, on = i == v->bm_sel;
        uint16_t c = on ? GH_RED : GH_P30;
        char d[3] = {'0', (char)('1' + i), 0};
        cv_rect(GX(42), y, GX(220) - GX(42), h, c);
        gh_t(GX(49), GY(16 + 35 * (int32_t)i), 10, d, on ? GH_WHITE : GH_GREY, c, GA_S);
        gh_t(GX(71), GY(16 + 35 * (int32_t)i), 10, OPT[i], GH_WHITE, c, GA_S);
        gh_sbox(GX(0), y, GX(34) - GX(0), GY(35 + 35 * (int32_t)i) - y, 3, GH_P30, GH_P62, GH_BG);
        for (k = 0; k < 4u; k++)
            cv_rrect(GX(6 + 11 * (int32_t)(k & 1u)), GY(9 + 35 * (int32_t)i + 11 * (int32_t)(k >> 1)), GX(14) - GX(6),
                     GY(17) - GY(9), 1, on ? GH_RED : GH_GREY, GH_P30);
    }
    gh_hint("SELECT: Auswahl", "OCT+ OK");
}

static void gh_kits(const gb_view_t *v)
{
    uint32_t i, first = v->bm_kit > 2u ? v->bm_kit - 2u : 0u;
    if (first + 5u > 11u)
        first = 6u;
    for (i = first; i < first + 5u; i++) {
        int32_t r = (int32_t)(i - first), y = GY(22 * r), h = GY(21 + 22 * r) - y, on = i == v->bm_kit;
        uint16_t c = on ? GH_RED : GH_P30;
        char d[4], b[16];
        const char *nm = trap_kit_name(i);
        d[0] = (char)('0' + (i + 1u) / 10u);
        d[1] = (char)('0' + (i + 1u) % 10u);
        d[2] = 0;
        if (!nm) {
            str_cpy(b, "KIT ", sizeof b);
            fmt_int(b + 4, (int32_t)i + 1);
            nm = b;
        }
        cv_rect(GX(0), y, GX(220) - GX(0), h, c);
        gh_t(GX(7), GY(13 + 22 * r), 10, d, on ? GH_WHITE : GH_GREY, c, GA_S);
        gh_tc(GX(29), GY(13 + 22 * r), 10, (v->bm_used >> i) & 1u ? nm : "\x97", GH_WHITE, c, GX(215) - GX(29));
    }
    gh_hint("OCT+ OK", "OCT- Zur\xfc" "ck");
}

static void gh_newbank(const gb_view_t *v)
{
    uint32_t i, lit = (uint32_t)v->bm_left * 8u / 16u;
    gh_t(GX(1), GY(12), 10, "BANK / INITIALISE", GH_BG, GH_BG, GA_S);
    for (i = 0; i < 8u; i++) {
        int32_t c = (int32_t)(i & 3u), r = (int32_t)(i >> 2), x = GX(55 * c), y = GY(23 + 32 * r);
        char n[2] = {(char)('A' + i), 0};
        gh_pad(x, y, GX(55 * c + 50) - x, GY(50 + 32 * r) - y, i < lit, n, 10);
    }
    cv_rect(GX(0), GY(101), GX(220) - GX(0), GY(109) - GY(101), GH_LBL);
    cv_rect(GX(0), GY(101), (GX(220) - GX(0)) * (int32_t)v->bm_left / 16, GY(109) - GY(101), GH_RED);
    gh_hint("OCT+ OK", "OCT- Zur\xfc" "ck");
}

static void gh_save(const gb_view_t *v)
{
    uint32_t i;
    int sv = v->sv == 1u;
    cv_rrect(4, 34, 232, 26, 3, GH_DARK, GH_BG);
    cv_rrect(10, 39, 61, 16, 2, GH_RED, GH_DARK);
    gh_t(40, 50, 10, sv ? "SAVE" : "LOAD", GH_WHITE, GH_RED, GA_M);
    gh_t(82, 51, 10, "SOUND / KIT", GH_WHITE, GH_DARK, GA_S);
    if (v->flash) {
        if (v->flash == 1u)
            gh_dialog(v->msg[0], v->msg[1]);
        gh_hint(sv ? "SAVE halten + Taste" : "Taste: laden", "16 Sounds / 11 Kits");
        return;
    }
    gh_t(5, 76, 9, "SOUND / WHITE KEYS", GH_INK, GH_BG, GA_S);
    gh_t(160, 76, 9, "KIT / BLACK", GH_INK, GH_BG, GA_S);
    for (i = 0; i < 16u; i++) {
        int32_t x = 4 + 37 * (int32_t)(i & 3u), y = 82 + 23 * (int32_t)(i >> 2), used = (v->su >> i) & 1u;
        char n[3] = {(char)('0' + (i + 1u) / 10u), (char)('0' + (i + 1u) % 10u), 0};
        uint16_t c = used ? GH_DARK : GH_CARD;
        gh_sbox(x, y, 33, 20, 3, c, GH_GREY, GH_BG);
        gh_t(x + 16, y + 14, 10, n, used ? GH_WHITE : GH_INK, c, GA_M);
    }
    for (i = 0; i < 11u; i++) {
        int32_t x = 160 + 26 * (int32_t)(i % 3u), y = 82 + 23 * (int32_t)(i / 3u), used = (v->ku >> i) & 1u;
        char n[3] = {(char)('0' + (i + 1u) / 10u), (char)('0' + (i + 1u) % 10u), 0};
        uint16_t c = used ? GH_DARK : GH_CARD;
        cv_rrect(x, y, 23, 20, 3, c, GH_BG);
        gh_t(x + 11, y + 14, 9, n, used ? GH_WHITE : GH_INK, c, GA_M);
    }
    gh_hint(sv ? "SAVE halten + Taste" : "Taste: laden", "16 Sounds / 11 Kits");
}
static void gh_gl(const gb_view_t *v);

#include "trap_approved_ui.c"

static void gh_scene(const gb_view_t *v, uint16_t bgc)
{
    const page_t *pg = cur_page();
    const char *t = pg->title;
    uint32_t grp = gh_group(v);
    char ttl[28];
    (void)bgc;
    cv_rect(0, 0, 240, 240, GH_BG);
    if (v->bm == 1u) {
        ws_image(WS_START_W29 + v->bm_sel % 3u, 0, 0);
        gh_status(v, "TRAPSTATION / PROJECT");
        return;
    }
    if (v->bm) {
        static const char *const ST[3] = {"START / NEW", "START / LOAD", "START / RESUME"};
        gh_status(v, 0);
        gh_title(v, v->bm == 2u ? "KIT LOAD" : v->bm == 3u ? (v->bm_left >= 16u ? "NEW / READY" : "NEW / BANK START") :
                 ST[v->bm_sel % 3u], GG_DAW);
        gh_footer(v, GG_DAW);
        if (v->bm == 2u)
            gh_kits(v);
        else if (v->bm == 3u)
            gh_newbank(v);
        else
            gh_start(v);
        return;
    }
    if (v->sv) {
        gh_status(v, 0);
        gh_title(v, v->sv == 1u ? "SAVE / SOUND + KIT" : "LOAD / SOUND + KIT", GG_DAW);
        gh_footer(v, GG_DAW);
        gh_save(v);
        return;
    }
    if (v->gl_mode) {
        gh_gl(v);
        return;
    }
    str_cpy(ttl, t, sizeof ttl);
    if ((pg->scope == SC_ENGINE || pg->scope == SC_SLICE) && !v->fx) {
        str_cpy(ttl, gh_pill(v->mach), sizeof ttl);
        str_cpy(ttl + str_len(ttl), " / ", sizeof ttl - str_len(ttl));
        str_cpy(ttl + str_len(ttl), t, sizeof ttl - str_len(ttl));
    }
    if (v->cfm) {
        static const char *const CQ[] = {"", "CLEAR SEQUENCE?", "CLEAR TRACK?", "", "", "", "", "", "INIT SOUND?",
                                         "CLEAR MOTION?", "", "", "CLEAN SOUND?", "GENERATE CHORDS?", "", "MAKE EUCLID?",
                                         "GENERATE DRUMS?", "GENERATE HATS?"};
        const char *q = v->cfm < NELEM(CQ) && CQ[v->cfm][0] ? CQ[v->cfm] : v->cfa;
        if (v->cfm == CF_PHONK)
            q = ph_request == PH_BMAKE ? "GENERATE BASS?" : ph_request == PH_FMAKE ? "MAKE FILL?" : "GENERATE CHORDS?";
        str_cpy(ttl, "CONFIRM / ", sizeof ttl);
        str_cpy(ttl + 10, q, sizeof ttl - 10u);
        if (str_len(ttl) && ttl[str_len(ttl) - 1u] == '?')
            ttl[str_len(ttl) - 1u] = 0;
        gh_status(v, 0);
        gh_title(v, ttl, GG_DAW);
        gh_footer(v, GG_DAW);
        gh_dialog(q, "OCT- BACK / OCT+ OK");
        gh_hint("OCT- CANCEL", "OCT+ OK");
        return;
    }
    if(!v->menu_f && !v->bn && !v->cross) {
        uint32_t gi=gen_index(pg);
        if(gi<4u) {ap_scene(v,gi);return;}
        if(str_eq(t,"TAPE")){ap_scene(v,4);return;}
        if(str_eq(t,"VHS")){ap_scene(v,5);return;}
        if(str_eq(t,"VINYL")){ap_scene(v,6);return;}
    }
    if (v->stg && !v->menu_f) {
        char b[16];
        cv_rect(0, 0, 240, 17, GH_DARK);
        cv_rrect(4, 2, 84, 13, 2, GH_RED, GH_DARK);
        b[0] = '>';
        b[1] = ' ';
        sg_chname(b + 2, v->stg_tgt, v);
        gh_t(8, 12, 9, b, GH_WHITE, GH_RED, GA_S);
        b[0] = '\x86';
        b[1] = ' ';
        fmt_int(b + 2, v->bpm);
        gh_t(235, 12, 9, b, GH_WHITE, GH_DARK, GA_E);
    } else
        gh_status(v, 0);
    if (v->menu_f) {
        static const char *const FN[FAM_COUNT] = {"HOME", "ENV", "LFO", "EDIT", "SCL", "FX", "GLO", "SAVE", "ARP", "SEQ",
                                                  "HOME", "SCL"};
        uint32_t f = (uint32_t)v->menu_f - 1u;
        str_cpy(ttl, "MENU / ", sizeof ttl);
        str_cpy(ttl + 7, FN[f % FAM_COUNT], sizeof ttl - 7u);
        if (f == FAM_EDIT && !v->fx) {
            str_cpy(ttl + str_len(ttl), " / ", sizeof ttl - str_len(ttl));
            str_cpy(ttl + str_len(ttl), gh_pill(v->mach), sizeof ttl - str_len(ttl));
        } else if (f == FAM_FX || f == FAM_FLT || f == FAM_ENV || f == FAM_LFO)
            str_cpy(ttl + str_len(ttl), v->fx ? " / FX" : f == FAM_FX ? " / TRACK" : "", sizeof ttl - str_len(ttl));
    }
    gh_mark = (uint8_t)(!v->fx && gh_group(v) == GG_SYN);
    gh_title(v, ttl, grp);
    if (str_eq(t, "PIANO ROLL") || str_eq(t, "QUICK FILL"))
        gh_t(198, 29, 7, v->pr_mode ? "SINGLE" : "CHORD", GH_LBL, GH_BG, GA_E);
    gh_footer(v, grp);
    if (v->bn && !v->menu_f) {
        cv_rect(4, 34, 232, 144, GH_BG);
        gh_dialog(v->bt[2u * v->bpg], v->bt[2u * v->bpg + 1u][0] ? v->bt[2u * v->bpg + 1u] : "OCT- BACK / OCT+ OK");
        gh_hint(0, 0);
        return;
    }
    if (v->menu_f) {
        gh_menu(v);
        gh_hint("Taste halten + SELECT", "NAVIGATION");
        return;
    }
    if (v->cross) {
        cv_rrect(4, 34, 232, 144, 4, GH_CARD, GH_BG);
        gh_t(120, 110, 13, "NOT ON THE FX TRACK", GH_LBL, GH_CARD, GA_M);
        gh_cards(v);
        return;
    }
    if (v->stg) {
        gh_stage(v);
        cv_rect(0, 220, 240, 20, GH_DARK);
        gh_t(5, 233, 9, "1\x96" "8 HOLD", GH_WHITE, GH_DARK, GA_S);
        gh_t(112, 233, 9, "9\x96" "16 LATCH", GH_WHITE, GH_DARK, GA_S);
        gh_t(235, 234, 9, "A", GH_WHITE, GH_DARK, GA_E);
        return;
    }
    if (v->perf) {
        gh_perf(v);
    } else if (str_eq(t, "AMP")) {
        gh_amp(v);
    } else if (str_eq(t, "SENDS")) {
        gh_sends(v);
    } else if (str_eq(t, "DELAY")) {
        ws_effect(v, 0);
    } else if (str_eq(t, "VERB")) {
        gh_title(v, "REVERB", GG_DAW);
        ws_effect(v, 1);
    } else if (str_eq(t, "LFO 1") || str_eq(t, "LFO 2") || str_eq(t, "LFO 3")) {
        gh_preview(v, "WAVE / PREVIEW", GC_LFO, gh_r(v, 0), v->cur[1], 0);
    } else if (str_eq(t, "DJ")) {
        gh_preview(v, "DJ / PREVIEW", GC_DJ, v->cur[0], 0, 0);
    } else if (str_eq(t, "COMB")) {
        gh_preview(v, "COMB / PREVIEW", GC_COMB, gh_r(v, 1), gh_r(v, 2), 0);
    } else if (str_eq(t, "BANKS")) {
        gh_banks(v);
    } else if (pg->graph == GR_GBSEQ || pg->graph == GR_ACID || pg->scope == SC_SFN) {
        gh_steps(v);
    } else if (str_eq(t, "CLOCK")) {
        gh_status(v, "MASTER / CLOCK");
        ws_clock(v);
    } else if (str_eq(t, "SCENES")) {
        gh_scenes(v);
    } else if (str_eq(t, "SEQUENCER")) {
        gh_sequencer(v);
    } else if (str_eq(t, "PIANO ROLL") || str_eq(t, "QUICK FILL")) {
        gh_roll(v);
    } else if (pg->scope == SC_TRK) {
        gh_mix(v);
    } else if (str_eq(t, "SCALE") || str_eq(t, "CHORD")) {
        gh_keys(v, 0);
    } else if (str_eq(t, "ARP")) {
        gh_keys(v, 1);
    } else if (str_eq(t, "CHORD GEN")) {
        ws_chordgen(v);
    } else if (str_eq(t, "CHORD VAR")) {
        gh_chordvar(v);
    } else if (str_eq(t, "EUCLID")) {
        gh_euclid(v);
    } else if (str_eq(t, "ENV") || str_eq(t, "FILTER ENV")) {
        gh_env(v, str_eq(t, "ENV") ? (MACHINES[v->mach % NMACH].drum ? "DRUM / ENV" : "AMP ENVELOPE / ADSR") : "FILTER ENVELOPE");
    } else if (str_eq(t, "VOL PAN")) {
        char l[32];
        str_cpy(l, "LEVEL ", sizeof l);
        str_cpy(l + str_len(l), v->val[0], sizeof l - str_len(l));
        str_cpy(l + str_len(l), " / PAN ", sizeof l - str_len(l));
        str_cpy(l + str_len(l), v->val[1], sizeof l - str_len(l));
        gh_two(v, "LEVEL / PAN", l);
    } else if (str_eq(t, "ENV DEST")) {
        char l[32];
        str_cpy(l, "ENV > ", sizeof l);
        str_cpy(l + str_len(l), v->val[0], sizeof l - str_len(l));
        str_cpy(l + str_len(l), " / AMOUNT ", sizeof l - str_len(l));
        str_cpy(l + str_len(l), v->val[1], sizeof l - str_len(l));
        gh_two(v, "ENVELOPE ROUTING", l);
    } else if (str_eq(t, "LFO") && !v->fx) {
        gh_lfo(v, "LFO / MODULATION", 0);
    } else if (str_eq(t, "SYNC")) {
        gh_lfo(v, "SYNC / TRIGGER", 1);
    } else if (str_eq(t, "FILTER")) {
        gh_lfo(v, "FILTER / RESPONSE", 2);
    } else if (pg->scope == SC_SLICE) {
        gh_sampler(v);
    } else if (pg->scope == SC_ENGINE) {
        const machine_t *m = &MACHINES[v->mach % NMACH];
        uint32_t e = m->engine;
        if (e == ENGI_SAMPLER)
            gh_sampler(v);
        else if (m->drum && !str_eq(m->pill, "8BAS") &&
                 !(m->icon == IC_CYMBAL || (e == ENGI_DIGITAL && (m->icon == IC_HATS || m->icon == IC_PERC)))) {
            char c[28];
            str_cpy(c, ttl, sizeof c);
            str_cpy(c + str_len(c), " PREVIEW", sizeof c - str_len(c));
            gh_preview(v, c, GC_DRUM, gh_r(v, 0), gh_r(v, 1), 0);
        } else
            gh_synth(v, ttl, str_eq(t, "OSC 1") && e == ENGI_DIGITAL && !m->drum ? 1u :
                     str_eq(t, "OSC 1") && e == ENGI_MONO ? 2u : str_eq(t, "MOD C") || str_eq(t, "MOD D") ? 3u :
                     str_eq(t, "TUNE") ? 4u : str_eq(t, "COLOR") || str_eq(t, "CRUSH") ? 5u :
                     str_eq(m->pill, "8BAS") ? 6u : 0u);
    } else {
        cv_rrect(4, 34, 232, 144, 4, GH_CARD, GH_BG);
    }
    gh_cards(v);
}

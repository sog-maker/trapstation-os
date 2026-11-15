/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION contributors */

#include "ui_fonts_gh.h"
#define GH_HEX(h) GB_RGB(((h) >> 16) & 255, ((h) >> 8) & 255, (h) & 255)
#define GH_BG GH_HEX(0xc9c9c9)
#define GH_DARK GH_HEX(0x242424)
#define GH_RED GH_HEX(0xc62d39)
#define GH_WHITE GH_HEX(0xf2f2f2)
#define GH_INK GH_HEX(0x171717)
#define GH_GREY GH_HEX(0x929292)
#define GH_CARD GH_HEX(0xe2e2e2)
#define GH_LBL GH_HEX(0x414141)
#define GH_P30 GH_HEX(0x303030)
#define GH_P62 GH_HEX(0x626262)
#define GH_PINK GH_HEX(0xf2a5ab)
#define GH_G1 GH_HEX(0x2c2c2c)
#define GH_G2 GH_HEX(0x353535)
static const uint32_t GH_TRK_HEX[NTRK] = {0x621a28, 0x872235, 0xac2940, 0xca4551, 0xdc7179, 0xed9ca1};
#define GH_TRK(k) GH_HEX(GH_TRK_HEX[(k) % NTRK])
#define GX(x) (4 + (int32_t)((x) * 10545 + 5000) / 10000)
#define GY(y) (34 + (int32_t)((y) * 125 + 50) / 100)
#define GXT(x) (4 + (int32_t)((x) * 10545 + 50000) / 100000)
#define GYT(y) (34 + (int32_t)((y) * 125 + 500) / 1000)
enum { GA_S, GA_M, GA_E };

static const aafont_t *gh_f(uint32_t sz)
{
    return sz == 23u ? &AF_GC : sz <= 8u ? &AF_GS : sz == 9u ? &AF_GM : sz <= 11u ? &AF_GT : sz == 12u ? &AF_GV : sz == 13u ? &AF_GB :
           sz <= 19u ? &AF_GH : &AF_GX;
}

static int32_t gh_t(int32_t x, int32_t base, uint32_t sz, const char *s, uint16_t fg, uint16_t bg, uint32_t anc)
{
    const aafont_t *f = gh_f(sz);
    int32_t w = anc == GA_S ? 0 : text_w(f, s);
    x -= anc == GA_M ? w / 2 : anc == GA_E ? w : 0;
    return cv_text_on(x, base - f->asc, f, s, fg, bg);
}

static void gh_tc(int32_t x, int32_t base, uint32_t sz, const char *s, uint16_t fg, uint16_t bg, int32_t maxw)
{
    const aafont_t *f = gh_f(sz);
    cv_free_text(x, base - f->asc, f, s, fg, bg, maxw);
}

static void gh_img(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *px)
{
    int32_t r, r0 = -cv_oy - y, r1 = -cv_oy + (int32_t)cv_h - y, c0 = x < 0 ? -x : 0, c1 = x + w > (int32_t)cv_w ? (int32_t)cv_w - x : w;
    for (r = r0 > 0 ? r0 : 0; r < h && r < r1; r++) {
        uint16_t *d = &cv_px[(uint32_t)(y + r + cv_oy) * cv_w + (uint32_t)x];
        const uint16_t *s = px + r * w;
        int32_t c;
        for (c = c0; c < c1; c++)
            d[c] = s[c];
    }
}
static void gh_box(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t c, uint16_t under)
{
    if (r)
        cv_rrect(x, y, w, h, r, c, under);
    else
        cv_rect(x, y, w, h, c);
}
static void gh_sbox(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t c, uint16_t st, uint16_t under)
{
    gh_box(x, y, w, h, r, st, under);
    gh_box(x + 1, y + 1, w - 2, h - 2, r > 1 ? r - 1 : 0, c, st);
}
static void gh_pt(int32_t cx, int32_t cy, int32_t r, int32_t deg, int32_t *x, int32_t *y)
{
    dial_pt(cx, cy, r, ((deg % 360) + 360) % 360, x, y);
}
static uint16_t gh_mixc(uint16_t a, uint16_t b, uint32_t t)
{
    int32_t ar = a >> 11, ag = (a >> 5) & 63, ab = a & 31, br = b >> 11, bg = (b >> 5) & 63, bb = b & 31;
    ar += ((br - ar) * (int32_t)t) / 256;
    ag += ((bg - ag) * (int32_t)t) / 256;
    ab += ((bb - ab) * (int32_t)t) / 256;
    return (uint16_t)(ar << 11 | ag << 5 | ab);
}
static void gh_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c, uint16_t under)
{
    int32_t x, y, r2o = (r * 4 + 2) * (r * 4 + 2), r2i = (r * 4 - 2) * (r * 4 - 2);
    int32_t y0 = -cv_oy - cy, y1 = -cv_oy + (int32_t)cv_h - 1 - cy;
    for (y = y0 > -r ? y0 : -r; y <= r && y <= y1; y++)
        for (x = -r; x <= r; x++) {
            int32_t d = 16 * (x * x + y * y);
            if (d <= r2i)
                cv_rect(cx + x, cy + y, 1, 1, c);
            else if (d < r2o) {
                uint32_t n = 0, sx, sy;
                for (sy = 0; sy < 4u; sy++)
                    for (sx = 0; sx < 4u; sx++) {
                        int32_t px = 8 * x - 3 + 2 * (int32_t)sx, py = 8 * y - 3 + 2 * (int32_t)sy;
                        n += (uint32_t)(px * px + py * py <= 64 * r * r);
                    }
                if (n)
                    cv_rect(cx + x, cy + y, 1, 1, gh_mixc(under, c, n * 16u));
            }
        }
}
static uint32_t gh_isqrt(uint32_t v)
{
    uint32_t r = 0, b = 1u << 30;
    while (b > v)
        b >>= 2;
    while (b) {
        if (v >= r + b) {
            v -= r + b;
            r = (r >> 1) + b;
        } else
            r >>= 1;
        b >>= 2;
    }
    return r;
}

static void gh_plot(int32_t x, int32_t y, uint32_t t, uint16_t c, uint16_t under)
{
    int32_t yy = y + cv_oy, ur = under >> 11, ug = (under >> 5) & 63, ub = under & 31, cr = (c >> 11) - ur, cg = ((c >> 5) & 63) - ug,
            cb = (c & 31) - ub, n = cr * cr + cg * cg + cb * cb, have;
    uint16_t p;
    if ((uint32_t)x >= cv_w || (uint32_t)yy >= cv_h)
        return;
    p = swap16(cv_px[(uint32_t)yy * cv_w + (uint32_t)x]);
    have = n ? (((p >> 11) - ur) * cr + (((p >> 5) & 63) - ug) * cg + ((p & 31) - ub) * cb) * 256 / n : 0;
    if ((int32_t)t > have)
        cv_px[(uint32_t)yy * cv_w + (uint32_t)x] = swap16(t >= 256u ? c : gh_mixc(under, c, t));
}

static void gh_aaline(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, uint16_t c, uint16_t under)
{
    int32_t xa = (x0 < x1 ? x0 : x1) - w, xb = (x0 > x1 ? x0 : x1) + w, ya = (y0 < y1 ? y0 : y1) - w, yb = (y0 > y1 ? y0 : y1) + w;
    int32_t dx = (x1 - x0) * 16, dy = (y1 - y0) * 16, l2 = dx * dx + dy * dy, x, y, hw = w * 8;
    if (ya < -cv_oy)
        ya = -cv_oy;
    if (yb > -cv_oy + (int32_t)cv_h - 1)
        yb = -cv_oy + (int32_t)cv_h - 1;
    for (y = ya; y <= yb; y++)
        for (x = xa; x <= xb; x++) {
            int32_t px = (x - x0) * 16, py = (y - y0) * 16, t = l2 ? (px * dx + py * dy) * 256 / l2 : 0, ex, ey, d, cov;
            t = clamp(t, 0, 256);
            ex = px - dx * t / 256;
            ey = py - dy * t / 256;
            d = ex * ex + ey * ey;
            if (d >= (hw + 8) * (hw + 8))
                continue;
            if (d < (hw - 7) * (hw - 7)) {
                gh_plot(x, y, 256u, c, under);
                continue;
            }
            d = (int32_t)gh_isqrt((uint32_t)d);
            cov = hw + 8 - d;
            if (cov <= 0)
                continue;
            gh_plot(x, y, cov >= 16 ? 256u : (uint32_t)cov * 16u, c, under);
        }
}

static void gh_knob(int32_t cx, int32_t cy, int32_t r, int32_t ratio, int dots, uint16_t under)
{
    int32_t k, px, py, deg = -135 + ratio * 270 / 1000;
    if (dots)
        for (k = 0; k < 13; k++) {
            int32_t a = -135 + k * 270 / 12;
            gh_pt(cx, cy, r + 4, a, &px, &py);
            cv_rect(px - 1, py - 1, 2, 2, a <= deg ? GH_RED : GH_GREY);
        }
    gh_disc(cx, cy, r, GH_GREY, under);
    gh_disc(cx, cy, r - 2, GH_LBL, GH_GREY);
    gh_disc(cx, cy, r - 5, GH_DARK, GH_LBL);
    gh_pt(cx, cy, r - 5, deg, &px, &py);
    gh_aaline(cx, cy, px, py, 2, GH_WHITE, GH_DARK);
}

static void gh_grid(int32_t x, int32_t y, int32_t w, int32_t h, int32_t nx, int32_t ny)
{
    int32_t k;
    cv_rect(x, y, w, h, GH_DARK);
    for (k = 1; k < nx; k++)
        cv_rect(x + k * w / nx, y, 1, h, k % 4 ? GH_G1 : GH_G2);
    for (k = 1; k < ny; k++)
        cv_rect(x, y + k * h / ny, w, 1, GH_G2);
}

static void gh_aaline(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t w, uint16_t c, uint16_t under);
static void gh_curve(int32_t x0, int32_t w, const int16_t *ys, uint16_t c, int32_t thick)
{
    int32_t i;
    for (i = 1; i < w; i++)
        gh_aaline(x0 + i - 1, ys[i - 1], x0 + i, ys[i], thick, c, GH_DARK);
}

enum { GG_DAW, GG_MIDI, GG_SYN };
static uint32_t gh_group(const gb_view_t *v)
{
    const page_t *p = cur_page();
    if (v->bm || v->sv || v->stg || v->gl_mode || v->menu_f || ui.home)
        return GG_DAW;
    if (p->fam == FAM_ARP)
        return GG_MIDI;
    if (!v->fx && (p->scope == SC_ENGINE || p->scope == SC_SLICE || p->fam == FAM_ENV || p->fam == FAM_FLT ||
                   (p->fam == FAM_LFO && p->scope == SC_TRACK)))
        return GG_SYN;
    return GG_DAW;
}
static const char *gh_pill(uint32_t mach)
{
    return MACHINES[mach % NMACH].pill;
}
static void gh_status(const gb_view_t *v, const char *left)
{
    char b[24], n[12];
    cv_rect(0, 0, 240, 17, GH_DARK);
    if (left)
        str_cpy(b, left, sizeof b);
    else {
        b[0] = 'T';
        b[1] = (char)('1' + v->sel % NTRK);
        str_cpy(b + 2, " / ", sizeof b - 2u);
        str_cpy(b + 5, gh_pill(v->tm[v->sel % NTRK]), sizeof b - 5u);
    }
    gh_t(5, 12, 9, b, GH_WHITE, GH_DARK, GA_S);
    n[0] = '\x86';
    n[1] = ' ';
    fmt_int(n + 2, v->bpm);
    gh_t(235, 12, 9, n, GH_WHITE, GH_DARK, GA_E);
}
static void gh_title(const gb_view_t *v, const char *title, uint32_t grp)
{
    static const char *const PILL[3] = {"DAW", "MIDI", "SYN"};
    gh_tc(4, 29, 10, title ? title : v->title, GH_INK, GH_BG, 196);
    cv_rrect(205, 20, 31, 12, 2, GH_RED, GH_BG);
    gh_t(220, 29, 8, PILL[grp % 3u], GH_WHITE, GH_RED, GA_M);
}

static void gh_card(int32_t x, const char *lab, const char *val)
{
    cv_rrect(x, 183, 55, 32, 2, GH_CARD, GH_BG);
    gh_tc(x + 4, 193, 8, lab && lab[0] ? lab : "\x97", GH_LBL, GH_CARD, 48);
    gh_tc(x + 4, 209, 12, val && val[0] ? val : "\x97", GH_INK, GH_CARD, 48);
}
static void gh_cards(const gb_view_t *v)
{
    uint32_t k;
    for (k = 0; k < 4u; k++)
        gh_card(4 + 59 * (int32_t)k, (v->mask >> k) & 1u ? v->lab[k] : 0, (v->mask >> k) & 1u ? v->val[k] : 0);
}

static void gh_hint(const char *a, const char *b)
{
    cv_rect(4, 181, 232, 1, GH_GREY);
    if (a)
        gh_tc(7, 195, 9, a, GH_INK, GH_BG, 226);
    if (b)
        gh_tc(7, 210, 9, b, GH_LBL, GH_BG, 226);
}
static void gh_footer(const gb_view_t *v, uint32_t grp)
{
    static const char *const KM[6] = {"KEY", "SEQ", "PARAM", "KEY", "LOCKS", "ACID"};
    uint32_t k;
    cv_rect(0, 220, 240, 20, GH_DARK);
    gh_t(5, 234, 9, grp == GG_MIDI && v->kmode == 3u ? "ARP" : KM[v->kmode % 6u], GH_WHITE, GH_DARK, GA_S);
    for (k = 0; k < NTRK; k++) {
        int32_t x = 48 + 25 * (int32_t)k, me = !v->fx && k == v->sel % NTRK, mute = (v->tmute >> k) & 1u;
        int32_t hit = (v->thit >> k) & 1u;
        char d[2] = {(char)('1' + k), 0};
        uint16_t c = mute ? GH_LBL : GH_TRK(k);
        gh_sbox(x, 224, 21, 12, 1, c, me || hit ? GH_WHITE : GH_GREY, GH_DARK);
        gh_t(x + 10, 233, 8, d, mute ? GH_GREY : k < 4u ? GH_WHITE : GH_INK, c, GA_M);
    }
    gh_t(235, 234, 9, grp == GG_MIDI ? "B" : grp == GG_SYN ? "C" : "A", GH_WHITE, GH_DARK, GA_E);
}

static void gh_module(const char *head)
{
    cv_rrect(4, 34, 232, 144, 4, GH_CARD, GH_BG);
    cv_rrect(4, 34, 232, 21, 3, GH_DARK, GH_CARD);
    cv_rect(4, 34, 3, 21, GH_RED);
    gh_tc(13, 48, 10, head, GH_WHITE, GH_DARK, 200);
    cv_rrect(220, 41, 7, 7, 2, GH_RED, GH_DARK);
}

static void gh_dialog(const char *msg, const char *sub)
{
    int32_t x = GXT(40), y = GYT(160), w = GXT(2160) - x, h = GYT(1010) - y;
    gh_sbox(x, y, w, h, 3, GH_P30, GH_P62, GH_BG);
    cv_rect(x, y, GXT(80) - x, h, GH_RED);
    if (text_w(gh_f(13), msg) <= w - 12)
        gh_t(x + w / 2, GYT(500), 13, msg, GH_WHITE, GH_P30, GA_M);
    else
        gh_tc(x + 8, GYT(500), 13, msg, GH_WHITE, GH_P30, w - 12);
    if (sub)
        gh_t(x + w / 2, GYT(800), 9, sub, GH_BG, GH_P30, GA_M);
}

static void gh_theme(void)
{
    uint32_t i;
    ux.bg = GH_BG;
    ux.surf = GH_CARD;
    ux.text = GH_INK;
    ux.theme = GH_RED;
    ux.accent = GH_DARK;
    ux.mid = GH_LBL;
    ux.dim = GH_GREY;
    ux.line = GH_GREY;
    ux.sel = GH_RED;
    ux.tint = GH_P30;
    ux.ink = GH_INK;
    ux.raise = GH_GREY;
    ux.key = GH_GREY;
    ux.lane = GH_P30;
    ux.grid = GH_LBL;
    ux.rec = GH_RED;
    ux.light = ux.mono = 0;
    ux.gen++;
    for (i = 0; i < NTRK; i++)
        gb_col[i] = GH_TRK(i);
}

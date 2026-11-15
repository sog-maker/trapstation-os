/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

typedef struct { uint16_t off, adv; int8_t bx; uint8_t by, bw, bh; } aag_t;
typedef struct {
    uint8_t h, asc, first, last, nex;
    uint16_t nk;
    const aag_t *g;
    const uint8_t *data;
    const uint16_t *kkey;
    const int8_t *kd;
    const uint8_t *ex;
    uint8_t psh;
    const uint8_t *hc;
} aafont_t;
typedef struct { const char *name; uint16_t bg, surf, text, theme, accent; } ui_pal_t;
typedef struct { uint16_t off; uint8_t w; const char *label; } kc_t;
#include "ui_fonts.h"
#include "ui_palettes.h"
#include "ui_keycaps.h"
#define ELLIPSIS '\x85'

#ifndef GFX_HOOK_TEXT
#define GFX_HOOKS 0
#define GFX_HOOK_TEXT(x0, y0, x1, y1, s, flags) ((void)0)
#define GFX_HOOK_BLIT(x, y, r0) ((void)0)
#define GFX_HOOK_BEGIN() ((void)0)
#define GFX_HOOK_PIXELS(n) ((void)0)
#endif
#ifndef GFX_HOOKS
#define GFX_HOOKS 1
#endif
#ifndef GFX_HOOK_CELL
#define GFX_HOOK_CELL(x0, y0, x1, y1) ((void)0)
#endif
#ifndef GFX_HOOK_INK
#define GFX_HOOK_INK(ink, bg) ((void)0)
#endif
#ifndef GFX_HOOK_RULE
#define GFX_HOOK_RULE(x, y, w, h) ((void)0)
#endif

#ifndef GFX_HOOK_ALIGN
#define GFX_HOOK_ALIGN(x0, y0, x1, y1, mode, tag) ((void)0)
#define GFX_HOOK_PEN(y, f) ((void)0)
#define GFX_HOOK_ITEM(x0, y0, x1, y1) ((void)0)
#define GFX_HOOK_ICON(x, y, size, id) ((void)0)
#endif
#define AL_H 1u
#define AL_V 2u
#define AL_HV 3u
#define AL_B 4u
#define AL_R 8u
#define AL_PASS 16u
#define AL_CELLS 32u
#define AL_N(n) ((uint32_t)(n) << 8)

#define CV_MAX (240u * 124u)
static uint16_t cv_px[CV_MAX] __attribute__((section(".pool")));
static uint32_t cv_w, cv_h;
static uint16_t cv_bg;
static uint8_t cv_scroll;
static int32_t cv_oy;
static int16_t cv_cy0, cv_cy1;
static uint8_t cv_flip;

#define RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))

static struct {
    uint16_t bg, surf, text, theme, accent;
    uint16_t mid, dim, line, sel, tint, ink, rec, raise, key, lane, grid;
    uint8_t light, mono;
    uint8_t style;
    uint32_t gen;
} ux;

#define ST_FLAT 0u
#define ST_LINE 1u
#define T_RULE ux.dim
#define T_BG ux.bg
#define T_SURF ux.surf
#define T_TEXT ux.text
#define T_THEME ux.theme
#define T_ACCENT ux.accent
#define T_MID ux.mid
#define T_DIM ux.dim
#define T_LINE ux.line
#define T_SEL ux.sel
#define T_TINT ux.tint
#define T_INK ux.ink
#define T_REC ux.rec
#define T_RAISE ux.raise
#define T_KEY ux.key
#define T_LANE ux.lane
#define T_GRID ux.grid
#define NPALETTES UI_NPALETTES

static inline uint16_t ux_gray(uint32_t x5) { return (uint16_t)((x5 << 11) | (x5 << 6) | x5); }

static uint16_t ux_mix(uint16_t a, uint16_t b, int32_t pct)
{
    static const uint8_t SH[3] = {11, 5, 0}, MK[3] = {31, 63, 31};
    uint32_t out = 0, k;
    if (ux.mono) {
        int32_t x = a >> 11, d = ((b >> 11) - x) * pct;
        return ux_gray((uint32_t)(x + (d + (d >= 0 ? 50 : -50)) / 100));
    }
    for (k = 0; k < 3u; k++) {
        int32_t x = (a >> SH[k]) & MK[k], d = (((b >> SH[k]) & MK[k]) - x) * pct;
        out |= (uint32_t)(x + (d + (d >= 0 ? 50 : -50)) / 100) << SH[k];
    }
    return (uint16_t)out;
}
static uint32_t ux_luma(uint16_t c)
{
    return ((c >> 11) * 255u / 31u * 54u + ((c >> 5) & 63u) * 255u / 63u * 183u + (c & 31u) * 255u / 31u * 19u) >> 8;
}

static ui_pal_t ux_cust = {"CUSTOM", 0x0841, 0xCE59, 0xEF5D, 0xFFDF, 0xFD20};

static void palette_set(uint32_t i)
{
    int cust = i == NPALETTES;
    const ui_pal_t *p = cust ? &ux_cust : &UI_PALETTES[i % NPALETTES];
    i %= NPALETTES;
    ux.mono = !cust && i == UI_GREY_INDEX;
    ux.bg = p->bg; ux.surf = ux.style ? p->bg : p->surf; ux.text = p->text; ux.theme = p->theme; ux.accent = p->accent;
    ux.mid = ux_mix(p->bg, p->text, UI_MID_PCT);
    ux.dim = ux_mix(p->bg, p->text, UI_DIM_PCT);
    ux.line = ux_mix(p->bg, p->text, UI_LINE_PCT);
    ux.sel = ux_mix(p->bg, p->theme, UI_SEL_PCT);
    ux.tint = ux_mix(p->bg, p->theme, UI_TINT_PCT);
    ux.ink = p->bg;
    ux.raise = ux_mix(p->surf, p->text, UI_RAISE_PCT);
    ux.key = ux_mix(p->bg, p->text, UI_KEY_PCT);
    ux.lane = ux_mix(ux.surf, p->theme, 10);
    ux.grid = ux_mix(ux.surf, p->text, 6);
    ux.light = ux_luma(p->bg) > 128u;
    ux.rec = ux.mono ? p->accent : ux.light ? UI_REC_LIGHT : UI_REC_DARK;
    if (!cust && i == UI_BW_INDEX) {
        ux.mid = ux.sel = ux.key = ux.rec = p->text;
        ux.dim = ux.line = ux.raise = ux.lane = UI_BW_GREY;
        ux.tint = ux.grid = p->bg;
    }
    ux.gen++;
}

static uint32_t palette_from_stored(uint32_t v)
{
    if (v >= UI_PAL_TAG && v - UI_PAL_TAG < NPALETTES) return v - UI_PAL_TAG;
    return v < 20u ? UI_PALETTE_MIGRATE[v] : UI_GREY_INDEX;
}
static uint32_t palette_to_stored(uint32_t i) { return UI_PAL_TAG + i % NPALETTES; }

static inline uint16_t swap16(uint32_t c) { return (uint16_t)(((c >> 8) & 0xFFu) | ((c & 0xFFu) << 8)); }

static void cv_begin(uint32_t w, uint32_t h, uint16_t bg)
{
    uint32_t i, n;
    uint16_t s = swap16(bg);
    if (w * h > CV_MAX)
        h = CV_MAX / w;
    lcd_sync();
    GFX_HOOK_BEGIN();
    cv_w = w;
    cv_h = h;
    cv_cy0 = 0;
    cv_cy1 = (int16_t)h;
    cv_bg = bg;
    n = w * h;
    for (i = 0; i < n; i++)
        cv_px[i] = s;
}

static void cv_blit(uint32_t x, uint32_t y)
{
    GFX_HOOK_BLIT(x, y, 0u);
    lcd_blit(x, y, cv_w, cv_h, cv_px);
}

static inline void cv_pset(int32_t x, int32_t y, uint16_t c)
{
    y += cv_oy;
    if ((uint32_t)x < cv_w && (uint32_t)y < cv_h)
        cv_px[(uint32_t)y * cv_w + (uint32_t)x] = swap16(c);
}

static void cv_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    int32_t x1 = x + w, y1 = y + h + cv_oy, i;
    uint16_t sc = swap16(c);
    y += cv_oy;
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if (x1 > (int32_t)cv_w)
        x1 = (int32_t)cv_w;
    if (y1 > (int32_t)cv_h)
        y1 = (int32_t)cv_h;
    for (; y < y1; y++)
        for (i = x; i < x1; i++)
            cv_px[(uint32_t)y * cv_w + (uint32_t)i] = sc;
}

static void cv_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)
{
    int32_t dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int32_t dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int32_t err = dx + dy, guard = 2000;
    while (guard--) {
        int32_t e2 = 2 * err;
        cv_pset(x0, y0, c);
        if (x0 == x1 && y0 == y1)
            break;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void cv_line_t(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c, int32_t thick)
{
    int32_t t;
    for (t = 0; t < thick; t++)
        cv_line(x0, y0 + t, x1, y1 + t, c);
}

static uint32_t rr_cov(int32_t r, int32_t i, int32_t j)
{
    int32_t sx, sy, n = 0;
    for (sy = 0; sy < 4; sy++)
        for (sx = 0; sx < 4; sx++) {
            int32_t dx = r * 8 - ((i * 4 + sx) * 2 + 1), dy = r * 8 - ((j * 4 + sy) * 2 + 1);
            n += dx * dx + dy * dy <= r * r * 64;
        }
    return (uint32_t)n;
}

static void cv_rrect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint16_t c, uint16_t under)
{
    int32_t i, j;
    GFX_HOOK_CELL(x, y + cv_oy, x + w, y + h + cv_oy);
    if (r < 0) r = -r;
    else if (ux.style == ST_LINE) r = 0;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (r < 1) {
        cv_rect(x, y, w, h, c);
        return;
    }
    cv_rect(x + r, y, w - 2 * r, h, c);
    cv_rect(x, y + r, r, h - 2 * r, c);
    cv_rect(x + w - r, y + r, r, h - 2 * r, c);
    for (j = 0; j < r; j++)
        for (i = 0; i < r; i++) {
            uint32_t a = rr_cov(r, i, j);
            uint16_t px = a >= 16u ? c : a == 0u ? under : ux_mix(under, c, (int32_t)(a * 100u / 16u));
            cv_pset(x + i, y + j, px);
            cv_pset(x + w - 1 - i, y + j, px);
            cv_pset(x + i, y + h - 1 - j, px);
            cv_pset(x + w - 1 - i, y + h - 1 - j, px);
        }
}

static const uint8_t CURVE_DARK[16] = {0, 24, 43, 62, 80, 97, 114, 130, 147, 163, 178, 194, 210, 225, 240, 255};
static const uint8_t CURVE_LIGHT[16] = {0, 12, 27, 42, 58, 75, 91, 109, 126, 144, 162, 180, 199, 217, 236, 255};
#define NRAMP 6u
static struct {
    uint16_t fg[NRAMP], bg[NRAMP], v[NRAMP][16];
    uint32_t gen;
    uint8_t next, n;
} rc;

static const uint16_t *ramp(uint16_t fg, uint16_t bg)
{
    static const uint8_t SH[3] = {11, 5, 0}, MK[3] = {31, 63, 31};
    const uint8_t *cv = ux.light ? CURVE_LIGHT : CURVE_DARK;
    uint32_t i, a, k;
    uint16_t *v;
    if (fg == bg)
        fg = ux.bg;
    GFX_HOOK_INK(fg, bg);
    if (rc.gen != ux.gen) {
        rc.gen = ux.gen;
        rc.n = rc.next = 0;
    }
    for (i = 0; i < rc.n; i++)
        if (rc.fg[i] == fg && rc.bg[i] == bg)
            return rc.v[i];
    i = rc.next;
    rc.next = (uint8_t)((i + 1u) % NRAMP);
    if (rc.n < NRAMP)
        rc.n++;
    rc.fg[i] = fg;
    rc.bg[i] = bg;
    v = rc.v[i];
    for (a = 0; a < 16u; a++) {
        uint32_t out = 0;
        int32_t w = cv[a];
        if (ux.mono) {
            int32_t x = bg >> 11;
            out = ux_gray((uint32_t)(x + ((((fg >> 11) - x) * w + 128) >> 8)));
        } else {
            for (k = 0; k < 3u; k++) {
                int32_t x = (bg >> SH[k]) & MK[k], y = (fg >> SH[k]) & MK[k];
                out |= (uint32_t)(x + (((y - x) * w + 128) >> 8)) << SH[k];
            }
        }
        v[a] = swap16(out);
    }
    return v;
}

static void cv_alpha(int32_t x, int32_t y, uint32_t w, uint32_t h, const uint8_t *d, const uint16_t *rv)
{
    uint32_t k = 0, gx, gy;
    y += cv_oy;
    for (gy = 0; gy < h; gy++) {
        int32_t py = y + (int32_t)gy;
        uint16_t *row;
        if (py < cv_cy0 || py >= cv_cy1) {
            k += w;
            continue;
        }
        row = cv_px + (uint32_t)py * cv_w;
        for (gx = 0; gx < w; gx++, k++) {
            uint32_t v = (d[k >> 1] >> ((k & 1u) ? 0 : 4)) & 15u;
            int32_t px = x + (int32_t)gx;
            if (v && (uint32_t)px < cv_w)
                row[px] = rv[v];
        }
    }
    GFX_HOOK_PIXELS(w * h);
}

#define HC_FONTS 3
static struct { const uint8_t *hc; uint16_t lut[256]; } hc_tab[HC_FONTS] __attribute__((section(".pool")));
static const uint16_t *hc_lut(const uint8_t *hc)
{
    uint32_t k, len, code = 0, sym = 15, i;
    for (k = 0; k < HC_FONTS - 1u && hc_tab[k].hc && hc_tab[k].hc != hc; k++)
        ;
    if (hc_tab[k].hc != hc) {
        hc_tab[k].hc = hc;
        for (i = 0; i < 256u; i++)
            hc_tab[k].lut[i] = 0;
        for (len = 1; len <= 8u; len++, code <<= 1)
            for (i = 0; i < hc[len - 1u]; i++, code++, sym++) {
                uint32_t a = code << (8u - len), n = 1u << (8u - len);
                while (n--)
                    hc_tab[k].lut[a + n] = (uint16_t)(hc[sym] | len << 8);
            }
    }
    return hc_tab[k].lut;
}

static void cv_alpha_hc(int32_t x, int32_t y, uint32_t w, uint32_t h, const uint8_t *d, const uint8_t *hc,
                        const uint16_t *rv)
{
    const uint16_t *lut = hc_lut(hc);
    uint32_t gy, v = 0, run = 0, acc = 0, nb = 0;
    y += cv_oy;
    for (gy = 0; gy < h; gy++) {
        int32_t py = y + (int32_t)gy;
        uint16_t *row = py < cv_cy0 || py >= cv_cy1 ? 0 : cv_px + (uint32_t)py * cv_w;
        uint32_t gx = 0;
        while (gx < w) {
            uint32_t n;
            if (!run) {
                uint32_t e, s;
                while (nb < 16u) {
                    acc = acc << 8 | *d++;
                    nb += 8;
                }
                e = lut[(acc >> (nb - 8u)) & 255u];
                if (e) {
                    nb -= e >> 8;
                    s = e & 255u;
                } else {
                    uint32_t code = 0, first = 0, idx = 0, len = 0;
                    for (;;) {
                        code |= (acc >> --nb) & 1u;
                        if (code - first < hc[len])
                            break;
                        idx += hc[len];
                        first = (first + hc[len]) << 1;
                        code <<= 1;
                        len++;
                    }
                    s = hc[15u + idx + code - first];
                }
                v = s & 15u;
                run = (s >> 4) + 1u;
            }
            n = run < w - gx ? run : w - gx;
            if (v && row) {
                int32_t px = cv_flip ? x + (int32_t)(w - 1u - gx) : x + (int32_t)gx, dx = cv_flip ? -1 : 1;
                uint32_t k;
                for (k = 0; k < n; k++, px += dx)
                    if ((uint32_t)px < cv_w)
                        row[px] = rv[v];
            }
            gx += n;
            run -= n;
        }
    }
    GFX_HOOK_PIXELS(w * h);
}

static int32_t glyph_at(const aafont_t *f, uint32_t ch)
{
    uint32_t i;
    if (ch >= f->first && ch <= f->last)
        return (int32_t)(ch - f->first);
    for (i = 0; i < f->nex; i++)
        if (f->ex[i] == ch)
            return (int32_t)(f->last - f->first + 1u + i);
    return -1;
}

static uint32_t glyph(const aafont_t *f, uint32_t ch)
{
    int32_t k;
    if (ch >= 'a' && ch <= 'z' && f->last < 'a')
        ch -= 32u;
    if ((k = glyph_at(f, ch)) < 0 && (ch != (uint8_t)ELLIPSIS || (k = glyph_at(f, '.')) < 0) &&
        (k = glyph_at(f, '?')) < 0)
        k = 0;
    return (uint32_t)k << f->psh;
}

static int32_t kern(const aafont_t *f, uint32_t a, uint32_t b)
{
    uint32_t key = (a << 8) | b, lo = 0, hi = f->nk;
    while (lo < hi) {
        uint32_t m = (lo + hi) / 2u;
        if (f->kkey[m] == key)
            return f->kd[m];
        if (f->kkey[m] < key)
            lo = m + 1u;
        else
            hi = m;
    }
    return 0;
}

static uint32_t fold(const aafont_t *f, uint32_t c) { return c >= 'a' && c <= 'z' && f->last < 'a' ? c - 32u : c; }

static int32_t text_w(const aafont_t *f, const char *s)
{
    int32_t pen = 0;
    uint32_t prev = 0;
    for (; *s; s++) {
        uint32_t c = fold(f, (uint8_t)*s);
        if (prev)
            pen += kern(f, prev, c);
        pen += f->g[glyph(f, c)].adv;
        prev = c;
    }
    return (pen + 8) >> 4;
}

static int32_t text_run(int32_t x, int32_t y, const aafont_t *f, const char *s, const uint16_t *rv, int32_t *bb)
{
    int32_t pen = 0, err = 0, x0 = 0x7FFF, y0 = 0x7FFF, x1 = -0x7FFF, y1 = -0x7FFF;
    const int32_t step = 16 >> f->psh;
    uint32_t prev = 0;
    for (; *s; s++) {
        uint32_t c = fold(f, (uint8_t)*s);
        const aag_t *g;
        int32_t pos, d0, d1;
        if (prev)
            pen += kern(f, prev, c);
        g = &f->g[glyph(f, c)];
        if (f->psh && g[1].off != g->off) {
            pos = pen - (pen & (step - 1));
            d0 = pos - pen - err;
            d1 = d0 + step;
            if ((d0 < 0 ? -d0 : d0) > (d1 < 0 ? -d1 : d1))
                pos += step;
            g += ((uint32_t)pos >> (4u - f->psh)) & ((1u << f->psh) - 1u);
        } else {
            pos = (pen + 8) & ~15;
        }
        err = pos - pen;
        if (g->bw) {
            int32_t gx = x + (pos >> 4) + g->bx, gy = y + g->by;
            if (!rv)
                ;
            else if (f->hc)
                cv_alpha_hc(gx, gy, g->bw, g->bh, f->data + g->off, f->hc, rv);
            else
                cv_alpha(gx, gy, g->bw, g->bh, f->data + g->off, rv);
            if (gx < x0) x0 = gx;
            if (gx + g->bw > x1) x1 = gx + g->bw;
            if (GFX_HOOKS && gy < y0) y0 = gy;
            if (GFX_HOOKS && gy + g->bh > y1) y1 = gy + g->bh;
        }
        pen += g->adv;
        prev = c;
    }
    bb[0] = x0;
    bb[2] = x1;
    if (GFX_HOOKS) {
        bb[1] = y0;
        bb[3] = y1;
    }
    return pen;
}

static int32_t cv_text_flags(int32_t x, int32_t y, const aafont_t *f, const char *s, uint16_t fg, uint16_t bg,
                             uint32_t flags)
{
    int32_t b[4], pen = text_run(x, y, f, s, ramp(fg, bg), b);
    if (GFX_HOOKS && b[2] > b[0]) {
        int32_t y0 = b[1], y1 = b[3];
        if (b[0] < 0 || b[2] > (int32_t)cv_w || y0 + cv_oy < cv_cy0 || y1 + cv_oy > cv_cy1)
            flags |= 2u;
        if (cv_scroll) {
            flags |= 32u;
            if (y0 + cv_oy < cv_cy0) y0 = cv_cy0 - cv_oy;
            if (y1 + cv_oy > cv_cy1) y1 = cv_cy1 - cv_oy;
        }
        GFX_HOOK_PEN(y, f);
        GFX_HOOK_TEXT(b[0], y0 + cv_oy, b[2], y1 + cv_oy, s, flags);
    }
    (void)flags;
    return x + ((pen + 8) >> 4);
}
static int32_t cv_text_on(int32_t x, int32_t y, const aafont_t *f, const char *s, uint16_t fg, uint16_t bg)
{
    return cv_text_flags(x, y, f, s, fg, bg, 0);
}

static int32_t cv_text(int32_t x, int32_t y, const aafont_t *f, const char *s, uint16_t fg)
{
    return cv_text_flags(x, y, f, s, fg, cv_bg, 0);
}

static int32_t cv_text_r(int32_t xr, int32_t y, const aafont_t *f, const char *s, uint16_t fg, uint16_t bg)
{
    int32_t w = text_w(f, s);
    cv_text_on(xr - w, y, f, s, fg, bg);
    return xr - w;
}

static void text_ink(const aafont_t *f, const char *s, int32_t *b) { text_run(0, 0, f, s, 0, b); }
#define HALF_UP(v) ((v) >> 1)

#define CAP_IN(F, h) (HALF_UP((h) - AF_##F##_CAP_H) - AF_##F##_CAP_Y)

static int32_t ink_in(const aafont_t *f, const char *s, int32_t w)
{
    int32_t b[4];
    text_ink(f, s, b);
    return HALF_UP(w - (b[2] - b[0])) - b[0];
}

static int32_t cv_text_in(int32_t x0, int32_t y, int32_t w, const aafont_t *f, const char *s, uint16_t fg, uint16_t bg)
{
    return cv_text_on(x0 + ink_in(f, s, w), y, f, s, fg, bg);
}

static int text_fit(char *d, uint32_t n, const char *s, const aafont_t *f, int32_t maxw)
{
    uint32_t k = 0;
    while (s[k] && k + 1u < n) {
        d[k] = s[k];
        k++;
    }
    d[k] = 0;
    if (!s[k] && text_w(f, d) <= maxw)
        return 0;
    if (k + 2u > n)
        k = n - 2u;
    for (;;) {
        while (k && d[k - 1u] == ' ')
            k--;
        d[k] = ELLIPSIS;
        d[k + 1u] = 0;
        if (!k || text_w(f, d) <= maxw)
            return 1;
        k--;
    }
}
static int32_t cv_free_text(int32_t x, int32_t y, const aafont_t *f, const char *s, uint16_t fg, uint16_t bg,
                            int32_t maxw)
{
    char b[48];
    int cut = text_fit(b, sizeof b, s, f, maxw);
    return cv_text_flags(x, y, f, b, fg, bg, cut ? 9u : 8u);
}

#define NKRAMP 4u
static struct {
    uint16_t key[NKRAMP][3], v[NKRAMP][16];
    uint32_t gen;
    uint8_t next, n;
} kr __attribute__((section(".pool")));
static const uint16_t *kc_ramp(uint16_t under, uint16_t fill, uint16_t ink)
{
    uint32_t i, a;
    uint16_t *v;
    if (ink == fill)
        ink = ux.bg;
    GFX_HOOK_INK(ink, fill);
    if (kr.gen != ux.gen) {
        kr.gen = ux.gen;
        kr.n = kr.next = 0;
    }
    for (i = 0; i < kr.n; i++)
        if (kr.key[i][0] == under && kr.key[i][1] == fill && kr.key[i][2] == ink)
            return kr.v[i];
    i = kr.next;
    kr.next = (uint8_t)((i + 1u) % NKRAMP);
    if (kr.n < NKRAMP)
        kr.n++;
    kr.key[i][0] = under;
    kr.key[i][1] = fill;
    kr.key[i][2] = ink;
    v = kr.v[i];
    v[0] = swap16(under);
    for (a = 1; a < 16u; a++) {
        int32_t p = a < 5u ? (int32_t)a * 20 : (int32_t)(a - 5u) * 10;
        v[a] = swap16(a < 5u ? ux_mix(under, fill, p) : ux_mix(fill, ink, p));
    }
    return v;
}
static int32_t kc_w(uint32_t id) { return id < KC_COUNT ? KC[id].w : 0; }

static int32_t cv_keycap(int32_t x, int32_t y, uint32_t id, uint16_t fill, uint16_t ink, uint16_t under)
{
    const kc_t *k = &KC[id < KC_COUNT ? id : 0u];
    const uint16_t *rv = kc_ramp(under, fill, ink);
    cv_alpha(x, y, k->w, KC_H, KC_DATA + k->off, rv);
    if (x < 0 || x + k->w > (int32_t)cv_w || y + cv_oy < 0 || y + KC_H + cv_oy > (int32_t)cv_h)
        GFX_HOOK_TEXT(x, y + cv_oy, x + k->w, y + KC_H + cv_oy, k->label, 6u);
    else
        GFX_HOOK_TEXT(x, y + cv_oy, x + k->w, y + KC_H + cv_oy, k->label, 4u);
    return x + k->w;
}

#define KH_GAP 4

static int32_t kh_ink(uint32_t id, const char *act)
{
    int32_t b[4];
    if (!act || !act[0])
        return kc_w(id);
    text_ink(&AF_S, act, b);
    return kc_w(id) + KH_GAP + b[2];
}
static int32_t cv_key_hint(int32_t x, int32_t y, uint32_t id, const char *act, int on, uint16_t under)
{
    x = cv_keycap(x, y, id, on ? T_KEY : T_DIM, T_INK, under);
    if (act && act[0]) {
        GFX_HOOK_ALIGN(0, y, 0, y + KC_H, AL_V | AL_PASS, "key hint word on its keycap's line");
        x = cv_text_on(x + KH_GAP, y - 1, &AF_S, act, on ? T_TEXT : T_DIM, under);
    }
    return x;
}

typedef struct { uint8_t key; const char *act; } khint_t;

static int32_t kc_tag(const char *s, uint32_t *len)
{
    uint32_t i, n;
    if (s[0] != '[')
        return -1;
    for (n = 1; s[n] && s[n] != ']'; n++)
        if (n > 8u)
            return -1;
    if (s[n] != ']')
        return -1;
    for (i = 0; i < KC_COUNT; i++) {
        const char *l = KC[i].label;
        uint32_t k = 0;
        while (k + 1u < n && l[k] == s[k + 1u])
            k++;
        if (k + 1u == n && !l[k]) {
            *len = n + 1u;
            return (int32_t)i;
        }
    }
    return -1;
}

static int32_t cv_free_hint(int32_t x, int32_t y, const char *s, uint16_t fg, uint16_t bg, int32_t maxw)
{
    uint32_t n, i;
    int32_t id = kc_tag(s, &n), x0 = x;
    for (i = 0; id < 0 && i < 7u && s[i] && s[i] != ' '; i++)
        ;
    if (id < 0 && i && s[i] == ' ' && (id = kc_tag(s + i + 1u, &n)) >= 0) {
        char w[8] = {0};
        uint32_t j;
        for (j = 0; j < i; j++)
            w[j] = s[j];
        GFX_HOOK_ALIGN(0, y + 1, 0, y + 1 + KC_H, AL_V | AL_PASS, "free hint words on the keycap's line");
        x = cv_text_on(x, y, &AF_S, w, fg, bg) + KH_GAP;
        s += i + 1u;
    }
    if (id >= 0 && kc_w((uint32_t)id) + KH_GAP < maxw - (x - x0)) {
        x = cv_keycap(x, y + 1, (uint32_t)id, T_KEY, T_INK, bg) + KH_GAP;
        s += n;
        while (*s == ' ')
            s++;
        if (!*s)
            return x - KH_GAP;
        GFX_HOOK_ALIGN(0, y + 1, 0, y + 1 + KC_H, AL_V | AL_PASS, "free hint words on the keycap's line");
    }
    return cv_free_text(x, y, &AF_S, s, fg, bg, maxw - (x - x0));
}

static void draw_text_line(uint32_t x, uint32_t y, uint32_t w, const aafont_t *f, const char *s,
                           uint16_t c, uint16_t bg, int align)
{
    int32_t tw = text_w(f, s), tx = 0;
    cv_begin(w, f->h, bg);
    if (align == 1) {
        tx = ink_in(f, s, (int32_t)w);
        GFX_HOOK_ALIGN(0, 0, (int32_t)w, 0, AL_H, "one-shot line centred");
    }
    else if (align == 2)
        tx = (int32_t)w - tw;
    cv_text(tx, 0, f, s, c);
    cv_blit(x, y);
    lcd_sync();
}
static void draw_text_box(uint32_t x, uint32_t y, uint32_t w, const aafont_t *f, const char *s,
                          uint16_t c, int align)
{
    draw_text_line(x, y, w, f, s, c, T_BG, align);
}

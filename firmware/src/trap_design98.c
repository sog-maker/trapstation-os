/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION contributors */

#define DZ_GOLD GB_RGB(212, 170, 70)
#define DZ_DGOLD GB_RGB(146, 108, 38)
#define DZ_PANEL GB_RGB(42, 42, 40)
#define DZ_SCREEN GB_RGB(22, 22, 20)
#define DZ_GRID GB_RGB(54, 54, 50)
#define DZ_CREAM GB_RGB(240, 232, 214)
#define DZ_KNOB GB_RGB(224, 212, 184)
#define DZ_OX GB_RGB(132, 28, 26)
#define DZ_LED GB_RGB(236, 46, 34)
#define DZ_INK GB_RGB(22, 22, 20)
#define DZ_Y (TL_D0 + 1)
#define DZ_H (TL_D1 - TL_D0 - 2)
#define DZ_KX(k) (36 + (int32_t)(k) * 56)

static void dz_panel(int32_t x, int32_t y, int32_t w, int32_t h, const char *tab, uint16_t bgc)
{
    cv_rrect(x, y, w, h, 7, DZ_DGOLD, bgc);
    cv_rrect(x + 2, y + 2, w - 4, h - 4, 6, DZ_GOLD, DZ_DGOLD);
    cv_rrect(x + 4, y + 4, w - 8, h - 8, 5, DZ_PANEL, DZ_GOLD);
    fxg_disc(x + 9, y + 9, 2, DZ_DGOLD);
    fxg_disc(x + w - 10, y + 9, 2, DZ_DGOLD);
    fxg_disc(x + 9, y + h - 10, 2, DZ_DGOLD);
    fxg_disc(x + w - 10, y + h - 10, 2, DZ_LED);
    if (tab) {
        int32_t tw = text_w(&AF_S, tab) + 22, tx = x + (w - tw) / 2;
        cv_rrect(tx, y + 4, tw, 15, 4, DZ_SCREEN, DZ_PANEL);
        fxg_disc(tx + 6, y + 11, 2, DZ_LED);
        fxg_disc(tx + tw - 7, y + 11, 2, DZ_LED);
        cv_text_in(tx, y + 4, tw, &AF_S, tab, DZ_CREAM, DZ_SCREEN);
    }
}
static void dz_screen(int32_t x, int32_t y, int32_t w, int32_t h)
{
    int32_t i;
    cv_rect(x - 1, y - 1, w + 2, h + 2, DZ_DGOLD);
    cv_rect(x, y, w, h, DZ_SCREEN);
    for (i = x + 10; i < x + w; i += 12)
        cv_rect(i, y, 1, h, DZ_GRID);
    for (i = y + 8; i < y + h; i += 10)
        cv_rect(x, i, w, 1, DZ_GRID);
}
static int32_t dz_ang(int32_t ratio) { return (-135 + (ratio < 0 ? 0 : ratio > 1000 ? 1000 : ratio) * 270 / 1000 + 360) % 360; }
static void dz_knob(int32_t cx, int32_t cy, int32_t r, int32_t ratio, int hot)
{
    int32_t a, px, py, on = dz_ang(ratio);
    for (a = -135; a <= 135; a += 15) {
        int32_t d = (a + 360) % 360, lit = ((a + 135) * 1000 / 270) <= ratio;
        dial_pt(cx, cy, r + 4, d, &px, &py);
        cv_rect(px - 1, py - 1, 2, 2, lit ? (hot ? DZ_LED : DZ_GOLD) : DZ_GRID);
    }
    fxg_disc(cx, cy, r, DZ_DGOLD);
    fxg_disc(cx, cy, r - 1, DZ_CREAM);
    fxg_disc(cx, cy, r - 3, DZ_KNOB);
    dial_pt(cx, cy, r - 2, on, &px, &py);
    cv_line_t(cx, cy, px, py, DZ_INK, 2);
}
static void dz_dead(int32_t cx, int32_t cy, int32_t r)
{
    fxg_disc(cx, cy, r, DZ_GRID);
    fxg_disc(cx, cy, r - 2, DZ_SCREEN);
    cv_rect(cx - 4, cy, 9, 2, DZ_GRID);
}
static void dz_box(int32_t x, int32_t y, int32_t w, int32_t h, const char *s, int gold)
{
    uint16_t f = gold ? DZ_GOLD : DZ_CREAM;
    cv_rrect(x, y, w, h, 3, DZ_DGOLD, DZ_PANEL);
    cv_rrect(x + 1, y + 1, w - 2, h - 2, 3, f, DZ_DGOLD);
    cv_text_in(x, y + (h - 14) / 2, w, text_w(&AF_M, s) <= w - 4 && h >= 18 ? &AF_M : &AF_S, s, DZ_INK, f);
}
static void dz_tag(int32_t x, int32_t y, int32_t w, const char *s)
{
    cv_rrect(x, y, w, 18, 3, DZ_GOLD, DZ_PANEL);
    cv_rrect(x + 1, y + 1, w - 2, 16, 3, DZ_SCREEN, DZ_GOLD);
    cv_text_in(x, y + 2, w, text_w(&AF_M, s) <= w - 4 ? &AF_M : &AF_S, s, DZ_GOLD, DZ_SCREEN);
}
static void dz_hline(int32_t x0, int32_t x1, int32_t y, uint16_t c, int32_t t) { cv_line_t(x0, y, x1, y, c, t); }
static void dz_arrow(int32_t x0, int32_t y, int32_t x1, uint16_t c)
{
    cv_line_t(x0, y, x1, y, c, 2);
    cv_line_t(x1, y, x1 - 4, y - 4, c, 2);
    cv_line_t(x1, y, x1 - 4, y + 4, c, 2);
}
static int32_t dz_wave(uint32_t w, uint32_t ph)
{
    if (w == 2u) return (int32_t)(ph & 1023u) / 2 - 256;
    if (w == 3u) return 256 - (int32_t)(ph & 1023u) / 2;
    if (w == 4u) return ph & 512u ? -256 : 256;
    if (w == 5u) return (int32_t)((((ph >> 8) * 2654435761u) >> 23) & 511u) - 256;
    if (w == 1u) {
        int32_t t = (int32_t)((ph + 256u) & 1023u);
        return t < 512 ? t - 256 : 768 - t;
    }
    return tl_sin(ph);
}

static void dz_curve(int32_t x0, int32_t x1, int32_t ty, int32_t xc, int32_t r, int32_t y0, int32_t yb, uint16_t line, uint16_t fill)
{
    int32_t x, py = yb, H = (yb - y0) * 3 / 5, sl = (x1 - x0) / 5;
    for (x = x0; x <= x1; x += 2) {
        int32_t d = x - xc, h, b = d < 0 ? -d : d, y;
        h = ty == 0 ? (d <= 0 ? H : H - d * H / sl) : ty == 2 ? (d >= 0 ? H : H + d * H / sl) : H - b * H / (sl + 6);
        if (b < 14)
            h += r * ((yb - y0) - H) / 1000 * (14 - b) * (14 - b) / 196;
        h = h < 0 ? 0 : h > yb - y0 ? yb - y0 : h;
        y = yb - h;
        if (h > 1)
            cv_rect(x, y + 1, 2, yb - y - 1, fill);
        if (x > x0)
            cv_line_t(x - 2, py, x, y, line, 2);
        py = y;
    }
}
static void dz_knobrow(const gb_view_t *v, int32_t cy, int32_t r)
{
    uint32_t k;
    for (k = 0; k < 4u; k++)
        if (v->mask & (1u << k))
            dz_knob(DZ_KX(k), cy, r, v->ratio[k], v->hot == k + 1u);
        else
            dz_dead(DZ_KX(k), cy, r);
}

static void dz_fm_osc1(const gb_view_t *v, uint16_t bgc)
{
    static const uint8_t E[8] = {1 | 8, 1 | 4, 1 | 4 | 8, 1 | 2 | 8, 1 | 2 | 4 | 8, 16 | 1, 16 | 1 | 2, 16 | 1 | 8};
    uint32_t a = (uint32_t)v->cur[0] & 7u, e = E[a];
    int32_t ax = 102, bx = 160, yb = DZ_Y + 54, yc = DZ_Y + 18, xc = ax, xd = bx, yd = yc, sx = 145, sy = DZ_Y + 86;
    char ab[8] = {'A', 'L', 'G', 0}, nb[6];
    dz_panel(12, DZ_Y, 216, DZ_H, 0, bgc);
    if (e & 16u) {
        xc = ax;
        yc = DZ_Y + 30;
        xd = ax;
        yd = DZ_Y + 8;
        xc = ax + 18;
        xd = ax + 18;
    }
    cv_rrect(24, DZ_Y + 30, 48, 36, 4, DZ_GOLD, DZ_PANEL);
    cv_rrect(25, DZ_Y + 31, 46, 34, 4, DZ_SCREEN, DZ_GOLD);
    cv_text_in(24, DZ_Y + 33, 48, &AF_S, ab, DZ_CREAM, DZ_SCREEN);
    fmt_int(nb, (int32_t)a + 1);
    str_cpy(nb + str_len(nb), "/8", sizeof nb - str_len(nb));
    cv_text_in(24, DZ_Y + 48, 48, &AF_S, nb, DZ_GOLD, DZ_SCREEN);
    fxg_disc(20, DZ_Y + 48, 2, DZ_LED);
#define DZ_OUT(x0, y0, x1, y1) cv_line_t((x0) + 14, (y0) + 22, (x1) + 14, (y1), DZ_CREAM, 2)
    if (e & 1u) DZ_OUT(xc, yc, ax, yb);
    if (e & 2u) DZ_OUT(xc, yc, bx, yb);
    if (e & 4u) DZ_OUT(xd, yd, ax, yb);
    if (e & 8u) DZ_OUT(xd, yd, bx, yb);
    if (e & 16u) DZ_OUT(xd, yd, xc, yc);
#undef DZ_OUT
    cv_line_t(ax + 14, yb + 22, ax + 14, sy, DZ_CREAM, 2);
    cv_line_t(ax + 14, sy, sx - 8, sy, DZ_CREAM, 2);
    cv_line_t(bx + 14, yb + 22, bx + 14, sy, DZ_CREAM, 2);
    cv_line_t(bx + 14, sy, sx + 8, sy, DZ_CREAM, 2);
    fxg_disc(sx, sy, 8, DZ_CREAM);
    fxg_disc(sx, sy, 6, DZ_PANEL);
    cv_rect(sx - 4, sy, 9, 1, DZ_CREAM);
    cv_rect(sx, sy - 4, 1, 9, DZ_CREAM);
    dz_box(ax, yb, 28, 22, "A", 0);
    dz_box(bx, yb, 28, 22, "B", 0);
    dz_box(xc, yc, 28, 22, "C", 1);
    dz_box(xd + (e & 16u ? 0 : 0), yd, 28, 22, "D", 1);
    cv_rect(xd + 28, yd + 10, 7, 2, DZ_GOLD);
    cv_rect(xd + 33, yd - 3, 2, 15, DZ_GOLD);
    cv_rect(xd + 18, yd - 3, 17, 2, DZ_GOLD);
    if (v->mask & 4u)
        cv_text_on(xc - 22, yc + 4, &AF_S, v->val[2], DZ_GOLD, DZ_PANEL);
    if ((v->mask & 8u) && !(e & 16u))
        cv_text_on(xd + 38, yd + 4, &AF_S, v->val[3], DZ_GOLD, DZ_PANEL);
}
static void dz_env_plot(int32_t x, int32_t y, int32_t w, int32_t h, int32_t atk, int32_t dec, int hold, int32_t amt)
{
    int32_t yb = y + h - 4, top = yb - 6 - amt * (h - 16) / 1000, x1 = x + 4 + atk * (w / 4) / 1000, x2;
    x2 = hold ? x + w - 6 : x1 + 8 + dec * (w * 2 / 3) / 1000;
    x2 = x2 > x + w - 6 ? x + w - 6 : x2;
    dz_screen(x, y, w, h);
    cv_line_t(x + 4, yb, x1, top, DZ_GOLD, 2);
    cv_line_t(x1, top, x2, hold ? top : yb, DZ_GOLD, 2);
    fxg_disc(x1, top, 3, DZ_GOLD);
    fxg_disc(x2, hold ? top : yb, 3, DZ_GOLD);
    if (hold)
        cv_text_in(x, y + h / 2 - 2, w, &AF_S, "HOLD", DZ_CREAM, DZ_SCREEN);
}
static void dz_op(const gb_view_t *v, uint16_t bgc, int d)
{
    int hold = v->val[1][0] == 'H';
    char nb[12];
    dz_panel(12, DZ_Y, 216, DZ_H, d ? "OP D" : "OP C", bgc);
    dz_env_plot(22, DZ_Y + 24, 92, 70, v->ratio[0], v->ratio[1], hold, 800);
    cv_text_in(118, DZ_Y + 24, 50, &AF_S, "AMT", DZ_CREAM, DZ_PANEL);
    dz_knob(143, DZ_Y + 62, 16, v->ratio[2], v->hot == 3u);
    cv_rect(172, DZ_Y + 26, 1, 66, DZ_GRID);
    if (d) {
        cv_text_in(174, DZ_Y + 24, 46, &AF_S, "FB", DZ_CREAM, DZ_PANEL);
        dz_box(186, DZ_Y + 42, 22, 18, "D", 1);
        cv_rect(208, DZ_Y + 50, 8, 2, DZ_GOLD);
        cv_rect(214, DZ_Y + 38, 2, 14, DZ_GOLD);
        cv_rect(180, DZ_Y + 38, 36, 2, DZ_GOLD);
        cv_rect(180, DZ_Y + 38, 2, 14, DZ_GOLD);
        cv_rect(180, DZ_Y + 50, 6, 2, DZ_GOLD);
    } else {
        cv_text_in(174, DZ_Y + 24, 46, &AF_S, "A<>B", DZ_CREAM, DZ_PANEL);
        cv_rect(180, DZ_Y + 82, 34, 3, DZ_GRID);
        cv_rect(180 + v->ratio[3] * 30 / 1000, DZ_Y + 78, 4, 11, DZ_GOLD);
    }
    str_cpy(nb, (v->mask & 8u) ? v->val[3] : "-", sizeof nb);
    dz_tag(176, DZ_Y + (d ? 64 : 50), 42, nb);
}
static void dz_tune(const gb_view_t *v, uint16_t bgc)
{
    int32_t sx = 72, sw = 92, sy = DZ_Y + 18, sh = 66, mx = sx + 18, my = sy + sh / 2, sp = 4 + v->ratio[0] * 24 / 1000;
    dz_panel(12, DZ_Y, 216, DZ_H, 0, bgc);
    cv_text_in(18, DZ_Y + 10, 54, &AF_S, "DTUN", DZ_CREAM, DZ_PANEL);
    dz_knob(45, DZ_Y + 46, 15, v->ratio[0], v->hot == 1u);
    dz_tag(22, DZ_Y + 72, 46, v->val[0]);
    dz_screen(sx, sy, sw, sh);
    {
        int32_t y;
        for (y = sy + 2; y < sy + sh - 2; y += 6)
            cv_rect(sx + sw / 2 + 10, y, 1, 3, DZ_CREAM);
    }
    cv_line_t(mx, my, sx + sw - 16, my - sp, DZ_CREAM, 2);
    cv_line_t(mx, my, sx + sw - 16, my + sp, DZ_CREAM, 2);
    fxg_disc(mx, my, 3, DZ_GOLD);
    dz_box(sx + sw - 16, my - sp - 8, 14, 16, "A", 1);
    dz_box(sx + sw - 16, my + sp - 8, 14, 16, "B", 1);
    cv_text_in(170, DZ_Y + 18, 52, &AF_S, "PITCH", DZ_CREAM, DZ_PANEL);
    dz_tag(172, DZ_Y + 38, 48, (v->mask & 2u) ? v->val[1] : "-");
}
static void dz_adsr(const gb_view_t *v, uint16_t bgc, int filt)
{
    int32_t x0 = 24, y0 = DZ_Y + 22, w = 192, h = 46, yb = y0 + h - 4, yt = y0 + 6, ad = (v->mask & 12u) == 0u, ys, x1, x2, x3, x4, r;
    dz_panel(12, DZ_Y, 216, DZ_H, filt ? "FILTER ENV (ADSR)" : "AMP ENV (ADSR)", bgc);
    dz_screen(x0, y0, w, h);
    x1 = x0 + 4 + v->ratio[0] * 40 / 1000;
    x2 = x1 + 6 + v->ratio[1] * 48 / 1000;
    ys = ad ? yb : yb - (int32_t)v->ratio[2] * (yb - yt) / 1000;
    x4 = ad ? x2 : x0 + w - 4;
    x3 = ad ? x2 : x4 - 6 - v->ratio[3] * 48 / 1000;
    for (r = x0 + 4; r < x4; r++) {
        int32_t y = r < x1 ? yb - (yb - yt) * (r - x0 - 4) / (x1 - x0 - 3) : r < x2 ? yt + (ys - yt) * (r - x1) / (x2 - x1 + 1)
                  : r < x3 ? ys : ys + (yb - ys) * (r - x3) / (x4 - x3 + 1);
        if (yb - y > 0)
            cv_rect(r, y, 1, yb - y, DZ_OX);
    }
    cv_line_t(x0 + 4, yb, x1, yt, DZ_GOLD, 2);
    cv_line_t(x1, yt, x2, ys, DZ_GOLD, 2);
    if (!ad) {
        cv_line_t(x2, ys, x3, ys, DZ_GOLD, 2);
        cv_line_t(x3, ys, x4, yb, DZ_GOLD, 2);
        fxg_disc(x3, ys, 3, DZ_GOLD);
    }
    fxg_disc(x1, yt, 3, DZ_GOLD);
    fxg_disc(x2, ys, 3, DZ_GOLD);
    if (filt) {
        cv_rect(x0 + 6, y0 + 4, 22, 14, GB_RGB(70, 150, 220));
        cv_rect(x0 + 7, y0 + 5, 20, 12, DZ_SCREEN);
        cv_line_t(x0 + 9, y0 + 8, x0 + 19, y0 + 8, GB_RGB(70, 150, 220), 1);
        cv_line_t(x0 + 19, y0 + 8, x0 + 24, y0 + 15, GB_RGB(70, 150, 220), 1);
    }
    cv_text_on((x0 + 4 + x1) / 2 - 3, yb - 14, &AF_S, "A", DZ_CREAM, DZ_SCREEN);
    if (x2 - x1 > 10)
        cv_text_on((x1 + x2) / 2 - 3, yb - 14, &AF_S, "D", DZ_CREAM, DZ_SCREEN);
    if (!ad) {
        cv_text_on((x2 + x3) / 2 - 3, yb - 14, &AF_S, "S", DZ_CREAM, DZ_SCREEN);
        cv_text_on((x3 + x4) / 2 - 3, yb - 14, &AF_S, "R", DZ_CREAM, DZ_SCREEN);
    }
    dz_knobrow(v, DZ_Y + 84, 9);
}
static void dz_envdest(const gb_view_t *v, uint16_t bgc)
{
    int32_t a = v->ratio[1], y = DZ_Y + 36;
    dz_panel(12, DZ_Y, 216, DZ_H, "ENV DEST", bgc);
    dz_box(24, y, 50, 28, "ENV", 0);
    dz_arrow(78, y + 14, 96, DZ_CREAM);
    dz_box(100, y, 60, 28, v->val[0], 1);
    dz_knob(192, y + 12, 17, a, v->hot == 2u);
    cv_text_in(172, y + 32, 40, &AF_S, v->val[1], DZ_GOLD, DZ_PANEL);
    cv_rect(36, DZ_Y + 86, 120, 2, DZ_CREAM);
    cv_rect(96, DZ_Y + 81, 1, 12, DZ_CREAM);
    cv_text_on(24, DZ_Y + 79, &AF_S, "-", DZ_CREAM, DZ_PANEL);
    cv_text_on(160, DZ_Y + 79, &AF_S, "+", DZ_CREAM, DZ_PANEL);
    cv_rect(36 + a * 116 / 1000, DZ_Y + 80, 5, 14, DZ_GOLD);
}
static void dz_volpan(const gb_view_t *v, uint16_t bgc)
{
    int32_t p = v->ratio[1];
    dz_panel(12, DZ_Y, 216, DZ_H, "VOL PAN", bgc);
    dz_knob(70, DZ_Y + 58, 24, v->ratio[0], v->hot == 1u);
    cv_text_in(52, DZ_Y + 50, 36, &AF_S, v->val[0], DZ_INK, DZ_KNOB);
    cv_rect(118, DZ_Y + 24, 1, 70, DZ_GRID);
    dz_knob(172, DZ_Y + 44, 15, p, v->hot == 2u);
    cv_text_in(160, DZ_Y + 38, 24, &AF_S, v->val[1], DZ_INK, DZ_KNOB);
    cv_text_on(134, DZ_Y + 64, &AF_S, "L", DZ_CREAM, DZ_PANEL);
    cv_text_r(214, DZ_Y + 64, &AF_S, "R", DZ_CREAM, DZ_PANEL);
    cv_rect(134, DZ_Y + 82, 80, 1, DZ_GOLD);
    cv_rect(134, DZ_Y + 86, 80, 1, DZ_GOLD);
    cv_rect(134, DZ_Y + 90, 80, 1, DZ_GOLD);
    cv_rect(134 + p * 76 / 1000, DZ_Y + 78, 4, 16, DZ_GOLD);
}
static void dz_filter(const gb_view_t *v, uint16_t bgc)
{
    static const char *const F[3] = {"100", "1k", "10k"};
    int32_t sx = 72, sw = 120, sy = DZ_Y + 12, sh = 56, i, ty, tr = v->ratio[2];
    char t = v->val[2][0];
    ty = t == 'L' ? 0 : t == 'B' ? 1 : t == 'H' ? 2 : tr * 3 / 1001;
    dz_panel(12, DZ_Y, 216, DZ_H, 0, bgc);
    dz_knob(42, DZ_Y + 42, 18, v->ratio[0], v->hot == 1u);
    dz_screen(sx, sy, sw, sh);
    dz_curve(sx + 2, sx + sw - 2, ty, sx + 8 + v->ratio[0] * (sw - 16) / 1000, v->ratio[1], sy + 4, sy + sh - 2, DZ_CREAM, DZ_OX);
    for (i = 0; i < 3; i++)
        cv_text_in(sx + 10 + i * 42 - 12, sy + sh + 2, 24, &AF_S, F[i], DZ_CREAM, DZ_PANEL);
    cv_text_on(26, DZ_Y + 80, &AF_S, "LP", DZ_CREAM, DZ_PANEL);
    cv_text_r(214, DZ_Y + 80, &AF_S, "HP", DZ_CREAM, DZ_PANEL);
    cv_rect(48, DZ_Y + 86, 140, 2, DZ_GOLD);
    for (i = 0; i <= 6; i++)
        cv_rect(48 + i * 23, DZ_Y + 83, 1, 7, DZ_GOLD);
    cv_rrect(48 + (t == 'L' ? 0 : t == 'B' ? 500 : t == 'H' ? 1000 : tr) * 132 / 1000, DZ_Y + 81, 9, 11, 2, DZ_LED, DZ_PANEL);
    cv_text_on(198, sy - 2, &AF_S, "ENV", DZ_CREAM, DZ_PANEL);
    for (i = 0; i < 6; i++) {
        int32_t lit = (v->mask & 8u) && (v->ratio[3] - 500 > 0 ? v->ratio[3] - 500 : 500 - v->ratio[3]) * 6 / 500 > i;
        cv_rect(202, sy + 50 - i * 7, 14, 5, lit ? DZ_LED : DZ_GRID);
    }
}
static void dz_lfo(const gb_view_t *v, uint16_t bgc)
{
    uint32_t w = tl_wave(v->val[1]), cyc = 1u + (uint32_t)v->ratio[0] * 3u / 1000u;
    int32_t sx = 76, sw = 140, sy = DZ_Y + 10, sh = 50, x, py = 0, mid = sy + sh / 2, amp = 20;
    dz_panel(12, DZ_Y, 216, DZ_H, 0, bgc);
    cv_text_in(20, DZ_Y + 8, 50, &AF_S, "RATE", DZ_CREAM, DZ_PANEL);
    dz_knob(45, DZ_Y + 40, 15, v->ratio[0], v->hot == 1u);
    dz_screen(sx, sy, sw, sh);
    for (x = sx + 2; x < sx + sw - 2; x += 6)
        cv_rect(x, mid, 3, 1, DZ_CREAM);
    if (v->mask & 8u) {
        int32_t a = v->ratio[3] - 500;
        amp = 6 + (a < 0 ? -a : a) * 16 / 500;
    }
    for (x = 0; x <= sw - 8; x += 2) {
        int32_t y = mid - dz_wave(w, (uint32_t)x * cyc * 1024u / (uint32_t)(sw - 8)) * amp / 256;
        if (x)
            cv_line_t(sx + 2 + x, py, sx + 4 + x, y, DZ_CREAM, 2);
        py = y;
    }
    cv_text_in(20, DZ_Y + 66, 50, &AF_S, "WAVE", DZ_CREAM, DZ_PANEL);
    fxg_disc(24, DZ_Y + 88, 3, DZ_LED);
    dz_box(30, DZ_Y + 80, 44, 16, v->val[1], 0);
    cv_text_in(84, DZ_Y + 66, 54, &AF_S, "DEST", DZ_CREAM, DZ_PANEL);
    fxg_disc(88, DZ_Y + 88, 3, DZ_LED);
    dz_box(94, DZ_Y + 80, 46, 16, (v->mask & 4u) ? v->val[2] : "-", 0);
    cv_text_in(150, DZ_Y + 66, 66, &AF_S, "AMT", DZ_CREAM, DZ_PANEL);
    cv_rect(152, DZ_Y + 88, 62, 2, DZ_GOLD);
    cv_rect(183, DZ_Y + 84, 1, 9, DZ_GOLD);
    cv_rect(152 + ((v->mask & 8u) ? v->ratio[3] : 500) * 58 / 1000, DZ_Y + 82, 5, 13, DZ_LED);
}
static void dz_sync(const gb_view_t *v, uint16_t bgc)
{
    static const char *const D[7] = {"1/16", "1/8", "1/4", "1/2", "1", "2", "4"};
    int32_t i, sy = DZ_Y + 48, sh = 42, x, py = 0, off = v->val[0][0] == 'O' && v->val[0][1] == 'F';
    int32_t note = v->val[1][0] == 'N', fade = v->ratio[3] * 60 / 1000;
    dz_panel(12, DZ_Y, 216, DZ_H, 0, bgc);
    cv_rrect(22, DZ_Y + 8, 196, 34, 4, DZ_GOLD, DZ_PANEL);
    cv_rrect(23, DZ_Y + 9, 194, 32, 4, DZ_SCREEN, DZ_GOLD);
    for (i = 0; i < 7; i++) {
        if (!(i & 1))
            cv_text_in(22 + i * 29, DZ_Y + 10, 34, &AF_S, D[i], DZ_CREAM, DZ_SCREEN);
        cv_rect(38 + i * 29, DZ_Y + 28, 1, 6, DZ_GOLD);
    }
    cv_rect(30, DZ_Y + 34, 180, 1, DZ_GOLD);
    if (!off) {
        int32_t mx = 30 + v->ratio[0] * 176 / 1000;
        for (i = 0; i < 5; i++)
            cv_rect(mx - i, DZ_Y + 30 + i, 2 * i + 1, 1, DZ_LED);
    }
    dz_screen(24, sy, 128, sh);
    for (x = 0; x < 124; x += 2) {
        uint32_t ph = (uint32_t)(note && x >= 64 ? x - 64 : x) * 2048u / 124u;
        int32_t a = fade && x < fade ? 14 * x / fade : 14, y = sy + sh / 2 - tl_sin(ph) * a / 256;
        if (x)
            cv_line_t(24 + x, py, 26 + x, y, DZ_CREAM, 2);
        py = y;
    }
    if (note)
        cv_rect(88, sy, 2, sh, DZ_LED);
    cv_rect(158, sy, 1, sh, DZ_GRID);
    cv_text_in(164, sy - 2, 52, &AF_S, "FADE", DZ_CREAM, DZ_PANEL);
    cv_rect(166, sy + 14, 48, 26, DZ_SCREEN);
    for (x = 0; x < 48; x++) {
        int32_t h = fade ? (x * 60 / 48 < fade ? x * 24 * 60 / 48 / (fade ? fade : 1) : 24) : 24;
        h = h > 24 ? 24 : h;
        cv_rect(166 + x, sy + 39 - h, 1, h, DZ_OX);
    }
}
static void dz_sends(const gb_view_t *v, uint16_t bgc)
{
    int32_t y1 = DZ_Y + 30, y2 = DZ_Y + 74;
    dz_panel(12, DZ_Y, 216, DZ_H, 0, bgc);
    dz_box(22, DZ_Y + 42, 34, 20, "TRK", 1);
    fxg_disc(62, DZ_Y + 52, 3, DZ_LED);
    cv_line_t(62, DZ_Y + 52, 72, y1, DZ_GOLD, 2);
    cv_line_t(62, DZ_Y + 52, 72, y2, DZ_GOLD, 2);
    cv_line_t(72, y1, 84, y1, DZ_GOLD, 2);
    cv_line_t(72, y2, 84, y2, DZ_GOLD, 2);
    dz_knob(98, y1, 12, v->ratio[0], v->hot == 1u);
    dz_knob(98, y2, 12, v->ratio[1], v->hot == 2u);
    cv_text_on(116, y1 - 18, &AF_S, "DLY", DZ_CREAM, DZ_PANEL);
    cv_text_on(116, y2 - 18, &AF_S, "REV", DZ_CREAM, DZ_PANEL);
    fxg_disc(120, y1, 3, DZ_LED);
    fxg_disc(120, y2, 3, DZ_LED);
    dz_arrow(124, y1, 150, DZ_GOLD);
    dz_arrow(124, y2, 150, DZ_GOLD);
    dz_tag(154, y1 - 9, 62, "DELAY");
    dz_tag(154, y2 - 9, 62, "REVERB");
}
static void dz_wavebox(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t wv, int32_t cyc)
{
    int32_t i, py = 0;
    cv_rrect(x, y, w, h, 3, DZ_GOLD, DZ_PANEL);
    cv_rrect(x + 2, y + 2, w - 4, h - 4, 3, DZ_SCREEN, DZ_GOLD);
    for (i = 0; i <= w - 12; i += 2) {
        int32_t yy = y + h / 2 - dz_wave(wv, (uint32_t)(i * cyc * 1024 / (w - 12))) * (h / 2 - 6) / 256;
        if (i)
            cv_line_t(x + 4 + i, py, x + 6 + i, yy, DZ_CREAM, 2);
        py = yy;
    }
}
static void dz_mono1(const gb_view_t *v, uint16_t bgc)
{
    const char *w = v->val[0];
    uint32_t w1 = w[0] == 'S' && w[1] == 'Q' ? 4u : w[0] == 'T' ? 1u : w[0] == 'S' && w[1] == 'I' ? 0u : 2u;
    dz_panel(12, DZ_Y, 216, DZ_H, 0, bgc);
    cv_text_in(24, DZ_Y + 8, 60, &AF_S, "OSC1", DZ_GOLD, DZ_PANEL);
    cv_text_in(156, DZ_Y + 8, 60, &AF_S, "OSC2", DZ_GOLD, DZ_PANEL);
    dz_wavebox(22, DZ_Y + 22, 64, 44, w1, 2);
    dz_wavebox(154, DZ_Y + 22, 64, 44, 4u, 2);
    cv_text_in(96, DZ_Y + 12, 48, &AF_S, "MIX", DZ_CREAM, DZ_PANEL);
    dz_knob(120, DZ_Y + 44, 14, v->ratio[1], v->hot == 2u);
    cv_rect(86, DZ_Y + 44, 16, 2, DZ_GOLD);
    cv_rect(138, DZ_Y + 44, 16, 2, DZ_GOLD);
    cv_rrect(24, DZ_Y + 74, 192, 20, 5, DZ_GOLD, DZ_PANEL);
    cv_rrect(25, DZ_Y + 75, 190, 18, 5, DZ_SCREEN, DZ_GOLD);
    cv_text_on(32, DZ_Y + 77, &AF_S, "FINE", DZ_CREAM, DZ_SCREEN);
    {
        int32_t i, fx = 74 + ((v->mask & 4u) ? v->ratio[2] : 500) * 70 / 1000;
        for (i = 0; i <= 14; i++)
            cv_rect(74 + i * 5, DZ_Y + (i == 7 ? 79 : 83), 1, i == 7 ? 10 : 6, DZ_GOLD);
        cv_rect(fx - 1, DZ_Y + 77, 3, 14, DZ_CREAM);
    }
    {
        char sb[14] = "SEMI ";
        str_cpy(sb + 5, (v->mask & 8u) ? v->val[3] : "0", sizeof sb - 5u);
        cv_rrect(152, DZ_Y + 77, 60, 14, 3, DZ_GOLD, DZ_SCREEN);
        cv_text_in(152, DZ_Y + 77, 60, &AF_S, sb, DZ_INK, DZ_GOLD);
    }
}
static void dz_cellhdr(int32_t x, int32_t w, const char *s)
{
    cv_rrect(x, DZ_Y + 10, w, 14, 3, DZ_GOLD, DZ_PANEL);
    cv_text_in(x, DZ_Y + 10, w, &AF_S, s, DZ_INK, DZ_GOLD);
}
static void dz_mono2(const gb_view_t *v, uint16_t bgc)
{
    int32_t k, x, py = 0, pw = 8 + v->ratio[2] * 24 / 1000;
    dz_panel(12, DZ_Y, 216, DZ_H, 0, bgc);
    for (k = 0; k < 4; k++) {
        dz_cellhdr(20 + k * 51, 46, (v->mask & (1u << k)) ? v->lab[k] : "-");
        if (k)
            cv_rect(18 + k * 51, DZ_Y + 28, 1, 64, DZ_GRID);
    }
    cv_rect(22, DZ_Y + 32, 42, 56, DZ_SCREEN);
    cv_line_t(28, DZ_Y + 66, 28, DZ_Y + 46, DZ_CREAM, 2);
    cv_line_t(28, DZ_Y + 46, 42, DZ_Y + 46, DZ_CREAM, 2);
    cv_line_t(42, DZ_Y + 46, 42, DZ_Y + 66, DZ_CREAM, 2);
    cv_line_t(42, DZ_Y + 66, 56, DZ_Y + 66, DZ_CREAM, 2);
    cv_line_t(56, DZ_Y + 66, 56, DZ_Y + 46, DZ_CREAM, 2);
    cv_text_in(22, DZ_Y + 72, 42, &AF_S, v->val[0], DZ_CREAM, DZ_SCREEN);
    dz_knob(94, DZ_Y + 60, 14, v->ratio[1], v->hot == 2u);
    cv_rect(124, DZ_Y + 32, 42, 56, DZ_SCREEN);
    cv_line_t(128, DZ_Y + 66, 145 - pw / 2, DZ_Y + 66, DZ_CREAM, 2);
    cv_line_t(145 - pw / 2, DZ_Y + 66, 145 - pw / 2, DZ_Y + 44, DZ_CREAM, 2);
    cv_line_t(145 - pw / 2, DZ_Y + 44, 145 + pw / 2, DZ_Y + 44, DZ_CREAM, 2);
    cv_line_t(145 + pw / 2, DZ_Y + 44, 145 + pw / 2, DZ_Y + 66, DZ_CREAM, 2);
    cv_line_t(145 + pw / 2, DZ_Y + 66, 162, DZ_Y + 66, DZ_CREAM, 2);
    cv_line_t(134, DZ_Y + 78, 156, DZ_Y + 78, DZ_CREAM, 1);
    cv_rect(175, DZ_Y + 32, 42, 56, DZ_SCREEN);
    for (x = 0; x <= 36; x += 2) {
        int32_t s = (x - 18) * 14, g = 256 + v->ratio[3] * 3, y;
        s = s * g / 256;
        s = s > 256 ? 256 : s < -256 ? -256 : s;
        y = DZ_Y + 60 - s * 22 / 256;
        if (x)
            cv_line_t(176 + x, py, 178 + x, y, DZ_LED, 2);
        py = y;
    }
}
static void dz_color(const gb_view_t *v, uint16_t bgc)
{
    uint32_t k;
    dz_panel(12, DZ_Y, 216, DZ_H, 0, bgc);
    for (k = 0; k < 4u; k++) {
        int32_t cx = DZ_KX(k), on = (v->mask >> k) & 1u, y = DZ_Y + 72;
        if (k)
            cv_rect(cx - 28, DZ_Y + 12, 1, 82, DZ_GRID);
        cv_text_in(cx - 26, DZ_Y + 8, 52, &AF_S, on ? v->lab[k] : "-", DZ_CREAM, DZ_PANEL);
        if (!on) {
            cv_rrect(cx - 16, DZ_Y + 28, 32, 58, 4, DZ_GRID, DZ_PANEL);
            cv_rrect(cx - 15, DZ_Y + 29, 30, 56, 4, DZ_SCREEN, DZ_GRID);
            cv_rect(cx - 5, DZ_Y + 56, 11, 2, DZ_GRID);
            continue;
        }
        dz_knob(cx, DZ_Y + 44, 14, v->ratio[k], v->hot == k + 1u);
        if (str_eq(v->lab[k], "CUT")) {
            cv_rect(cx - 14, y + 4, 16, 1, DZ_GOLD);
            cv_line_t(cx + 2, y + 4, cx + 12, y + 14, DZ_GOLD, 1);
            cv_rect(cx - 14, y + 15, 28, 1, DZ_GOLD);
        } else if (str_eq(v->lab[k], "DEC") || str_eq(v->lab[k], "DECAY")) {
            cv_line_t(cx - 12, y + 14, cx - 8, y + 2, DZ_GOLD, 1);
            cv_line_t(cx - 8, y + 2, cx - 2, y + 10, DZ_GOLD, 1);
            cv_line_t(cx - 2, y + 10, cx + 12, y + 14, DZ_GOLD, 1);
        } else if (str_eq(v->lab[k], "SPRD")) {
            fxg_ring(cx - 4, y + 8, 6, DZ_GOLD);
            fxg_ring(cx + 4, y + 8, 6, DZ_GOLD);
        } else {
            int32_t x, py = y + 8;
            for (x = -12; x <= 12; x += 2) {
                int32_t yy = y + 8 - tl_sin((uint32_t)(x + 12) * 1024u / 24u) * 6 / 256;
                if (x > -12)
                    cv_line_t(cx + x - 2, py, cx + x, yy, DZ_GOLD, 1);
                py = yy;
            }
        }
    }
}
static void dz_808a(const gb_view_t *v, uint16_t bgc)
{
    int32_t sx = 32, sw = 140, sy = DZ_Y + 10, sh = 50, x, py = sy + sh / 2, dec = 20 + v->ratio[1] * 200 / 1000;
    char tb[16] = "TUNE ";
    dz_panel(12, DZ_Y, 216, DZ_H, 0, bgc);
    dz_screen(sx, sy, sw, sh);
    for (x = 0; x < sw - 4; x += 2) {
        int32_t a = 22 * dec / (dec + x * 2), y = sy + sh / 2 - tl_sin((uint32_t)(x * 1024 * (5 + x / 30) / sw)) * a / 22 * 20 / 256;
        if (x)
            cv_line_t(sx + 2 + x - 2, py, sx + 2 + x, y, DZ_CREAM, 2);
        py = y;
    }
    cv_rect(176, sy, 44, sh, DZ_SCREEN);
    cv_rect(180, sy + sh / 2, 36, 1, DZ_GRID);
    cv_line_t(188, sy + sh / 2, 190, sy + 6 + (1000 - v->ratio[2]) * 14 / 1000, DZ_CREAM, 1);
    cv_line_t(190, sy + 6 + (1000 - v->ratio[2]) * 14 / 1000, 193, sy + sh - 8, DZ_CREAM, 1);
    cv_line_t(193, sy + sh - 8, 214, sy + sh / 2, DZ_CREAM, 1);
    str_cpy(tb + 5, (v->mask & 1u) ? v->val[0] : "-", sizeof tb - 5u);
    cv_rrect(110, sy + 2, 60, 14, 3, DZ_GOLD, DZ_SCREEN);
    cv_rrect(111, sy + 3, 58, 12, 3, DZ_SCREEN, DZ_GOLD);
    cv_text_in(110, sy + 2, 60, &AF_S, tb, DZ_GOLD, DZ_SCREEN);
    dz_knobrow(v, DZ_Y + 80, 11);
}
static void dz_808b(const gb_view_t *v, uint16_t bgc)
{
    int32_t sx = 30, sw = 120, sy = DZ_Y + 10, sh = 50, x, py = 0, swp = 6 + v->ratio[0] * 36 / 1000, st = 6 + v->ratio[1] * 60 / 1000;
    int32_t i, dr = 256 + v->ratio[2] * 3;
    dz_panel(12, DZ_Y, 216, DZ_H, 0, bgc);
    dz_screen(sx, sy, sw, sh);
    for (x = 0; x < sw - 6; x += 2) {
        int32_t y = sy + sh - 8 - swp * st / (st + x);
        if (x)
            cv_line_t(sx + 2 + x - 2, py, sx + 2 + x, y, DZ_CREAM, 2);
        py = y;
    }
    fxg_ring(sx + 3, sy + sh - 8 - swp, 3, DZ_CREAM);
    cv_rect(154, sy, 40, sh, DZ_SCREEN);
    py = 0;
    for (x = 0; x <= 36; x += 2) {
        int32_t s = (x - 18) * 14 * dr / 256, y;
        s = s > 256 ? 256 : s < -256 ? -256 : s;
        y = sy + sh / 2 - s * 18 / 256;
        if (x)
            cv_line_t(156 + x - 2, py, 156 + x, y, DZ_CREAM, 2);
        py = y;
    }
    for (i = 0; i < 7; i++)
        cv_rect(200, sy + sh - 6 - i * 7, 14, 5, (v->mask & 8u) && v->ratio[3] * 7 / 1000 > i ? DZ_GOLD : DZ_GRID);
    dz_knobrow(v, DZ_Y + 80, 11);
}
static void dz_crush(const gb_view_t *v, uint16_t bgc)
{
    int32_t k, x, py, c = v->ratio[0], hold = 1 + c / 120;
    dz_panel(12, DZ_Y, 216, DZ_H, 0, bgc);
    for (k = 0; k < 3; k++)
        dz_screen(22 + k * 66, DZ_Y + 10, 60, 44);
    py = DZ_Y + 32;
    for (x = 0; x < 56; x += 2) {
        int32_t xs = x / hold * hold, y = DZ_Y + 32 - tl_sin((uint32_t)xs * 1024u / 56u) * 16 / 256;
        y = c > 30 ? DZ_Y + 32 + (y - DZ_Y - 32) / (1 + c / 250) * (1 + c / 250) : y;
        if (x)
            cv_line_t(24 + x - 2, py, 24 + x, y, DZ_CREAM, 2);
        py = y;
    }
    py = DZ_Y + 32;
    for (x = 0; x < 56; x += 2) {
        int32_t y = DZ_Y + 32 - tl_sin((uint32_t)x * 1536u / 56u) * (6 + v->ratio[1] * 12 / 1000) / 256;
        if (x)
            cv_line_t(90 + x - 2, py, 90 + x, y, DZ_CREAM, 2);
        py = y;
    }
    py = DZ_Y + 46;
    for (x = 0; x < 56; x += 2) {
        int32_t g = 2 + v->ratio[2] * 40 / 1000, y = x < 10 ? DZ_Y + 46 : DZ_Y + 20 + 26 * g / (g + (x - 10) * 2);
        if (x)
            cv_line_t(156 + x - 2, py, 156 + x, y, DZ_CREAM, 2);
        py = y;
    }
    dz_knobrow(v, DZ_Y + 78, 11);
}
static void dz_knobs(const gb_view_t *v, uint16_t bgc)
{
    uint32_t k;
    dz_panel(12, DZ_Y, 216, DZ_H, 0, bgc);
    for (k = 0; k < 4u; k++)
        if (v->mask & (1u << k))
            cv_text_in(DZ_KX(k) - 26, DZ_Y + 12, 52, &AF_S, v->lab[k], DZ_CREAM, DZ_PANEL);
    dz_knobrow(v, DZ_Y + 56, 16);
}

static void dz_tape(const gb_view_t *v, uint16_t bgc)
{
    const uint16_t shell = GB_RGB(236, 226, 204), red = GB_RGB(214, 40, 34), lab = GB_RGB(246, 240, 226), sh2 = GB_RGB(200, 188, 164);
    int32_t x = 22, y = DZ_Y + 2, w = 196, h = DZ_H - 4, i, on = v->val[0][0] == 'O' && v->val[0][1] == 'N';
    uint32_t ang = on && v->play ? (fm1_ms / 40u) % 360u : 0u;
    cv_rrect(x, y, w, h, 6, GB_RGB(150, 140, 120), bgc);
    cv_rrect(x + 1, y + 1, w - 2, h - 2, 6, shell, GB_RGB(150, 140, 120));
    for (i = 0; i < 4; i++) {
        int32_t sx = i & 1 ? x + w - 8 : x + 7, sy = i & 2 ? y + h - 8 : y + 7;
        fxg_disc(sx, sy, 3, sh2);
        cv_rect(sx - 2, sy, 5, 1, GB_RGB(110, 100, 86));
    }
    cv_rrect(x + 12, y + 8, w - 24, 34, 4, lab, shell);
    cv_text_on(x + 16, y + 9, &AF_M, "TRAPSTATION", DZ_INK, lab);
    cv_text_on(x + 16, y + 26, &AF_S, "KASSETTE HELL", GB_RGB(90, 84, 74), lab);
    cv_rect(x + 124, y + 11, 1, 28, DZ_INK);
    cv_rect(x + 130, y + 24, 30, 1, GB_RGB(150, 140, 120));
    cv_rect(x + 130, y + 34, 30, 1, GB_RGB(150, 140, 120));
    cv_text_r(x + w - 16, y + 10, &AF_M, "1", DZ_INK, lab);
    cv_rect(x + 12, y + 42, w - 24, 22, red);
    cv_text_on(x + 16, y + 44, &AF_M, "C-60", DZ_INK, red);
    cv_rrect(x + 62, y + 42, 96, 22, 10, GB_RGB(226, 214, 190), red);
    cv_rect(x + 92, y + 46, 36, 14, GB_RGB(110, 72, 44));
    {
        int32_t r;
        for (r = 0; r < 2; r++) {
            int32_t cx = r ? x + 140 : x + 80, cy = y + 53, a;
            fxg_disc(cx, cy, 9, GB_RGB(248, 244, 236));
            fxg_ring(cx, cy, 9, GB_RGB(150, 140, 120));
            for (a = 0; a < 360; a += 60) {
                int32_t px, py;
                dial_pt(cx, cy, 6, (int32_t)((ang + (uint32_t)a) % 360u), &px, &py);
                cv_rect(px - 1, py - 1, 2, 2, GB_RGB(150, 140, 120));
            }
            fxg_disc(cx, cy, 3, GB_RGB(150, 140, 120));
        }
    }
    for (i = 0; i < 3; i++)
        fxg_disc(x + 22 + i * 13, y + 76, 5, red);
    cv_rrect(x + 112, y + 69, 70, 15, 2, DZ_INK, shell);
    cv_rrect(x + 113, y + 70, 68, 13, 2, shell, DZ_INK);
    cv_text_in(x + 112, y + 69, 70, &AF_S, "CASSETTE", DZ_INK, shell);
    cv_rect(x + 50, y + h - 14, w - 100, 1, GB_RGB(150, 140, 120));
    fxg_disc(x + 62, y + h - 7, 3, GB_RGB(60, 56, 50));
    fxg_disc(x + w - 62, y + h - 7, 3, GB_RGB(60, 56, 50));
    cv_rect(x + 84, y + h - 9, 6, 5, GB_RGB(60, 56, 50));
    cv_rect(x + w - 90, y + h - 9, 6, 5, GB_RGB(60, 56, 50));
    if (!on)
        cv_text_on(x + 66, y + 70, &AF_S, "OFF", GB_RGB(150, 140, 120), shell);
}
static void dz_vhs(const gb_view_t *v, uint16_t bgc)
{
    const uint16_t silver = GB_RGB(214, 214, 208), red = GB_RGB(212, 38, 32), blk = GB_RGB(28, 28, 26);
    int32_t x = 30, y = DZ_Y + 4, w = 136, h = DZ_H - 10, r;
    (void)v;
    cv_rrect(x + 104, y + 8, 76, h - 14, 4, blk, bgc);
    fxg_disc(x + 160, y + h / 2, 18, GB_RGB(70, 70, 66));
    fxg_disc(x + 160, y + h / 2, 12, GB_RGB(150, 146, 136));
    fxg_disc(x + 160, y + h / 2, 5, GB_RGB(70, 70, 66));
    cv_rect(x + w - 4, y + 12, 6, h - 22, GB_RGB(70, 70, 66));
    cv_rect(x + 2, y + h, w, 3, GB_RGB(160, 150, 130));
    cv_rect(x, y, w, h, GB_RGB(150, 150, 144));
    cv_rect(x + 1, y + 1, w - 2, h - 2, silver);
    for (r = 0; r < h - 2; r++) {
        int32_t yy = y + 1 + r, xs = x + 1 + (h - r) * 5 - 30, xe = xs + 26;
        if (xs >= x + w - 1)
            continue;
        if (xs < x + 1)
            xs = x + 1;
        if (xe > x + w - 1)
            xe = x + w - 1;
        if (xe > xs)
            cv_rect(xs, yy, xe - xs, 1, red);
        if (xe < x + w - 1)
            cv_rect(xe, yy, x + w - 1 - xe, 1, blk);
    }
    cv_text_on(x + 8, y + 6, &AF_M, "TRAPSTATION", blk, silver);
    cv_rect(x + 8, y + 24, 92, 2, red);
    cv_text_on(x + 12, y + 28, &AF_S, "VIDEO CASSETTE", blk, silver);
    cv_text_on(x + 8, y + 42, &AF_L, "E-180", blk, silver);
    cv_rrect(x + w - 40, y + h - 22, 34, 16, 2, silver, blk);
    cv_rrect(x + w - 39, y + h - 21, 32, 14, 2, blk, silver);
    cv_text_in(x + w - 40, y + h - 21, 34, &AF_S, "VHS", silver, blk);
}
static void dz_vinyl(const gb_view_t *v, uint16_t bgc)
{
    const uint16_t plinth = GB_RGB(196, 192, 182), dark = GB_RGB(118, 114, 106), blk = GB_RGB(26, 26, 24);
    int32_t x = 20, y = DZ_Y + 6, w = 200, h = DZ_H - 14, cx = x + 82, cy = y + h / 2 + 1, r;
    uint32_t ang = v->play ? (fm1_ms / 6u) % 360u : 40u;
    cv_rect(x + 8, y + h, 14, 6, DZ_DGOLD);
    cv_rect(x + w - 22, y + h, 14, 6, DZ_DGOLD);
    cv_rrect(x, y, w, h, 6, dark, bgc);
    cv_rrect(x + 1, y + 1, w - 2, h - 4, 6, plinth, dark);
    fxg_disc(cx, cy, 44, GB_RGB(160, 156, 146));
    fxg_disc(cx, cy, 41, blk);
    for (r = 38; r > 18; r -= 4)
        fxg_ring(cx, cy, r, GB_RGB(52, 52, 48));
    {
        int32_t px, py;
        dial_pt(cx, cy, 30, (int32_t)ang, &px, &py);
        cv_rect(px - 1, py - 1, 3, 3, GB_RGB(120, 120, 112));
    }
    fxg_disc(cx, cy, 15, DZ_CREAM);
    fxg_ring(cx, cy, 15, DZ_GOLD);
    fxg_disc(cx, cy, 2, DZ_INK);
    fxg_disc(x + w - 30, y + 18, 9, GB_RGB(150, 146, 136));
    fxg_disc(x + w - 30, y + 18, 5, DZ_DGOLD);
    cv_line_t(x + w - 30, y + 18, x + w - 26, y + 62, GB_RGB(170, 166, 156), 3);
    cv_line_t(x + w - 26, y + 62, cx + 26, cy + 18, GB_RGB(170, 166, 156), 3);
    cv_rect(cx + 20, cy + 14, 12, 8, DZ_GOLD);
    cv_rect(cx + 22, cy + 22, 4, 3, DZ_LED);
    fxg_disc(x + 12, y + h - 12, 3, DZ_DGOLD);
    fxg_disc(x + 24, y + h - 12, 3, DZ_DGOLD);
}
static void dz_comb(const gb_view_t *v, uint16_t bgc)
{
    static const char *const L[4] = {"MIX", "TUNE", "FDBK", "DAMP"};
    const uint16_t orange = GB_RGB(246, 150, 40), oglow = GB_RGB(120, 66, 20);
    int32_t k, x, sx = 34, sw = 172, sy = DZ_Y + 56, sh = 38, n = 4 + v->ratio[1] * 8 / 1000, fb = v->ratio[2], dm = v->ratio[3];
    dz_panel(16, DZ_Y, 208, DZ_H, 0, bgc);
    cv_rect(24, DZ_Y + 30, 4, 60, DZ_GOLD);
    cv_rect(212, DZ_Y + 30, 4, 60, DZ_GOLD);
    for (k = 0; k < 4; k++) {
        int32_t cx = 60 + k * 40, px, py;
        cv_text_in(cx - 20, DZ_Y + 6, 40, &AF_S, L[k], DZ_CREAM, DZ_PANEL);
        fxg_disc(cx, DZ_Y + 34, 11, GB_RGB(120, 110, 90));
        fxg_disc(cx, DZ_Y + 34, 10, DZ_CREAM);
        dial_pt(cx, DZ_Y + 34, 9, dz_ang(v->ratio[k]), &px, &py);
        cv_line_t(cx, DZ_Y + 34, px, py, DZ_INK, 2);
        if (v->hot == (uint32_t)k + 1u)
            cv_rect(cx - 6, DZ_Y + 48, 12, 2, DZ_LED);
    }
    cv_rect(sx - 2, sy - 2, sw + 4, sh + 4, DZ_GOLD);
    cv_rect(sx, sy, sw, sh, DZ_SCREEN);
    for (x = sx + 8; x < sx + sw; x += 12)
        cv_rect(x, sy, 1, sh, GB_RGB(40, 30, 20));
    for (x = 0; x < sw - 4; x++) {
        int32_t c = tl_sin((uint32_t)x * (uint32_t)n * 512u / (uint32_t)sw + 256u), a = c < 0 ? -c : c, h;
        a = a * a / 256;
        if (fb > 400)
            a = a * a / 256;
        if (fb > 750)
            a = a * a / 256;
        a = a * (1000 - dm * x / sw * 8 / 10) / 1000;
        h = 2 + a * (sh - 8) * (300 + v->ratio[0] * 7 / 10) / 1000 / 256;
        cv_rect(sx + 2 + x, sy + sh - 2 - h, 1, h, h > 6 ? orange : oglow);
    }
    {
        char t[16];
        if (v->dj_sel < NTRK) {
            t[0] = 'T';
            t[1] = (char)('1' + v->dj_sel);
            t[2] = ' ';
            str_cpy(t + 3, MACHINES[v->tm[v->dj_sel] % NMACH].pill, sizeof t - 3u);
        } else
            str_cpy(t, "MASTER", sizeof t);
        cv_rect(sx + 3, sy + 2, text_w(&AF_S, t) + 6, 14, DZ_SCREEN);
        cv_text_on(sx + 6, sy + 2, &AF_S, t, orange, DZ_SCREEN);
    }
}
static void dz_amp(const gb_view_t *v, uint16_t bgc)
{
    const uint16_t tolex = GB_RGB(36, 36, 34), plate = GB_RGB(206, 168, 80), grille = GB_RGB(150, 120, 80), badge = GB_RGB(130, 30, 26);
    int32_t x = 28, y = DZ_Y + 12, w = 184, h = DZ_H - 16, k, i;
    cv_rrect(x + 64, DZ_Y + 2, 56, 12, 5, GB_RGB(110, 50, 36), bgc);
    cv_rect(x + 60, DZ_Y + 8, 10, 6, DZ_GOLD);
    cv_rect(x + 114, DZ_Y + 8, 10, 6, DZ_GOLD);
    cv_rrect(x, y, w, h, 6, tolex, bgc);
    for (i = 0; i < 4; i++)
        cv_rect(i & 1 ? x + w - 12 : x, i & 2 ? y + h - 12 : y, 12, 12, DZ_GOLD);
    cv_rrect(x + 10, y + 8, w - 20, 38, 3, plate, tolex);
    for (k = 0; k < 2; k++) {
        int32_t cx = x + 56 + k * 66, cy = y + 30, px, py;
        cv_text_in(cx - 24, y + 9, 48, &AF_S, k ? "PAN" : "AMP", DZ_INK, plate);
        fxg_disc(cx, cy, 11, GB_RGB(80, 66, 40));
        fxg_disc(cx, cy, 10, DZ_CREAM);
        dial_pt(cx, cy, 9, dz_ang(v->ratio[k]), &px, &py);
        cv_line_t(cx, cy, px, py, DZ_INK, 2);
    }
    fxg_disc(x + w - 24, y + 22, 5, DZ_LED);
    cv_rect(x + 10, y + 48, w - 20, h - 56, grille);
    for (i = y + 50; i < y + h - 8; i += 3)
        cv_rect(x + 10, i, w - 20, 1, GB_RGB(124, 98, 64));
    fxg_ring(x + w / 2, y + 48 + (h - 56) / 2, 16, GB_RGB(96, 76, 50));
    cv_rrect(x + w / 2 - 40, y + h - 20, 80, 12, 5, DZ_GOLD, grille);
    cv_rrect(x + w / 2 - 39, y + h - 19, 78, 10, 5, badge, DZ_GOLD);
    cv_text_in(x + w / 2 - 40, y + h - 21, 80, &AF_S, "TRAPSTATION", DZ_CREAM, badge);
}

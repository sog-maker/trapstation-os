/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION OS contributors */

#define FXG_TAPE_RGB(r, g, b) GB_RGB(r, g, b)
static void fxg_disc(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t y, x;
    for (y = -r; y <= r; y++) {
        for (x = 0; x * x + y * y <= r * r; x++)
            ;
        cv_rect(cx - x + 1, cy + y, 2 * x - 1, 1, c);
    }
}
static void fxg_ring(int32_t cx, int32_t cy, int32_t r, uint16_t c)
{
    int32_t a, px, py;
    for (a = 0; a < 360; a += r > 20 ? 1 : 3) {
        dial_pt(cx, cy, r, a, &px, &py);
        cv_rect(px, py, 1, 1, c);
    }
}
static void fxg_reel(int32_t cx, int32_t cy, int32_t tape, uint32_t ang, uint16_t win)
{
    int32_t k, x0, y0, x1, y1;
    fxg_disc(cx, cy, 5 + tape, FXG_TAPE_RGB(70, 50, 38));
    fxg_disc(cx, cy, 5, GB_RGB(236, 233, 226));
    for (k = 0; k < 6; k++) {
        dial_pt(cx, cy, 2, (int32_t)ang * 10 + k * 60, &x0, &y0);
        dial_pt(cx, cy, 4, (int32_t)ang * 10 + k * 60, &x1, &y1);
        cv_line(x0, y0, x1, y1, win);
    }
    fxg_disc(cx, cy, 1, win);
}
static void fxg_cassette(const gb_view_t *v, uint16_t bgc)
{
    const uint16_t ink = GB_W, body = T_SURF, red = T_THEME, win = T_ACCENT, wht = GB_RGB(245, 240, 232);
    int32_t i;
    (void)bgc;
    cv_rrect(23, 49, 194, 72, 6, ink, bgc);
    cv_rrect(24, 50, 192, 70, 5, body, ink);
    fxg_disc(29, 55, 1, ink);
    fxg_disc(211, 55, 1, ink);
    cv_rect(32, 54, 176, 11, wht);
    cv_text_on(35, 53, &AF_S, "TRAPSTATION", ink, wht);
    cv_text_r(205, 53, &AF_S, "A", red, wht);
    cv_rect(32, 66, 176, 25, red);
    cv_text_on(36, 70, &AF_M, "T-70", wht, red);
    cv_rrect(88, 68, 64, 21, 4, win, red);
    fxg_reel(101, 78, 5, v->fxang, win);
    fxg_reel(139, 78, 2, v->fxang, win);
    cv_rect(104, 86, 32, 1, FXG_TAPE_RGB(70, 50, 38));
    for (i = 0; i < 3; i++)
        fxg_disc(38 + i * 9, 101, 3, red);
    cv_text_on(64, 95, &AF_S, "low noise", ink, body);
    cv_rect(66, 108, 108, 12, gb_mix(body, 40));
    for (i = 0; i < 4; i++)
        fxg_disc(i < 2 ? 80 + i * 14 : 146 + (i - 2) * 14, 114, 2, win);
    if (v->ratio[0] >= 500)
        fxg_disc(212, 113, 3, red);
}
static void fxg_vinyl(const gb_view_t *v, uint16_t bgc)
{
    const uint16_t ink = GB_W, plat = T_SURF, red = T_THEME, rec = GB_RGB(14, 14, 14), gro = GB_RGB(44, 44, 44);
    int32_t r, a, px, py, n, i;
    cv_rrect(23, 47, 194, 76, 6, ink, bgc);
    cv_rrect(24, 48, 192, 74, 5, plat, ink);
    for (a = 0; a < 360; a++) {
        uint32_t t = (uint32_t)a * 255u / 359u;
        dial_pt(92, 85, 35, a, &px, &py);
        cv_rect(px - 1, py - 1, 2, 2, GB_RGB(142 + (253 - 142) * t / 255, 28 + (207 - 28) * t / 255, 24 + (61 - 24) * t / 255));
    }
    fxg_disc(92, 85, 33, rec);
    for (r = 15; r < 33; r += 3)
        fxg_ring(92, 85, r, gro);
    fxg_disc(92, 85, 11, red);
    cv_text_in(81, 79, 22, &AF_S, "T", GB_RGB(245, 240, 232), red);
    fxg_disc(92, 85, 1, rec);
    n = 2 + v->ratio[1] * 14 / 1000;
    for (i = 0; i < n; i++) {
        dial_pt(92, 85, 14 + (i * 7) % 18, (i * 137 + (int32_t)v->fxang * 10) % 360, &px, &py);
        cv_rect(px, py, 1, 1, GB_RGB(210, 205, 190));
    }
    fxg_disc(190, 58, 6, gb_mix(plat, 90));
    fxg_disc(190, 58, 3, ink);
    cv_line_t(190, 58, 186, 96, ink, 3);
    cv_line_t(186, 96, 168, 108, ink, 3);
    cv_rect(158, 104, 12, 8, ink);
    cv_text_on(194, 106, &AF_S, "33", ink, plat);
}

/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION contributors */

static void dz_dj(const gb_view_t *v, uint16_t bgc)
{
    static const char *const F[3] = {"100", "1k", "10k"};
    uint32_t i;
    int32_t val = v->dj_v[v->dj_sel % (NTRK + 1u)], lp = val < -1, hp = val > 1, sy = DZ_Y + 36, sh = 34, xc;
    char nb[8];
    for (i = 0; i <= NTRK; i++) {
        int32_t cx = 22 + (int32_t)i * 33, cy = DZ_Y + 9, vv = v->dj_v[i], px, py;
        char tb[3] = {i < NTRK ? 'T' : 'M', i < NTRK ? (char)('1' + i) : 0, 0};
        if (i == v->dj_sel)
            cv_rect(cx - 13, cy - 10, 26, 36, T_THEME);
        if (i == v->dj_sel)
            cv_rect(cx - 11, cy - 8, 22, 32, bgc);
        fxg_disc(cx, cy, 9, GB_W);
        fxg_disc(cx, cy, 7, bgc);
        dial_pt(cx, cy, 7, (vv * 135 / 64 + 360) % 360, &px, &py);
        cv_line_t(cx, cy, px, py, vv > 1 || vv < -1 ? T_THEME : GB_W, 2);
        cv_text_in(cx - 11, cy + 10, 22, &AF_S, tb, GB_W, bgc);
    }
    cv_rect(12, sy - 2, 216, 1, T_LINE);
    cv_rect(16, sy, 208, sh, GB_W);
    cv_rect(17, sy + 1, 206, sh - 2, T_SURF);
    xc = lp ? 20 + (val + 64) * 200 / 64 : hp ? 20 + (val) * 200 / 64 : 300;
    dz_curve(18, 222, hp ? 2 : 0, xc, lp || hp ? 500 : 0, sy + 6, sy + sh - 2, T_THEME, tl_tint(T_SURF, 200));
    nb[0] = 0;
    if (lp || hp) {
        nb[0] = lp ? 'L' : 'H';
        nb[1] = 'P';
        fmt_int(nb + 2, (lp ? -val : val) * 100 / 64);
    } else
        str_cpy(nb, "OPEN", sizeof nb);
    cv_text_on(22, sy + 2, &AF_M, nb, GB_W, T_SURF);
    for (i = 0; i < 3u; i++) {
        int32_t x = 48 + (int32_t)i * 72;
        cv_rect(x, sy + sh - 4, 1, 6, GB_W);
        cv_text_in(x - 14, sy + sh + 2, 28, &AF_S, F[i], GB_W, bgc);
    }
    cv_text_on(14, DZ_Y + 90, &AF_S, "LP", GB_W, bgc);
    cv_text_r(226, DZ_Y + 90, &AF_S, "HP", GB_W, bgc);
    cv_rect(40, DZ_Y + 96, 160, 2, GB_W);
    for (i = 0; i <= 8u; i++)
        cv_rect(40 + (int32_t)i * 20, DZ_Y + 93, 1, 8, GB_W);
    cv_rect(40 + (val + 64) * 156 / 128, DZ_Y + 90, 5, 13, T_THEME);
}

static void dz_clef(int32_t cx, int32_t cy, int32_t s, uint16_t c)
{
    int32_t a, px, py, qx = 0, qy = 0;
    for (a = 270; a <= 540; a += 15) {
        int32_t r = 7 * s - (a - 270) * s * 3 / 270;
        dial_pt(cx - s, cy - 2 * s, r < s ? s : r, a % 360, &px, &py);
        if (a > 270)
            cv_line_t(qx, qy, px, py, c, s + 1);
        qx = px;
        qy = py;
    }
    fxg_disc(cx - 5 * s, cy - 3 * s, 2 * s, c);
    fxg_disc(cx + 6 * s, cy - 5 * s, s + 1, c);
    fxg_disc(cx + 6 * s, cy + 1 * s, s + 1, c);
}
static void dz_reel_sym(uint32_t k, uint32_t r, int32_t cx, int32_t cy, int32_t s, const gb_view_t *v, uint16_t bg)
{
    const uint16_t ink = DZ_INK, gold = GB_RGB(220, 170, 40);
    if (k == TL_AN_TRAP) {
        static const uint8_t IC[3] = {IC_KICK, IC_SNARE, IC_CYMBAL};
        gb_icon(cx - 8 * s, cy - 8 * s, s, IC[r], r == 2u ? GB_RGB(200, 150, 30) : ink);
    } else if (k == TL_AN_BASS) {
        if (r == 0u)
            dz_clef(cx, cy + 2 * s, s, ink);
        else if (r == 1u) {
            int32_t x, py = cy;
            for (x = -9 * s; x <= 9 * s; x += 2) {
                int32_t y = cy - tl_sin((uint32_t)(x + 9 * s) * 1024u / (uint32_t)(18 * s)) * 5 * s / 256;
                if (x > -9 * s)
                    cv_line_t(cx + x - 2, py, cx + x, y, ink, s + 1);
                py = y;
            }
        } else {
            cv_text_in(cx - 24, cy - 9, 48, s > 2 ? &AF_L : &AF_M, "808", ink, bg);
            cv_rect(cx - 16, cy + (s > 2 ? 14 : 9), 32, 2, ink);
            cv_rect(cx - 16, cy + (s > 2 ? 18 : 12), 32, 2, ink);
        }
    } else if (k == TL_AN_JAZZ) {
        if (r == 0u) {
            int32_t i, w = 4 * s, h = 9 * s;
            cv_rect(cx - 2 * w - 1, cy - h / 2 - 1, 4 * w + 2, h + 2, ink);
            for (i = 0; i < 4; i++)
                cv_rect(cx - 2 * w + i * w, cy - h / 2, w - 1, h, DZ_CREAM);
            for (i = 1; i < 4; i++)
                cv_rect(cx - 2 * w + i * w - s, cy - h / 2, 2 * s, h * 3 / 5, ink);
        } else if (r == 1u) {
            char b[10];
            str_cpy(b, tl_note((uint32_t)TSEL->p[P_ROOT], (uint32_t)TSEL->p[P_ROOT], (uint32_t)TSEL->p[P_SCALE]), sizeof b);
            str_cpy(b + str_len(b), (v->mask & 1u) ? v->val[0] : "m7", sizeof b - str_len(b));
            cv_text_in(cx - 26, cy - 9, 52, text_w(&AF_M, b) <= 50 ? &AF_M : &AF_S, b, ink, bg);
        } else {
            int32_t i;
            for (i = 0; i < 3; i++) {
                cv_rect(cx - 8 * s, cy - 5 * s + i * 5 * s, 16 * s, s, ink);
                cv_rect(cx - 6 * s + (i == 1 ? 7 * s : i * 3 * s), cy - 5 * s + i * 5 * s - 2 * s / 2 - 1, 3 * s, 3 * s, ink);
            }
        }
    } else {
        if (r == 0u) {
            cv_line_t(cx - 9 * s, cy, cx + 9 * s, cy, ink, s);
            cv_line_t(cx - 6 * s, cy - s, cx + 6 * s, cy - s, ink, s);
            cv_rect(cx - s / 2, cy - 5 * s, s, 10 * s, ink);
        } else if (r == 1u) {
            cv_line_t(cx - 8 * s, cy - 3 * s, cx + 8 * s, cy - 3 * s, ink, s);
            cv_line_t(cx - 5 * s, cy - 4 * s, cx + 5 * s, cy - 4 * s, ink, s);
            cv_line_t(cx - 8 * s, cy + 2 * s, cx + 8 * s, cy + 2 * s, ink, s);
            cv_line_t(cx - 5 * s, cy + 3 * s, cx + 5 * s, cy + 3 * s, ink, s);
            cv_rect(cx - s / 2, cy - 7 * s, s, 14 * s, ink);
        } else {
            int32_t i;
            for (i = -1; i <= 1; i++)
                cv_line_t(cx + i * 5 * s - 2 * s, cy + 5 * s, cx + i * 5 * s + 2 * s, cy - 5 * s, ink, s + 1);
        }
    }
    (void)gold;
}
static void d99_hat(const gb_view_t *, int32_t, int32_t, int32_t, int32_t, uint32_t);
static void tl_automat(const gb_view_t *v, uint32_t k, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t f)
{
    if (k == TL_AN_HAT) { d99_hat(v, x, y, w, h, f); return; }
    static const char *const NM[4] = {"BASSLINE JACKPOT", "JAZZ JACKPOT", "PATTERN JACKPOT", "HIHAT JACKPOT"};
    const uint16_t cream = DZ_CREAM, gold = GB_RGB(214, 170, 70), dgold = GB_RGB(150, 112, 40);
    uint16_t body, trim, mq, mqt;
    int32_t mh = h / 6 + 4, s = h > 140 ? 3 : 2, rw, rh, ry, rx0, i, base;
    uint32_t stop[3] = {14u, 19u, 24u}, kk = k == TL_AN_HAT ? 3u : (k - 1u) % 3u;
    switch (k) {
    case TL_AN_BASS: body = GB_RGB(92, 58, 30); trim = gold; mq = GB_RGB(110, 24, 20); mqt = GB_RGB(244, 200, 90); break;
    case TL_AN_JAZZ: body = GB_RGB(24, 44, 104); trim = gold; mq = GB_RGB(30, 62, 150); mqt = GB_RGB(250, 236, 200); break;
    case TL_AN_TRAP: body = GB_RGB(50, 52, 56); trim = GB_RGB(90, 92, 96); mq = GB_RGB(160, 24, 24); mqt = GB_RGB(250, 236, 220); break;
    default: body = GB_RGB(26, 34, 44); trim = GB_RGB(70, 80, 92); mq = GB_RGB(30, 100, 210); mqt = GB_RGB(250, 246, 236); break;
    }
    rw = (w - 64) / 3;
    ry = y + mh + 10;
    rh = h - mh - (k == TL_AN_HAT ? 46 : 34);
    rx0 = x + (w - (3 * rw + 16)) / 2;
    base = y + h - 16;
    if (k == TL_AN_JAZZ) {
        int32_t a, px, py;
        for (a = -60; a <= 60; a += 15) {
            dial_pt(x + w / 2, y + mh, mh, (a + 360) % 360, &px, &py);
            cv_line_t(x + w / 2, y + mh, px, py, gold, 2);
        }
    }
    cv_rrect(x + 4, y + mh / 2 + 4, w - 8, h - mh / 2 - 6, k == TL_AN_BASS ? 14 : 6, trim, T_BG);
    cv_rrect(x + 6, y + mh / 2 + 6, w - 12, h - mh / 2 - 10, k == TL_AN_BASS ? 12 : 5, body, trim);
    if (k == TL_AN_JAZZ || k == TL_AN_BASS)
        for (i = 0; i < 5; i++) {
            cv_rect(x + 10, ry + 4 + i * (rh / 5), 10, 3, gold);
            cv_rect(x + w - 20, ry + 4 + i * (rh / 5), 10, 3, gold);
        }
    else {
        static const uint16_t LC[3] = {GB_RGB(250, 190, 40), GB_RGB(60, 200, 210), GB_RGB(230, 40, 30)};
        static const uint16_t LH[3] = {GB_RGB(240, 150, 40), GB_RGB(40, 110, 230), GB_RGB(230, 40, 30)};
        for (i = 0; i < 3; i++) {
            uint16_t c = k == TL_AN_TRAP ? LC[i] : LH[i];
            cv_rrect(x + 10, ry + 4 + i * (rh / 3), 8, 8, 2, c, body);
            cv_rrect(x + w - 18, ry + 4 + i * (rh / 3), 8, 8, 2, c, body);
        }
    }
    cv_rrect(x + 14, y, w - 28, mh + 4, k == TL_AN_BASS ? 12 : 7, k == TL_AN_TRAP ? GB_RGB(30, 30, 32) : trim, T_BG);
    cv_rrect(x + 17, y + 3, w - 34, mh - 2, k == TL_AN_BASS ? 10 : 5, mq, trim);
    {
        const char *nm = NM[kk];
        const aafont_t *ff = text_w(&AF_M, nm) <= w - 60 ? &AF_M : &AF_S;
        cv_text_in(x + 17, y + 3 + (mh - 2 - (ff == &AF_M ? 17 : 14)) / 2, w - 34, ff, nm, mqt, mq);
        if (k == TL_AN_TRAP) {
            int32_t tw = text_w(ff, nm), lx = x + w / 2 - tw / 2 - 12, rx = x + w / 2 + tw / 2 + 4;
            cv_line_t(lx, y + mh - 2, lx + 4, y + 6, GB_RGB(250, 80, 60), 2);
            cv_line_t(lx + 5, y + mh - 2, lx + 9, y + 6, GB_RGB(250, 80, 60), 2);
            cv_line_t(rx, y + mh - 2, rx + 4, y + 6, GB_RGB(70, 220, 200), 2);
            cv_line_t(rx + 5, y + mh - 2, rx + 9, y + 6, GB_RGB(70, 220, 200), 2);
        }
    }
    if (k == TL_AN_BASS)
        fxg_disc(x + w - 22, y + 4, 5, GB_RGB(230, 40, 30));
    for (i = 0; i < 3; i++) {
        int32_t rx = rx0 + i * (rw + 8), cy = ry + rh / 2;
        cv_rrect(rx - 2, ry - 2, rw + 4, rh + 4, 4, k == TL_AN_TRAP ? GB_RGB(30, 30, 32) : gold, body);
        cv_rrect(rx, ry, rw, rh, 3, cream, gold);
        if (f && f < stop[i]) {
            int32_t j;
            for (j = 0; j < 4; j++) {
                int32_t sy = ry + 2 + (int32_t)((f * 11u + (uint32_t)j * (uint32_t)rh / 4u + (uint32_t)i * 7u) % (uint32_t)(rh - 6));
                cv_rect(rx + 4, sy, rw - 8, 3, j & 1 ? gb_mix(cream, 90) : gb_mix(cream, 160));
            }
        } else
            dz_reel_sym(k, (uint32_t)i, rx + rw / 2, cy, s, v, cream);
    }
    if (k == TL_AN_HAT) {
        for (i = 0; i < 16; i++)
            cv_rect(x + 30 + i * ((w - 60) / 16), base - 2 - ((i * 7 + 3) % 5) * 2, 3, 4 + ((i * 7 + 3) % 5) * 2, cream);
        cv_rect(x + 24, base + 3, w - 48, 1, gold);
    } else
        for (i = 0; i < 5; i++)
            cv_rect(x + w / 2 - 62 + i * 26, base, 20, 5, i == 2 && k == TL_AN_JAZZ ? GB_RGB(200, 40, 30) : cream);
    (void)dgold;
}

static void dz_chordgen(const gb_view_t *v, uint16_t bgc)
{
    static const char *const SUF[CH_LAST + 1] = {"", "", "", "", "m", "7", "M7", "m7", "sus", "5", "m9", "m7b5", "M9", "m11", "m13",
                                                 "s2", "a9", "ma9"};
    static const char *const SCN[16] = {"", "MAJ", "MIN", "DOR", "MIX", "PEN", "MPEN", "HMIN", "PHR", "LYD", "LOC", "MMIN",
                                        "BLU", "WHL", "DIM", "DIM"};
    const track_t *t = TSEL;
    const uint16_t paper = GB_RGB(244, 236, 214), wood = GB_RGB(120, 76, 40);
    uint32_t k = song.sel % NTRK, b, len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP), sty = (uint32_t)chord_cfg[k][CG_STYLE] % 8u;
    gen_recipe_t r = {1, (uint8_t)sty, (uint8_t)chord_cfg[k][CG_RHYTHM], (uint8_t)(gen_var[k] - 1),
                      (int8_t)chord_cfg[k][CG_OCT], (uint8_t)t->p[P_ROOT], GEN_SCL[sty], (uint8_t)t->p[P_CHRD],
                      (uint8_t)t->p[P_VOIC], (uint8_t)trk_vmode(t), (uint8_t)len, GEN_V2};
    uint32_t at = v->sq_pos ? gen_chord_at(&r, (uint32_t)v->sq_at + v->sq_pos - 1u) : 9u;
    int32_t x0 = 30, x1 = 210, y0 = DZ_Y + 12, y1 = DZ_Y + DZ_H - 10;
    char kb[12];
    str_cpy(kb, tl_note(r.root % 12u, r.root, r.scale), sizeof kb);
    str_cpy(kb + str_len(kb), " ", sizeof kb - str_len(kb));
    str_cpy(kb + str_len(kb), SCN[r.scale % 16u], sizeof kb - str_len(kb));
    cv_text_r(228, 38, &AF_S, kb, GB_W, bgc);
    dz_panel(12, DZ_Y, 216, DZ_H, 0, bgc);
    cv_rect(x0, y0, x1 - x0, y1 - y0, paper);
    for (b = 0; b < 2u; b++) {
        int32_t rx = b ? x1 : x0 - 10;
        cv_rrect(rx, y0 - 4, 10, y1 - y0 + 8, 4, wood, DZ_PANEL);
        cv_rect(rx + 3, y0 - 6, 4, 4, DZ_GOLD);
        cv_rect(rx + 3, y1 + 2, 4, 4, DZ_GOLD);
    }
    for (b = y0 + 6; b < (uint32_t)y1; b += 8)
        cv_rect(x0, (int32_t)b, x1 - x0, 1, GB_RGB(226, 216, 190));
    if (chord_kit(t) || acid_on(t)) {
        cv_text_in(x0, y0 + 30, x1 - x0, &AF_S, "USE A SYNTH TRACK", DZ_INK, paper);
        return;
    }
    for (b = 0; b < 4u; b++) {
        uint32_t root = 0, mode = 0, i = gen_chord_step(&r, b), j, n;
        int32_t cx = x0 + (int32_t)b * 45, iv[CHORD_MAX + 1], rr;
        char nm[10];
        if (b)
            for (j = (uint32_t)y0 + 2; j < (uint32_t)y1 - 2; j += 6)
                cv_rect(cx, (int32_t)j, 1, 3, DZ_INK);
        if (i >= len || !gen_plan(&r, i, &root, &mode))
            continue;
        str_cpy(nm, tl_note(root % 12u, r.root, r.scale), sizeof nm);
        str_cpy(nm + str_len(nm), SUF[mode % (CH_LAST + 1u)], sizeof nm - str_len(nm));
        cv_text_in(cx + 1, y0 + 6, 44, text_w(&AF_M, nm) <= 42 ? &AF_M : &AF_S, nm, b == at % 4u && at < 4u ? T_THEME : DZ_INK, paper);
        rr = (int32_t)root;
        n = chord_tones_cfg(mode, &rr, iv, gen_scale(&r), (int32_t)r.root);
        for (j = 0; j < n && j < 4u; j++) {
            int32_t y = y1 - 8 - (iv[j] % 24) * 2 - (int32_t)(root % 12u) / 2;
            cv_rect(cx + 8, y, 30, 3, DZ_OX);
        }
    }
}

static void dz_tray(int32_t y0, int32_t h, uint16_t bgc)
{
    const uint16_t felt = GB_RGB(150, 26, 24), rim = GB_RGB(110, 50, 24), gold = GB_RGB(214, 170, 70);
    int32_t x0 = 20, w = 200, r;
    for (r = 0; r < h; r++) {
        int32_t in = (h - r) / 5;
        cv_rect(x0 + in, y0 + r, w - 2 * in, 1, r < 3 || r > h - 6 ? rim : felt);
        cv_rect(x0 + in, y0 + r, 3, 1, gold);
        cv_rect(x0 + w - in - 3, y0 + r, 3, 1, gold);
    }
    cv_rect(x0, y0 + h - 2, w, 2, gold);
    (void)bgc;
}
static void dz_var_plate(int32_t cy, uint16_t bgc)
{
    char b[12] = "VAR ";
    uint32_t var = (uint32_t)gen_var[song.sel % NTRK];
    b[4] = (char)('0' + var / 10u % 10u);
    b[5] = (char)('0' + var % 10u);
    b[6] = '/'; b[7] = '2'; b[8] = '0'; b[9] = 0;
    cv_rrect(68, cy, 104, 18, 8, GB_RGB(150, 112, 40), bgc);
    cv_rrect(70, cy + 2, 100, 14, 7, GB_RGB(236, 210, 140), GB_RGB(150, 112, 40));
    cv_text_in(68, cy + 2, 104, &AF_S, b, DZ_INK, GB_RGB(236, 210, 140));
}
static void dz_cup(int32_t cx, int32_t cy, int32_t tilt, uint32_t f)
{
    const uint16_t cup = GB_RGB(160, 30, 26), dk = GB_RGB(100, 18, 16), gold = GB_RGB(214, 170, 70);
    int32_t i, sh = (int32_t)(f & 1u) * 3 - 1;
    cx += tilt < 2 ? sh : 0;
    for (i = 0; i < 40; i++) {
        int32_t off = tilt * (40 - i) / 3, wdt = 36 + i / 5;
        cv_rect(cx - wdt / 2 + off, cy - 40 + i, wdt, 1, i < 3 ? gold : i > 36 ? dk : cup);
        cv_rect(cx - wdt / 2 + off, cy - 40 + i, 2, 1, gold);
    }
    if (tilt >= 2) {
        fxg_disc(cx + 8, cy + 2, 15, dk);
        fxg_disc(cx + 8, cy + 2, 11, GB_RGB(40, 14, 12));
    }
    cv_line_t(cx + 24, cy - 38, cx + 28, cy - 44, gold, 1);
    cv_line_t(cx + 28, cy - 30, cx + 33, cy - 32, gold, 1);
}
static void dz_chordvar(const gb_view_t *v, uint16_t bgc, int32_t y0, int32_t h, uint32_t f)
{
    uint32_t var = (uint32_t)gen_var[song.sel % NTRK], i;
    int32_t s = h > 120 ? 40 : 34, ty = y0 + h - s - 26, plate = y0 + (f >= 25u ? 20 : 2);
    if (f >= 25u || !f)
        dz_var_plate(plate, bgc);
    dz_tray(ty, s + 22, bgc);
    if (f && f < 10u) {
        dz_cup(160, ty + 24, 0, f);
        tl_die(146 + (int32_t)(f & 1u) * 2, ty - 18, 14, f % 6u + 1u, GB_RGB(160, 30, 26));
        tl_die(160 - (int32_t)(f & 1u) * 2, ty - 22, 14, (f + 3u) % 6u + 1u, GB_RGB(160, 30, 26));
    } else if (f >= 10u && f < 25u) {
        int32_t g = (int32_t)f - 10;
        dz_cup(170, ty + 6 - (g < 4 ? g * 2 : 8), g < 4 ? 2 : 3, f);
        for (i = 0; i < 3u; i++) {
            int32_t x = 150 - (int32_t)i * 34 - g * 4 + tl_sin(f * 90u + i * 300u) * 4 / 256, y = ty + 4 + (g < 8 ? (8 - g) * -4 : 0) +
                        (int32_t)i * 4;
            tl_die(x < 24 ? 24 : x, y, s - 6, (f * 5u + i * 2u) % 6u + 1u, GB_RGB(150, 26, 24));
        }
        return;
    }
    if (f && f < 10u)
        return;
    for (i = 0; i < 3u; i++) {
        uint32_t face = (var * (3u + i * 2u) + i) % 6u + 1u;
        int32_t x = 120 - s / 2 + ((int32_t)i - 1) * (s + 18), y = ty + 6 - (i == 1u ? 4 : 0);
        tl_die(x, y, s, face, GB_RGB(150, 26, 24));
    }
    if (f >= 25u)
        cv_text_in(0, y0, 240, &AF_M, "= NEUE VARIANTE =", GB_W, bgc);
    (void)v;
}

static void dz_euclid(const gb_view_t *v, uint16_t bgc)
{
    const uint16_t gold = GB_RGB(212, 170, 55), dgold = GB_RGB(150, 112, 30), strap = GB_RGB(120, 34, 30), dial = GB_RGB(246, 238, 220);
    int32_t len = v->cur[0] < 1 ? 1 : v->cur[0] > 64 ? 64 : v->cur[0], hits = v->cur[1] < 0 ? 0 : v->cur[1] > len ? len : v->cur[1];
    int32_t rot = v->cur[2] < 0 ? 0 : v->cur[2], i, cx = 120, cy = DZ_Y + DZ_H / 2, R = DZ_H / 2 - 2, px, py;
    char nb[6];
    cv_rect(cx - 22, DZ_Y - 1, 44, DZ_H + 2, strap);
    for (i = DZ_Y + 2; i < DZ_Y + DZ_H; i += 6) {
        cv_rect(cx - 20, i, 1, 3, GB_RGB(170, 70, 60));
        cv_rect(cx + 19, i, 1, 3, GB_RGB(170, 70, 60));
    }
    cv_rect(cx + R - 2, cy - 5, 7, 10, gold);
    fxg_disc(cx, cy, R, dgold);
    fxg_disc(cx, cy, R - 1, gold);
    for (i = 0; i < 360; i += 8) {
        dial_pt(cx, cy, R - 3, i, &px, &py);
        cv_rect(px, py, 2, 2, dgold);
    }
    fxg_disc(cx, cy, R - 7, dgold);
    fxg_disc(cx, cy, R - 8, dial);
    for (i = 0; i < len; i++) {
        int32_t on = hits && ((i + len - rot % len) % len) * hits % len < hits, sz = len > 32 ? 2 : len > 16 ? 3 : 4;
        dial_pt(cx, cy, R - 12, i * 360 / len, &px, &py);
        cv_rrect(px - sz, py - sz, 2 * sz + 1, 2 * sz + 1, 1, GB_RGB(120, 116, 108), dial);
        cv_rect(px - sz + 1, py - sz + 1, 2 * sz - 1, 2 * sz - 1, on ? GB_RGB(220, 30, 26) : GB_RGB(214, 210, 200));
    }
    cv_text_in(cx - 28, cy - 18, 24, &AF_S, "LEN", DZ_INK, dial);
    fmt_int(nb, len);
    cv_text_in(cx - 28, cy - 7, 24, &AF_S, nb, DZ_INK, dial);
    cv_text_in(cx + 4, cy - 18, 24, &AF_S, "ROT", DZ_INK, dial);
    fmt_int(nb, rot % len);
    cv_text_in(cx + 4, cy - 7, 24, &AF_S, nb, DZ_INK, dial);
    cv_text_in(cx - 13, cy + 5, 26, &AF_S, "HITS", DZ_INK, dial);
    fmt_int(nb, hits);
    cv_rrect(cx - 9, cy + 17, 18, 14, 2, DZ_INK, dial);
    cv_rect(cx - 8, cy + 18, 16, 12, dial);
    cv_text_in(cx - 9, cy + 17, 18, &AF_S, nb, DZ_INK, dial);
    {
        int32_t st = v->sq_pos ? ((int32_t)v->sq_at + (int32_t)v->sq_pos - 1) % len : 0;
        dial_pt(cx, cy, R - 14, st * 360 / len, &px, &py);
        cv_line_t(cx, cy, px, py, GB_RGB(220, 30, 26), 2);
    }
    fxg_disc(cx, cy, 3, gold);
    (void)bgc;
}

static void dz_keys(int32_t x, int32_t w, int32_t y0, int32_t rh, uint32_t lo, uint32_t hi)
{
    uint32_t k, rows = hi - lo + 1u;
    cv_rect(x - 1, y0 - 1, w + 2, rh * (int32_t)rows + 2, GB_W);
    cv_rect(x, y0, w, rh * (int32_t)rows, GB_RGB(248, 246, 240));
    for (k = 0; k < rows; k++) {
        uint32_t note = hi - k, pc = note % 12u;
        int32_t y = y0 + (int32_t)k * rh;
        int bl = pc == 1u || pc == 3u || pc == 6u || pc == 8u || pc == 10u;
        if (bl)
            cv_rect(x, y, w * 3 / 5, rh, GB_W);
        else if (pc == 4u || pc == 11u)
            cv_rect(x, y + rh - 1, w, 1, GB_RGB(150, 150, 146));
    }
}
static void dz_grid(int32_t x0, int32_t sw, int32_t y0, int32_t rh, uint32_t lo, uint32_t hi, uint32_t len)
{
    uint32_t k, i, rows = hi - lo + 1u;
    int32_t y1 = y0 + rh * (int32_t)rows;
    for (k = 0; k < rows; k++) {
        uint32_t pc = (hi - k) % 12u;
        int bl = pc == 1u || pc == 3u || pc == 6u || pc == 8u || pc == 10u;
        cv_rect(x0, y0 + (int32_t)k * rh, 16 * sw, rh, bl ? GB_RGB(204, 202, 194) : GB_RGB(226, 224, 216));
    }
    for (i = 0; i <= 16u; i++)
        cv_rect(x0 + (int32_t)i * sw, y0, (i & 3u) ? 1 : 2, y1 - y0, (i & 3u) ? GB_RGB(176, 174, 166) : GB_RGB(110, 110, 104));
    if (len < 16u)
        cv_rect(x0 + (int32_t)len * sw + 2, y0, (16 - (int32_t)len) * sw - 2, y1 - y0, GB_RGB(236, 234, 228));
}
static void dz_roll_range(const gb_view_t *v, uint32_t *plo, uint32_t *phi, uint32_t *pn)
{
    uint32_t i, k, lo = 127u, hi = 0u, n = 0;
    for (i = 0; i < 16u; i++)
        for (k = 0; k < v->pr_n[i]; k++) {
            lo = v->pr_note[i][k] < lo ? v->pr_note[i][k] : lo;
            hi = v->pr_note[i][k] > hi ? v->pr_note[i][k] : hi;
            n++;
        }
    if (!n) {
        lo = 60u;
        hi = 71u;
    }
    lo = lo / 12u * 12u;
    hi = hi / 12u * 12u + 11u;
    if (hi > 127u)
        hi = 127u;
    while (hi - lo > 23u)
        hi -= 12u;
    *plo = lo;
    *phi = hi;
    *pn = n;
}
static void dz_note_name(char *b, uint32_t note)
{
    str_cpy(b, TL_NOTE[note % 12u], 4);
    fmt_int(b + str_len(b), (int32_t)note / 12 - 1);
}

static void dz_quickfill(const gb_view_t *v, uint16_t bgc)
{
    static const char *const TY[5] = {"ROLL", "RISE", "BURST", "GHOST", "TRIP"};
    uint32_t lo, hi, n, i, k, len = v->pr_len, rows;
    int32_t x0 = 36, sw = 12, y0 = DZ_Y + 14, rh, f0, fn, base = v->sq_at;
    uint16_t c = gb_col[v->sel % NTRK];
    char t[16];
    dz_roll_range(v, &lo, &hi, &n);
    rows = hi - lo + 1u;
    rh = (DZ_H - 16) / (int32_t)rows;
    rh = rh < 2 ? 2 : rh > 9 ? 9 : rh;
    dz_note_name(t, hi + 1u > 127u ? hi - 11u : hi + 1u);
    cv_text_on(10, DZ_Y, &AF_S, t, GB_W, bgc);
    for (i = 0; i < 16u; i += 4u) {
        char nb[4];
        fmt_int(nb, base + (int32_t)i + 1);
        cv_text_on(x0 + (int32_t)i * sw + 2, DZ_Y, &AF_S, nb, GB_W, bgc);
    }
    {
        char tb[16] = "TYPE ";
        str_cpy(tb + 5, TY[v->qf_type % 5u], sizeof tb - 5u);
        cv_text_r(230, DZ_Y, &AF_S, tb, T_THEME, bgc);
    }
    dz_keys(12, 22, y0, rh, lo, hi);
    dz_grid(x0, sw, y0, rh, lo, hi, len);
    f0 = v->qf_at ? (int32_t)v->qf_at - 1 - base : (int32_t)v->sq_len >= 4 ? (int32_t)v->sq_len - 4 : 0;
    fn = v->qf_at ? (int32_t)v->qf_n : (int32_t)(v->sq_len < 4u ? v->sq_len : 4u);
    {
        int32_t a = f0 < 0 ? 0 : f0, b = f0 + fn > 16 ? 16 : f0 + fn;
        if (b > a)
            cv_rect(x0 + a * sw + 2, y0, (b - a) * sw - 2, rh * (int32_t)rows, GB_RGB(240, 196, 192));
    }
    for (i = 0; i < 16u && i < len; i++) {
        uint32_t e = i + 1u;
        int fl = (int32_t)i >= f0 && (int32_t)i < f0 + fn;
        while (e < 16u && e < len && ((v->pr_tie >> e) & 1u))
            e++;
        for (k = 0; k < v->pr_n[i]; k++) {
            int32_t y = y0 + (int32_t)(hi - v->pr_note[i][k]) * rh, x = x0 + (int32_t)i * sw + 2, w = (int32_t)(e - i) * sw - 3;
            int sel = v->pr_sel == i + 1u && ((v->pr_selm >> k) & 1u);
            if (v->pr_note[i][k] < lo || v->pr_note[i][k] > hi)
                continue;
            if (sel) {
                cv_rect(x - 2, y - 2, w + 4, rh + 3, GB_W);
                cv_rect(x - 1, y - 1, w + 2, rh + 1, GB_RGB(250, 250, 246));
            }
            cv_rect(x, y, w, rh > 3 ? rh - 1 : rh, fl ? T_THEME : c);
        }
    }
    if (len && fn && f0 < 16 && f0 + fn > 0) {
        char fb[16] = "FILL ";
        int32_t e = f0 + fn > 16 ? 16 : f0 + fn;
        fmt_int(fb + 5, base + f0 + 1);
        str_cpy(fb + str_len(fb), "-", sizeof fb - str_len(fb));
        fmt_int(fb + str_len(fb), base + f0 + fn);
        {
            int32_t tw = text_w(&AF_S, fb) + 6, tx = x0 + e * sw - tw;
            tx = tx < x0 ? x0 : tx;
            cv_rect(tx, y0, tw, 13, T_THEME);
            cv_text_on(tx + 3, y0, &AF_S, fb, T_SURF, T_THEME);
        }
    }
    if (v->pr_pos)
        cv_rect(x0 + ((int32_t)v->pr_pos - 1) * sw + 1, y0, 2, rh * (int32_t)rows, T_THEME);
    if (!n)
        cv_text_in(x0, y0 + rh * (int32_t)rows / 2 - 7, 16 * sw, &AF_S, "NO NOTES YET", T_MID, GB_RGB(226, 224, 216));
}

static void tl_overview(const gb_view_t *v, uint16_t bgc)
{
    uint32_t r, i;
    int32_t x0 = 62, sw = 10, y0 = TL_D0 + 14, rh = 13, pos = v->fx ? v->ov_fxpos : v->ov_pos[v->sel % NTRK];
    const uint16_t off = GB_RGB(196, 196, 190), edge = GB_RGB(128, 128, 124);
    char b[24];
    for (i = 0; i < 16u; i += 4u) {
        char nb[4];
        fmt_int(nb, (int32_t)i + 1);
        cv_text_on(x0 + (int32_t)i * sw + (int32_t)(i / 4u) * 3, TL_D0 - 1, &AF_S, nb, GB_W, bgc);
    }
    for (r = 0; r <= NTRK; r++) {
        int32_t y = r < NTRK ? y0 + (int32_t)r * rh : y0 + (int32_t)NTRK * rh + 6;
        int mute = r < NTRK && ((v->tmute >> r) & 1u), fx = r == NTRK, me = fx ? v->fx : (!v->fx && r == v->sel);
        uint32_t len = fx ? v->ov_fxlen : v->ov_len[r], hit = fx ? v->ov_fx : v->ov_hit[r];
        uint16_t c = fx ? GB_RGB(120, 128, 140) : mute ? gb_mix(gb_col[r], 120) : gb_col[r];
        if (fx)
            cv_rect(12, y - 4, 216, 1, T_LINE);
        if (me)
            cv_rect(8, y, 3, 11, T_THEME);
        if (fx)
            str_cpy(b, "FX", sizeof b);
        else {
            const char *p = MACHINES[v->ov_m[r] % NMACH].pill;
            b[0] = 'T';
            b[1] = (char)('1' + r);
            b[2] = ' ';
            if (p[0] == '8')
                p++;
            else if (p[0] == 'F' && p[1] == 'M')
                p += 2;
            str_cpy(b + 3, p, 5);
        }
        cv_text_on(13, y - 1, &AF_S, b, fx ? T_MID : mute ? T_MID : GB_W, bgc);
        for (i = 0; i < 16u; i++) {
            int32_t x = x0 + (int32_t)i * sw + (int32_t)(i / 4u) * 3;
            if (i >= len) {
                cv_rect(x, y, sw - 1, 11, gb_mix(bgc, 12));
                continue;
            }
            cv_rect(x, y, sw - 1, 11, edge);
            cv_rect(x + 1, y + 1, sw - 3, 9, (hit >> i) & 1u ? c : off);
        }
    }
    if (pos) {
        int32_t x = x0 + (pos - 1) * sw + ((pos - 1) / 4) * 3 - 2, yt = y0 - 2, yb = y0 + (int32_t)NTRK * rh + 6 + 13;
        cv_rect(x, yt, sw + 3, 2, T_THEME);
        cv_rect(x, yb - 2, sw + 3, 2, T_THEME);
        cv_rect(x, yt, 2, yb - yt, T_THEME);
        cv_rect(x + sw + 1, yt, 2, yb - yt, T_THEME);
    }
    cv_rect(10, TL_CY - 2, 220, 1, T_LINE);
    str_cpy(b, "BAR ", sizeof b);
    str_cpy(b + 4, v->val[0], sizeof b - 4u);
    cv_text_on(10, TL_CY + 8, &AF_M, b, v->hot == 1u ? T_THEME : GB_W, bgc);
    b[0] = (char)('0' + v->ov_bar % 10u);
    b[1] = ' ';
    b[2] = '/';
    b[3] = ' ';
    b[4] = (char)('0' + (v->ov_nbar ? v->ov_nbar : 1u) % 10u);
    b[5] = 0;
    cv_text_in(104, TL_CY + 8, 50, &AF_M, b, GB_W, bgc);
    fmt_int(b, v->fx ? (int32_t)v->ov_fxlen : (int32_t)v->ov_len_all);
    str_cpy(b + str_len(b), " STEPS", sizeof b - str_len(b));
    cv_text_r(228, TL_CY + 10, &AF_S, b, T_MID, bgc);
    cv_rect(10, TL_FY - 4, 220, 1, T_LINE);
}

static void tl_pianoroll(const gb_view_t *v, uint16_t bgc)
{
    uint32_t i, k, lo, hi, n, rows;
    int32_t x0 = 36, sw = 12, y0 = TL_D0 + 14, rh, y1;
    char t[24];
    uint16_t c = gb_col[v->sel % NTRK];
    tl_status(v);
    tl_title(v, bgc);
    dz_roll_range(v, &lo, &hi, &n);
    rows = hi - lo + 1u;
    rh = (TL_CY - 20 - y0) / (int32_t)rows;
    rh = rh < 2 ? 2 : rh > 9 ? 9 : rh;
    y1 = y0 + rh * (int32_t)rows;
    dz_note_name(t, hi + 1u > 127u ? hi - 11u : hi + 1u);
    cv_text_on(10, TL_D0 - 1, &AF_S, t, GB_W, bgc);
    dz_note_name(t, lo);
    cv_text_on(10, y1 + 2, &AF_S, t, GB_W, bgc);
    for (i = 0; i < 16u; i += 4u) {
        char nb[4];
        fmt_int(nb, (int32_t)i + 1);
        cv_text_on(x0 + (int32_t)i * sw + 3, TL_D0 - 1, &AF_S, nb, GB_W, bgc);
    }
    cv_rect(x0, y0 - 3, 16 * sw, 1, T_MID);
    dz_keys(12, 20, y0, rh, lo, hi);
    dz_grid(x0, sw, y0, rh, lo, hi, v->pr_len);
    for (i = 0; i < 16u && i < v->pr_len; i++) {
        uint32_t e = i + 1u;
        while (e < 16u && e < v->pr_len && ((v->pr_tie >> e) & 1u))
            e++;
        for (k = 0; k < v->pr_n[i]; k++) {
            int32_t y = y0 + (int32_t)(hi - v->pr_note[i][k]) * rh, x = x0 + (int32_t)i * sw + 2, w = (int32_t)(e - i) * sw - 3;
            int sel = v->pr_sel == i + 1u && ((v->pr_selm >> k) & 1u);
            if (v->pr_note[i][k] < lo || v->pr_note[i][k] > hi)
                continue;
            if (sel) {
                cv_rect(x - 2, y - 2, w + 4, rh + 3, GB_W);
                cv_rect(x - 1, y - 1, w + 2, rh + 1, GB_RGB(250, 250, 246));
            }
            cv_rect(x, y, w, rh > 3 ? rh - 1 : rh, c);
        }
    }
    if (v->pr_pos)
        cv_rect(x0 + ((int32_t)v->pr_pos - 1) * sw + 1, y0 - 2, 2, y1 - y0 + 4, T_THEME);
    if (!n)
        cv_text_in(x0, (y0 + y1) / 2 - 8, 16 * sw, &AF_M, "NO NOTES", T_MID, GB_RGB(226, 224, 216));
    cv_rect(10, TL_CY - 2, 220, 1, T_LINE);
    str_cpy(t, "BAR ", sizeof t);
    str_cpy(t + 4, v->val[0], sizeof t - 4u);
    {
        char b[8] = {' ', ' ', (char)('0' + v->pr_bar % 10u), 0};
        str_cpy(t + str_len(t), b, sizeof t - str_len(t));
    }
    cv_text_on(10, TL_CY + 8, &AF_M, t, GB_W, bgc);
    if (v->pr_sel && v->pr_sel <= 16u && v->pr_n[v->pr_sel - 1u]) {
        uint32_t s = v->pr_sel - 1u, e = s + 1u, q;
        while (e < 16u && ((v->pr_tie >> e) & 1u))
            e++;
        t[0] = 'S';
        fmt_int(t + 1, (int32_t)s + 1);
        str_cpy(t + str_len(t), " ", sizeof t - str_len(t));
        if (v->pr_selm == 0x0Fu || v->pr_n[s] == 1u) {
            if (v->pr_n[s] > 1u)
                str_cpy(t + str_len(t), "CHORD", sizeof t - str_len(t));
            else
                dz_note_name(t + str_len(t), v->pr_note[s][0]);
        } else
            for (q = 0; q < 4u; q++)
                if ((v->pr_selm >> q) & 1u)
                    dz_note_name(t + str_len(t), v->pr_note[s][q]);
        str_cpy(t + str_len(t), " L", sizeof t - str_len(t));
        fmt_int(t + str_len(t), (int32_t)(e - s));
        cv_text_r(228, TL_CY + 10, &AF_S, t, T_THEME, bgc);
    } else if (n) {
        char a[6], b2[6];
        dz_note_name(a, lo);
        dz_note_name(b2, hi);
        str_cpy(t, a, sizeof t);
        str_cpy(t + str_len(t), "-", sizeof t - str_len(t));
        str_cpy(t + str_len(t), b2, sizeof t - str_len(t));
        cv_text_r(228, TL_CY + 10, &AF_S, t, T_MID, bgc);
    }
    cv_rect(10, TL_FY - 4, 220, 1, T_LINE);
    tl_footer(v, bgc);
}

static void dz_banks(const gb_view_t *v, uint16_t bgc)
{
    static const char L[5] = {'A', 'B', 'C', 'D', 'E'};
    const uint16_t pad = GB_RGB(236, 226, 204), dk = GB_RGB(36, 36, 34);
    int16_t *vp;
    const param_desc_t *d = page_desc(cur_page(), 1, &vp);
    uint32_t i, b = (uint32_t)v->cur[0] % 5u, s = (uint32_t)(v->cur[1] - (d ? d->min : 0)) % 8u;
    char t[24];
    for (i = 0; i < 5u; i++) {
        int32_t x = 26 + (int32_t)i * 39;
        char lb[2] = {L[i], 0};
        cv_rrect(x, DZ_Y - 1, 37, 15, 4, i == b ? T_THEME : GB_W, bgc);
        if (i != b)
            cv_rrect(x + 1, DZ_Y, 35, 13, 4, pad, GB_W);
        cv_text_in(x, DZ_Y - 1, 37, &AF_S, lb, i == b ? T_SURF : GB_W, i == b ? T_THEME : pad);
    }
    cv_rrect(14, DZ_Y + 16, 212, DZ_H - 16, 6, DZ_DGOLD, bgc);
    cv_rrect(16, DZ_Y + 18, 208, DZ_H - 20, 5, dk, DZ_DGOLD);
    for (i = 0; i < 4u; i++)
        cv_rect(20, DZ_Y + 22 + (int32_t)i * 3, 12, 1, GB_RGB(110, 110, 104));
    fxg_disc(26, DZ_Y + 38, 2, DZ_LED);
    fxg_disc(214, DZ_Y + 38, 2, DZ_LED);
    t[0] = L[b];
    t[1] = 0;
    str_cpy(t + 1, " / ", sizeof t - 1u);
    str_cpy(t + 4, d && d->names ? d->names[s] : "-", sizeof t - 4u);
    str_cpy(t + str_len(t), " / 01-08", sizeof t - str_len(t));
    cv_rrect(40, DZ_Y + 20, 160, 18, 4, DZ_GOLD, dk);
    cv_rrect(41, DZ_Y + 21, 158, 16, 4, GB_RGB(16, 16, 14), DZ_GOLD);
    cv_text_in(40, DZ_Y + 22, 160, &AF_S, t, GB_RGB(230, 210, 150), GB_RGB(16, 16, 14));
    for (i = 0; i < 8u && d && d->names; i++) {
        int32_t x = 24 + (int32_t)(i & 1u) * 98, y = DZ_Y + 42 + (int32_t)(i >> 1) * 15;
        int on = i == s;
        if (on)
            cv_rrect(x - 2, y - 1, 94, 15, 3, DZ_LED, dk);
        cv_rrect(x, y, 90, 13, 3, pad, on ? DZ_LED : dk);
        cv_text_in(x, y, 90, &AF_S, d->names[i], DZ_INK, pad);
    }
}

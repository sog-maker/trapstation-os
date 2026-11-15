/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION OS contributors */

static void d99_bootmenu(const gb_view_t *, uint16_t);
static void d99_save(const gb_view_t *, uint16_t);
#define TL_TOP 22
#define TL_D0 62
#define TL_D1 168
#define TL_CY 170
#define TL_FY 214
#define TL_SQX 80u
#define TL_SQS 18u
#define TL_SQY 220u
#define TL_SQW 16u

static int tl_on(const gb_view_t *v) { (void)v; return 1; }

static int32_t tl_sin(uint32_t ph)
{
    int32_t t = (int32_t)(ph & 511u), y = 4 * t * (512 - t), s;
    s = (int32_t)(((int64_t)y * 256 * 4) / (5 * 262144 - y));
    return ph & 512u ? -s : s;
}

#define TL_BPMX 222
static void tl_status(const gb_view_t *v)
{
    const uint16_t bg = T_ACCENT, ink = T_SURF;
    char tb[16], bb[8];
    int32_t x = 8, bw, rx;
    cv_rect(0, 0, 240, TL_TOP, bg);
    cv_rect(0, TL_TOP, 240, 3, gb_hc);
    if (v->icon) {
        if (v->hit) {
            cv_rrect(5, 2, 20, 18, 3, ink, bg);
            gb_icon(7, 3, 1, v->icon, bg);
        } else
            gb_icon(7, 3, 1, v->icon, ink);
        x = 30;
    }
    if (v->fx)
        str_cpy(tb, "FX", sizeof tb);
    else {
        tb[0] = 'T';
        tb[1] = (char)('1' + v->sel % NTRK);
        tb[2] = ' ';
        str_cpy(tb + 3, MACHINES[v->mach % NMACH].pill, sizeof tb - 3u);
    }
    cv_text_on(x, 4, &AF_S, tb, ink, bg);
    fmt_int(bb, v->bpm);
    bw = text_w(&AF_M, bb);
    if (v->bpm_alt) {
        cv_rrect(TL_BPMX + 2 - bw - 4, 2, bw + 6, 18, 3, ink, bg);
        cv_text_r(TL_BPMX, 2, &AF_M, bb, bg, ink);
    } else
        cv_text_r(TL_BPMX, 2, &AF_M, bb, ink, bg);
    rx = TL_BPMX - bw - 14;
    if (v->play) {
        int32_t i;
        for (i = 0; i < 9; i++)
            cv_rect(rx, 6 + i, i < 5 ? i + 1 : 9 - i, 1, ink);
    } else
        cv_rect(rx, 7, 7, 7, ink);
    if (v->rec) {
        int32_t rw = text_w(&AF_S, "REC") + 8;
        uint16_t rc = (uint16_t)((8u + 23u * (v->rec - 1u) / 15u) << 11);
        cv_rrect(rx - 8 - rw, 3, rw, 16, 3, rc, bg);
        cv_text_on(rx - 4 - rw, 4, &AF_S, "REC", 0xFFFFu, rc);
    }
}

static void tl_title(const gb_view_t *v, uint16_t bgc)
{
    const aafont_t *tf = large_face_has(v->title) && text_w(&AF_L, v->title) <= 218 ? &AF_L : &AF_M;
    cv_text_on(10, tf == &AF_L ? 27 : 34, tf, v->title, GB_W, bgc);
    cv_rect(10, 56, 26, 3, T_THEME);
}

static void tl_cols(const gb_view_t *v, uint16_t bgc)
{
    uint32_t k;
    for (k = 0; k < 4u; k++) {
        int32_t x = 12 + (int32_t)k * 56, w = 48, fw = (int32_t)v->ratio[k] * w / 1000;
        uint16_t ink = GB_W, mid = T_MID, bk = bgc;
        const aafont_t *vf;
        if (!(v->mask & (1u << k)))
            continue;
        if (v->alt[k]) {
            cv_rrect(x - 3, TL_CY, w + 6, 40, 4, GB_W, bgc);
            ink = mid = bgc;
            bk = GB_W;
        } else
            cv_rect(x, TL_CY, w, 1, T_LINE);
        cv_text_in(x - 4, TL_CY + 1, w + 8, &AF_S, v->lab[k], mid, bk);
        vf = text_w(&AF_M, v->val[k]) <= w + 6 ? &AF_M : &AF_S;
        if (v->vic[k])
            gb_icon(x + 8, TL_CY + 14, 2, v->vic[k], ink);
        else if (vf == &AF_S)
            cv_free_text(x, TL_CY + 17, &AF_S, v->val[k], ink, bk, w + 6);
        else
            cv_text_in(x - 4, TL_CY + 14, w + 8, vf, v->val[k], ink, bk);
        if (v->alt[k])
            continue;
        cv_rect(x, TL_CY + 34, w, 4, T_LINE);
        cv_rect(x, TL_CY + 34, fw < 0 ? 0 : fw > w ? w : fw, 4, v->hot == k + 1u ? T_THEME : GB_W);
        if (v->mixer && k == v->ring)
            cv_rect(x, TL_CY + 40, w, 2, GB_W);
        if (v->pk == k + 1u)
            cv_rect(x, TL_CY + 40, w, 2, T_THEME);
    }
}

static void tl_footer(const gb_view_t *v, uint16_t bgc)
{
    static const char *const KM[6] = {"", "SEQ", "PARAM", "KEY", "LOCKS", "ACID"};
    char kb[20], pb[8];
    uint32_t n = 0;
    cv_rect(10, TL_FY, 220, 1, T_LINE);
    str_cpy(kb, KM[v->kmode % 6u], sizeof kb);
    if (v->snp > 1u) {
        char sb[6] = {' ', (char)('1' + v->spg), '/', (char)('0' + v->snp), 0, 0};
        str_cpy(kb + str_len(kb), sb, sizeof kb - str_len(kb));
    }
    if (v->kmode == 2u && v->pname[0]) {
        str_cpy(kb + str_len(kb), " ", sizeof kb - str_len(kb));
        str_cpy(kb + str_len(kb), v->pname, sizeof kb - str_len(kb));
    }
    cv_text_on(10, text_w(&AF_M, kb) <= (int32_t)TL_SQX - 16 ? 219 : 221, text_w(&AF_M, kb) <= (int32_t)TL_SQX - 16 ? &AF_M : &AF_S,
               kb, GB_W, bgc);
    if (v->npages > 1u) {
        uint32_t a = v->page + 1u, b = v->npages;
        if (a >= 10u)
            pb[n++] = (char)('0' + a / 10u % 10u);
        pb[n++] = (char)('0' + a % 10u);
        pb[n++] = '/';
        if (b >= 10u)
            pb[n++] = (char)('0' + b / 10u % 10u);
        pb[n++] = (char)('0' + b % 10u);
        pb[n] = 0;
        cv_text_r(226, 221, &AF_S, pb, T_MID, bgc);
    }
}

static void tl_base(uint16_t bgc)
{
    (void)bgc;
    cv_rect(12, TL_D1 - 2, 216, 1, T_LINE);
}

static void tl_env(const gb_view_t *v, uint16_t bgc)
{
    int32_t x0 = 14, yb = TL_D1 - 2, yt = TL_D0 + 4, h = yb - yt, ad = (v->mask & 12u) == 0u;
    int32_t wa = 6 + v->ratio[0] * 44 / 1000, wd = 8 + v->ratio[1] * 56 / 1000, ys, wr, x1, x2, x3;
    tl_base(bgc);
    ys = ad ? yb : yb - (int32_t)v->ratio[2] * h / 1000;
    wr = ad ? 0 : 8 + v->ratio[3] * 56 / 1000;
    x1 = x0 + wa;
    x2 = x1 + wd;
    x3 = ad ? x2 : 226 - wr;
    cv_line_t(x0, yb, x1, yt, T_THEME, 2);
    cv_line_t(x1, yt, x2, ys, T_THEME, 2);
    if (!ad) {
        cv_line_t(x2, ys, x3, ys, T_THEME, 2);
        cv_line_t(x3, ys, x3 + wr, yb, T_THEME, 2);
        cv_rect(x3, yt, 1, yb - yt, T_LINE);
    }
}

static uint32_t tl_wave(const char *s)
{
    if (s[0] == 'T')
        return 1;
    if ((s[0] == 'S' && s[1] == 'A') || s[0] == 'U')
        return 2;
    if (s[0] == 'D')
        return 3;
    if (s[0] == 'S' && s[1] == 'Q')
        return 4;
    if ((s[0] == 'S' && s[1] == '&') || s[0] == 'R' || (s[0] == 'S' && s[1] == 'H'))
        return 5;
    return 0;
}
static void tl_lfo(const gb_view_t *v, uint16_t bgc)
{
    uint32_t w = tl_wave(v->val[1]), cyc = 2u + (uint32_t)v->ratio[0] * 3u / 1000u, x;
    int32_t mid = (TL_D0 + TL_D1) / 2, amp, a = v->ratio[3] - 500, px = 0, py = 0;
    amp = a < 0 ? -a : a;
    amp = 8 + amp * 26 / 500;
    if (!(v->mask & 8u))
        amp = 30;
    cv_rect(12, mid, 216, 1, T_LINE);
    for (x = 0; x <= 214u; x += 2u) {
        uint32_t ph = x * cyc * 1024u / 214u;
        int32_t y, s;
        switch (w) {
        case 1: s = 0; break;
        case 2: s = (int32_t)(ph & 1023u) / 2 - 256; break;
        case 3: s = 256 - (int32_t)(ph & 1023u) / 2; break;
        case 4: s = ph & 512u ? -256 : 256; break;
        case 5: {
            uint32_t q = (ph >> 8) * 2654435761u;
            s = (int32_t)(q >> 23) - 256;
            break;
        }
        default: s = tl_sin(ph); break;
        }
        if (w == 1u) {
            int32_t t = (int32_t)((ph + 256u) & 1023u);
            s = t < 512 ? t - 256 : 768 - t;
        }
        y = mid - s * amp / 256;
        if (x)
            cv_line_t(12 + px, py, 12 + (int32_t)x, y, T_THEME, 2);
        px = (int32_t)x;
        py = y;
    }
}

static void tl_filter(const gb_view_t *v, uint16_t bgc)
{
    int32_t xc = 20 + v->ratio[0] * 196 / 1000, r = v->ratio[1], yb = TL_D1 - 2, H = 44, x, px = 0, py = 0;
    char t = v->val[2][0];
    uint32_t ty = t == 'L' ? 0u : t == 'B' ? 1u : t == 'H' ? 2u : (uint32_t)v->ratio[2] * 3u / 1001u;
    tl_base(bgc);
    for (x = 12; x <= 228; x += 2) {
        int32_t d = x - xc, h, b = d < 0 ? -d : d, y;
        if (ty == 0u)
            h = d <= 0 ? H : H - d * H / 46;
        else if (ty == 2u)
            h = d >= 0 ? H : H + d * H / 46;
        else
            h = H - b * H / 52;
        if (b < 16)
            h += r * 26 / 1000 * (16 - b) * (16 - b) / 256;
        h = h < 0 ? 0 : h > yb - TL_D0 ? yb - TL_D0 : h;
        y = yb - h;
        if (x > 12)
            cv_line_t(px, py, x, y, T_THEME, 2);
        px = x;
        py = y;
    }
    cv_rect(xc, TL_D0 + 2, 1, yb - TL_D0 - 2, T_LINE);
}

static void tl_fmbox(int32_t x, int32_t y, char c, int carrier, uint16_t bgc)
{
    char s[2] = {c, 0};
    if (carrier) {
        cv_rrect(x, y, 34, 18, 4, GB_W, bgc);
        cv_text_in(x, y + 1, 34, &AF_M, s, bgc, GB_W);
    } else {
        cv_rrect(x, y, 34, 18, 4, GB_W, bgc);
        cv_rrect(x + 2, y + 2, 30, 14, 3, bgc, GB_W);
        cv_text_in(x, y + 1, 34, &AF_M, s, GB_W, bgc);
    }
}
static void tl_fmalg(const gb_view_t *v, uint16_t bgc)
{

    static const uint8_t E[8] = {1 | 8, 1 | 4, 1 | 4 | 8, 1 | 2 | 8, 1 | 2 | 4 | 8, 16 | 1, 16 | 1 | 2, 16 | 1 | 8};
    uint32_t a = (uint32_t)v->cur[0] & 7u, e = E[a];
    int32_t ax = 58, bx = 148, yb = TL_D1 - 22, yc = e & 16u ? yb - 30 : yb - 34, xc = ax, xd = bx, yd = yc;
    if (e & 16u) {
        yd = yc - 28;
        xd = ax + 30;
    }
    if (e & 1u) cv_line_t(xc + 17, yc + 18, ax + 17, yb, GB_W, 2);
    if (e & 2u) cv_line_t(xc + 17, yc + 18, bx + 17, yb, GB_W, 2);
    if (e & 4u) cv_line_t(xd + 17, yd + 18, ax + 17, yb, GB_W, 2);
    if (e & 8u) cv_line_t(xd + 17, yd + 18, bx + 17, yb, GB_W, 2);
    if (e & 16u) cv_line_t(xd + 17, yd + 18, xc + 17, yc, GB_W, 2);
    cv_rect(ax + 17, yb + 18, 1, 6, T_THEME);
    cv_rect(bx + 17, yb + 18, 1, 6, T_THEME);
    cv_rect(ax + 17, yb + 23, bx - ax + 1, 2, T_THEME);
    tl_fmbox(ax, yb, 'A', 1, bgc);
    tl_fmbox(bx, yb, 'B', 1, bgc);
    tl_fmbox(xc, yc, 'C', 0, bgc);
    tl_fmbox(xd, yd, 'D', 0, bgc);
    cv_rect(xd + 34, yd + 8, 9, 2, GB_W);
    cv_rect(xd + 41, yd - 6, 2, 16, GB_W);
    cv_rect(xd + 25, yd - 6, 18, 2, GB_W);
    cv_rect(xd + 25, yd - 6, 2, 6, GB_W);
    cv_text_on(196, TL_D0 + 2, &AF_S, "4-OP", T_MID, bgc);
}

static void tl_osc(const gb_view_t *v, uint16_t bgc, int drum)
{
    int32_t mid = (TL_D0 + TL_D1) / 2, x, px = 0, py = mid;
    uint32_t w = drum ? 0u : tl_wave(v->val[0]);
    int32_t dec = 30 + v->ratio[1] * 170 / 1000;
    cv_rect(12, mid, 216, 1, T_LINE);
    for (x = 0; x <= 214; x += 2) {
        uint32_t ph = drum ? (uint32_t)(x * 1024 * (6 + x / 40) / 214) : (uint32_t)x * 2048u / 214u;
        int32_t s, a = 30, y;
        if (drum) {
            a = 34 * dec / (dec + x);
            s = tl_sin(ph);
        } else if (w == 2u || w == 3u)
            s = w == 2u ? (int32_t)(ph & 1023u) / 2 - 256 : 256 - (int32_t)(ph & 1023u) / 2;
        else if (w == 4u)
            s = ph & 512u ? -256 : 256;
        else if (w == 1u) {
            int32_t t = (int32_t)((ph + 256u) & 1023u);
            s = t < 512 ? t - 256 : 768 - t;
        } else
            s = tl_sin(ph);
        y = mid - s * a / 256;
        if (x)
            cv_line_t(12 + px, py, 12 + x, y, T_THEME, 2);
        px = x;
        py = y;
    }
}

static void tl_sample(const gb_view_t *v, uint16_t bgc)
{
    const uint16_t c = GB_W, dim = gb_mix(bgc, 70);
    int32_t x, y0 = TL_D0 + 2, hh = TL_D1 - TL_D0 - 6;
    if (v->wv == 1u) {
        cv_text_in(0, (TL_D0 + TL_D1) / 2 - 8, 240, &AF_M, "NO SAMPLE", c, bgc);
        return;
    }
    if (v->zoom) {
        uint32_t sl = v->wkey & 63u, n = sl < smp_n ? smp_tab[sl].len : 0u, w = n / 6u, f = v->zoom == 1u ? v->za : v->zb;
        int32_t x0, ma, mb, wa;
        const uint8_t *pk;
        w = w < GB_WAVE ? GB_WAVE : w;
        x0 = (int32_t)f - (int32_t)(w / 2u);
        pk = smp_zpeaks(sl, x0, w);
        ma = ((int32_t)v->za - x0) * (int32_t)GB_WAVE / (int32_t)w;
        mb = ((int32_t)v->zb - x0) * (int32_t)GB_WAVE / (int32_t)w;
        for (x = 0; pk && x < (int32_t)GB_WAVE; x++) {
            int32_t h2 = (int32_t)pk[x] * hh / 512;
            h2 = h2 < 1 ? 1 : h2;
            cv_rect(8 + x, y0 + hh / 2 - h2, 1, h2 * 2, x >= ma && x <= mb ? c : dim);
        }
        for (wa = 0; wa < 2; wa++) {
            int32_t mx = wa ? mb : ma, dx = wa ? -4 : 1, bw = wa + 1 == v->zoom ? 3 : 1;
            if (mx < 0 || mx >= (int32_t)GB_WAVE)
                continue;
            cv_rect(8 + mx - bw / 2, y0 - 2, bw, hh + 4, T_THEME);
            cv_rect(8 + mx + dx, y0 - 2, 4, 2, T_THEME);
            cv_rect(8 + mx + dx, y0 + hh, 4, 2, T_THEME);
        }
        return;
    }
    if (v->wv == 2u) {
        const uint8_t *pk = smp_peaks(v->wkey & 63u);
        for (x = 0; pk && x < (int32_t)GB_WAVE; x++) {
            int32_t h2 = (int32_t)pk[x] * hh / 512;
            h2 = h2 < 1 ? 1 : h2;
            cv_rect(8 + x, y0 + hh / 2 - h2, 1, h2 * 2, x >= v->wa && x <= v->wb ? c : dim);
        }
        cv_rect(8 + v->wa, y0 - 2, 1, hh + 4, T_THEME);
        cv_rect(8 + v->wb, y0 - 2, 1, hh + 4, T_THEME);
        cv_rect(8 + v->wa, y0 - 2, 4, 2, T_THEME);
        cv_rect(8 + v->wb - 3, y0 - 2, 4, 2, T_THEME);
    }
}

static void tl_clock(const gb_view_t *v, uint16_t bgc)
{
    uint32_t i;
    int32_t sw = v->ratio[1] * 6 / 1000;
    tl_base(bgc);
    for (i = 0; i < 16u; i++) {
        int32_t x = 18 + (int32_t)i * 13 + ((i & 1u) ? sw : 0), h = (i & 3u) == 0u ? 50 : (i & 1u) ? 20 : 30;
        cv_rect(x, TL_D1 - 2 - h, 4, h, (i & 1u) && sw ? T_THEME : GB_W);
    }
}

static void tl_fxg(const gb_view_t *v, uint16_t bgc)
{
    int32_t oy = cv_oy;
    cv_oy += 12;
    if (v->fxg == 1u)
        fxg_cassette(v, bgc);
    else
        fxg_vinyl(v, bgc);
    cv_oy = oy;
}

static int32_t tl_r(const gb_view_t *v, uint32_t k) { return v->mask & (1u << k) ? v->ratio[k] : 0; }
static int tl_lab(const gb_view_t *v, uint32_t k, const char *s) { return (v->mask & (1u << k)) && str_eq(v->lab[k], s); }
static void tl_poly(int32_t *py, int32_t x, int32_t y, uint16_t c)
{
    if (x > 12)
        cv_line_t(x - 2, *py, x, y, c, 2);
    *py = y;
}

static void tl_volpan(const gb_view_t *v, uint16_t bgc)
{
    int32_t h = tl_r(v, 0) * (TL_D1 - TL_D0 - 8) / 1000, px = 70 + tl_r(v, 1) * 148 / 1000, my = (TL_D0 + TL_D1) / 2 + 6;
    cv_rect(14, TL_D0 + 4, 22, TL_D1 - TL_D0 - 6, T_LINE);
    cv_rect(14, TL_D1 - 2 - h, 22, h, GB_W);
    cv_rect(70, my, 148, 2, GB_W);
    cv_rect(143, my - 8, 2, 18, T_LINE);
    cv_text_on(60, my - 24, &AF_S, "L", T_MID, bgc);
    cv_text_r(226, my - 24, &AF_S, "R", T_MID, bgc);
    cv_rrect(px - 5, my - 10, 11, 22, 3, T_THEME, bgc);
}

static void tl_envdest(const gb_view_t *v, uint16_t bgc)
{
    int32_t y = (TL_D0 + TL_D1) / 2 - 12, a = tl_r(v, 1) - 500, on = a > 8 || a < -8;
    uint16_t c = on ? T_THEME : T_LINE;
    cv_rrect(16, y, 70, 26, 4, GB_W, bgc);
    cv_text_in(16, y + 4, 70, &AF_M, "ENV", bgc, GB_W);
    cv_rrect(150, y, 76, 26, 4, GB_W, bgc);
    cv_rrect(152, y + 2, 72, 22, 3, bgc, GB_W);
    cv_text_in(150, y + 4, 76, &AF_M, v->val[0], GB_W, bgc);
    cv_line_t(92, y + 13, 140, y + 13, c, 3);
    cv_line_t(140, y + 13, 132, y + 5, c, 3);
    cv_line_t(140, y + 13, 132, y + 21, c, 3);
    if (on)
        cv_text_in(92, y + 24, 48, &AF_S, a < 0 ? "-" : "+", T_THEME, bgc);
}

static void tl_sync(const gb_view_t *v, uint16_t bgc)
{
    int32_t mid = (TL_D0 + TL_D1) / 2, py = mid, x, uni = v->val[2][0] == 'U', note = v->val[1][0] == 'N';
    int32_t fade = tl_r(v, 3) * 180 / 1000, synced = !(v->val[0][0] == 'O' && v->val[0][1] == 'F');
    (void)bgc;
    if (synced)
        for (x = 12; x <= 226; x += 27)
            cv_rect(x, TL_D0 + 2, 1, TL_D1 - TL_D0 - 4, T_LINE);
    cv_rect(12, uni ? TL_D1 - 4 : mid, 216, 1, T_LINE);
    for (x = 12; x <= 226; x += 2) {
        uint32_t ph = (uint32_t)(x - 12) * 3072u / 214u;
        int32_t a = 30, s, y;
        if (note && x >= 120)
            ph = (uint32_t)(x - 120) * 3072u / 214u;
        if (fade && x - 12 < fade)
            a = a * (x - 12) / fade;
        s = tl_sin(ph);
        y = uni ? TL_D1 - 4 - (s + 256) * a / 256 : mid - s * a / 256;
        tl_poly(&py, x, y, T_THEME);
    }
    if (note)
        cv_rect(120, TL_D0 + 2, 2, TL_D1 - TL_D0 - 4, GB_W);
}

static void tl_sends(const gb_view_t *v, uint16_t bgc)
{
    uint32_t k;
    for (k = 0; k < 2u; k++) {
        int32_t y = TL_D0 + 12 + (int32_t)k * 34;
        cv_text_on(14, y, &AF_M, k ? "REV" : "DLY", GB_W, bgc);
        cv_rect(60, y + 2, 166, 14, T_LINE);
        cv_rect(60, y + 2, tl_r(v, k) * 166 / 1000, 14, T_THEME);
    }
}

static void tl_delay(const gb_view_t *v, uint16_t bgc)
{
    int32_t gap = 10 + tl_r(v, 0) * 30 / 1000, fb = 200 + tl_r(v, 1) * 750 / 1000, h = TL_D1 - TL_D0 - 8, x = 14, w;
    w = tl_r(v, 2) > 600 ? 2 : tl_r(v, 2) < 400 ? 6 : 4;
    tl_base(bgc);
    cv_rect(x, TL_D1 - 2 - h, 6, h, GB_W);
    for (x += gap; x < 224 && h > 2; x += gap) {
        h = h * fb / 1000;
        cv_rect(x, TL_D1 - 2 - h, w, h, T_THEME);
    }
}

static void tl_verb(const gb_view_t *v, uint16_t bgc)
{
    int32_t x, len = 40 + tl_r(v, 0) * 176 / 1000, d = tl_r(v, 1), H = TL_D1 - TL_D0 - 8, fz = v->val[0][0] == 'F';
    tl_base(bgc);
    for (x = 14; x < 226; x += 4) {
        int32_t t = x - 14, h, j = (int32_t)(((uint32_t)x * 2654435761u) >> 28);
        if (fz)
            h = H * 3 / 5 + j;
        else if (t > len)
            break;
        else
            h = H * (len - t) / len * (1000 - d * t / (len + 1)) / 1000 - j;
        if (h > 1)
            cv_rect(x, TL_D1 - 2 - h, 2, h, t < 8 ? GB_W : T_THEME);
    }
    if (fz)
        cv_text_r(226, TL_D0 + 2, &AF_S, "FREEZE", GB_W, bgc);
}

static void tl_osc2(const gb_view_t *v, uint16_t bgc, int mono)
{
    int32_t mid = (TL_D0 + TL_D1) / 2, py = mid, x, k;
    if (mono) {
        int32_t pw = 20 + tl_r(v, 2) * 60 / 1000, sub = 4 + tl_r(v, 1) * 18 / 1000, y0 = TL_D0 + 22;
        cv_rect(12, y0, 216, 1, T_LINE);
        py = y0;
        for (x = 12; x <= 226; x += 2) {
            int32_t ph = (x - 12) * 300 / 214 % 100;
            tl_poly(&py, x, ph < pw ? y0 - 16 : y0 + 16, T_THEME);
        }
        py = TL_D1 - 22;
        for (x = 12; x <= 226; x += 2)
            tl_poly(&py, x, TL_D1 - 22 - tl_sin((uint32_t)(x - 12) * 1536u / 214u) * sub / 256, GB_W);
        return;
    }
    if (tl_lab(v, 0, "SWEEP")) {
        int32_t sw = 6 + tl_r(v, 0) * 56 / 1000, st = 8 + tl_r(v, 1) * 120 / 1000;
        cv_rect(12, TL_D1 - 8, 216, 1, T_LINE);
        cv_text_on(14, TL_D0, &AF_S, "PITCH", T_MID, bgc);
        py = TL_D1 - 8 - sw;
        for (x = 12; x <= 226; x += 2)
            tl_poly(&py, x, TL_D1 - 8 - sw * st / (st + (x - 12)), T_THEME);
        return;
    }
    for (k = 0; k < 4; k++)
        if (tl_lab(v, (uint32_t)k, "DRIVE"))
            break;
    {
        int32_t g = 256 + (k < 4 ? tl_r(v, (uint32_t)k) : 0);
        cv_rect(12, mid, 216, 1, T_LINE);
        for (x = 12; x <= 226; x += 2) {
            int32_t s = tl_sin((uint32_t)(x - 12) * 2048u / 214u) * g / 256;
            s = s > 256 ? 256 : s < -256 ? -256 : s;
            tl_poly(&py, x, mid - s * 30 / 256, T_THEME);
        }
    }
}

static void tl_crush(const gb_view_t *v, uint16_t bgc)
{
    int32_t mid = (TL_D0 + TL_D1) / 2, x, py = mid, c = tl_r(v, 0), hold = 1 + c / 90, q = 3 + (1000 - c) * 30 / 1000;
    int32_t sub = tl_r(v, 1);
    (void)bgc;
    cv_rect(12, mid, 216, 1, T_LINE);
    for (x = 12; x <= 226; x += 2) {
        int32_t xs = (x - 12) / hold * hold, s = tl_sin((uint32_t)xs * 2048u / 214u);
        s = s * q / 256 * 256 / q;
        tl_poly(&py, x, mid - s * 30 / 256, T_THEME);
    }
    if (sub > 8)
        for (x = 12; x <= 226; x += 4)
            cv_rect(x, mid - tl_sin((uint32_t)(x - 12) * 1024u / 214u) * 30 * sub / 1000 / 256, 2, 2, GB_W);
}

static void tl_mod(const gb_view_t *v, uint16_t bgc, int d)
{
    int32_t x0 = 14, yb = TL_D1 - 2, top = yb - 6 - tl_r(v, 2) * (TL_D1 - TL_D0 - 12) / 1000;
    int32_t x1 = x0 + 6 + tl_r(v, 0) * 50 / 1000, x2 = x1 + 10 + tl_r(v, 1) * 150 / 1000;
    tl_base(bgc);
    if (x2 > 226)
        x2 = 226;
    cv_line_t(x0, yb, x1, top, T_THEME, 2);
    cv_line_t(x1, top, x2, yb, T_THEME, 2);
    cv_text_r(226, TL_D0 + 2, &AF_M, d ? "D" : "C", GB_W, bgc);
    if (d && tl_r(v, 3) > 8) {
        int32_t t = 1 + tl_r(v, 3) / 340;
        cv_rect(186, TL_D0 + 22, 18, t, GB_W);
        cv_rect(186, TL_D0 + 22, t, 12, GB_W);
        cv_rect(203, TL_D0 + 22, t, 12, GB_W);
        cv_text_r(182, TL_D0 + 22, &AF_S, "FB", T_MID, bgc);
    }
}

static void tl_tune(const gb_view_t *v, uint16_t bgc)
{
    int32_t mid = TL_D0 + 24, x, pa = mid, pb = mid, dt = tl_r(v, 0), px = 20 + tl_r(v, 1) * 200 / 1000, ys = TL_D1 - 12;
    (void)bgc;
    for (x = 12; x <= 226; x += 2) {
        uint32_t ph = (uint32_t)(x - 12) * 3072u / 214u;
        tl_poly(&pa, x, mid - tl_sin(ph) * 20 / 256, T_THEME);
        tl_poly(&pb, x, mid - tl_sin(ph + (uint32_t)((x - 12) * dt / 300)) * 20 / 256, GB_W);
    }
    cv_rect(20, ys, 200, 1, GB_W);
    for (x = 0; x <= 4; x++)
        cv_rect(20 + x * 50, ys - (x == 2 ? 6 : 3), 1, x == 2 ? 12 : 6, GB_W);
    cv_rrect(px - 4, ys - 8, 9, 16, 3, T_THEME, bgc);
}

static void tl_vhs(const gb_view_t *v, uint16_t bgc)
{
    int32_t mid = (TL_D0 + TL_D1) / 2, x, py = mid, w = 3 + tl_r(v, 0) * 26 / 1000, f = tl_r(v, 2) * 8 / 1000;
    uint32_t wc = 1u + (uint32_t)tl_r(v, 1) * 3u / 1000u, fc = 12u + (uint32_t)tl_r(v, 3) * 12u / 1000u;
    (void)bgc;
    cv_rect(12, mid, 216, 1, T_LINE);
    for (x = 12; x <= 226; x += 2) {
        uint32_t t = (uint32_t)(x - 12);
        tl_poly(&py, x, mid - tl_sin(t * wc * 1024u / 214u) * w / 256 - tl_sin(t * fc * 1024u / 214u) * f / 256, T_THEME);
    }
}

static void tl_comb(const gb_view_t *v, uint16_t bgc)
{
    int32_t x, py = TL_D1 - 24, n = 3 + tl_r(v, 1) * 9 / 1000, fb = tl_r(v, 2), mx = tl_r(v, 0), dm = tl_r(v, 3);
    tl_base(bgc);
    for (x = 12; x <= 226; x += 2) {
        int32_t c = tl_sin((uint32_t)(x - 12) * (uint32_t)n * 512u / 214u + 256u), a = c < 0 ? -c : c;
        a = a * a / 256;
        if (fb > 500)
            a = a * a / 256;
        a = a * (1000 - dm * (x - 12) / 260) / 1000;
        tl_poly(&py, x, TL_D1 - 4 - (20 + a * (30 + mx * 20 / 1000) / 256), T_THEME);
    }
}

static void tl_dj(const gb_view_t *v, uint16_t bgc)
{
    int32_t r = tl_r(v, 0), x, yb = TL_D1 - 2, H = 44, py = yb - H;
    int32_t lp = r < 480, hp = r > 520, xc = lp ? 20 + r * 400 / 1000 : hp ? 20 + (r - 500) * 400 / 1000 : 0;
    tl_base(bgc);
    for (x = 12; x <= 226; x += 2) {
        int32_t h = H;
        if (lp && x > xc)
            h = H - (x - xc) * H / 40;
        if (hp && x < xc)
            h = H - (xc - x) * H / 40;
        h = h < 0 ? 0 : h;
        tl_poly(&py, x, yb - h, T_THEME);
    }
    cv_text_in(0, TL_D0, 240, &AF_S, lp ? "LOW-PASS" : hp ? "HIGH-PASS" : "OPEN", T_MID, bgc);
}

static void tl_brake(const gb_view_t *v, uint16_t bgc)
{
    int32_t len = 40 + tl_r(v, 0) * 170 / 1000, x, H = TL_D1 - TL_D0 - 18, py = TL_D1 - 4 - H;
    tl_base(bgc);
    cv_text_on(14, TL_D0, &AF_S, "SPEED", T_MID, bgc);
    for (x = 12; x <= 226; x += 2) {
        int32_t t = x - 12, h = t >= len ? 0 : H * (len - t) / len * (len - t) / len;
        tl_poly(&py, x, TL_D1 - 4 - h, T_THEME);
    }
}

static int tl_is(const char *t, const char *a) { return str_eq(t, a); }
static int tl_pattern_page(const page_t *pg)
{
    const char *t = pg->title;
    return pg->graph == GR_GBSEQ || tl_is(t, "CHORD GEN") || tl_is(t, "CHORD VAR") || tl_is(t, "LOW ROLLER") ||
           tl_is(t, "GOLDEN CHORDS") || tl_is(t, "QUICK FILL") || tl_is(t, "EUCLID");
}

static void tl_anim_fill(gb_view_t *v);
static void tl_smp_fill(gb_view_t *v, const page_t *pg);
static uint32_t hg_target(void);
static void tl_fill(gb_view_t *v, const page_t *pg)
{
    const track_t *t = TSEL;
    uint32_t len = t->p[P_SLEN] ? (uint32_t)t->p[P_SLEN] : 1u, i, at = 0, idx = t->seq_idx % len;
    tl_anim_fill(v);
    if (tl_is(pg->title, "ACE HATS")) {
        uint32_t h = hg_target();
        v->hg_trk = (uint8_t)h;
        for (i = 0; h < NTRK && i < 16u; i++)
            if (trk[h].step[i].time == ST_NOTE && trk[h].step[i].n)
                v->sq_hit |= (uint16_t)(1u << i);
        return;
    }
    if (fxl_sel)
        return;
    if (eng_idx(TSEL->eng_req) == ENGI_SAMPLER && pg->scope == SC_ENGINE && (str_eq(pg->title, "OSC 1") || str_eq(pg->title, "OSC 2")))
        tl_smp_fill(v, pg);
    if (tl_is(pg->title, "PIANO ROLL") || tl_is(pg->title, "QUICK FILL")) {
        uint32_t bar = tl_is(pg->title, "QUICK FILL") ? (song.playing ? idx : len - 1u) / 16u
                                                      : ov_bar ? (uint32_t)ov_bar - 1u : song.playing ? idx / 16u : 0u;

        if (bar > (len - 1u) / 16u) {
            bar = (len - 1u) / 16u;
            if (!tl_is(pg->title, "QUICK FILL") && ov_bar) ov_bar = (uint8_t)(bar + 1u);
        }
        uint32_t s0 = bar * 16u, q;
        for (i = 0; i < NVOICE && v->pr_ln < 8u; i++)
            if (t->v[i].active && t->v[i].gate)
                v->pr_lv[v->pr_ln++] = t->v[i].note;
        v->pr_bar = (uint8_t)(bar + 1u);
        v->pr_sel = pr_sel;
        v->pr_selm = pr_selm;
        v->pr_len = (uint8_t)(len > s0 ? (len - s0 > 16u ? 16u : len - s0) : 0u);
        if (song.playing && idx >= s0 && idx < s0 + 16u)
            v->pr_pos = (uint8_t)(idx - s0 + 1u);
        for (i = 0; i < 16u && s0 + i < NSTEP; i++) {
            const step_t *s = &t->step[s0 + i];
            if (s->time == ST_NOTE && s->n) {
                v->pr_n[i] = (uint8_t)(s->n > 4u ? 4u : s->n);
                for (q = 0; q < v->pr_n[i]; q++)
                    v->pr_note[i][q] = s->note[q];
            } else if (s->time == ST_TIE)
                v->pr_tie |= (uint16_t)(1u << i);
        }
        if (!tl_is(pg->title, "QUICK FILL"))
            return;
        v->qf_type = (uint8_t)ph_fill;
    }
    if (tl_is(pg->title, "SCENES")) {
        for (i = 0; i < 4u; i++)
            v->sc_m[i] = (uint8_t)ph_masks[i];
        v->sc_live = (uint8_t)((song.playing ? 1u : 0u) | (fxl_mute ? 2u : 0u));
        return;
    }
    if (tl_is(pg->title, "CLOCK") && song.playing) {
        uint32_t bs = beat_samples();
        v->clk_beat = (uint8_t)(beat_n & 3u);
        v->clk_step = (uint8_t)(clk_step & 15u);
        v->clk_ph = (uint8_t)(((beat_n & 1u) * 16u + (beat_pos < bs ? beat_pos : bs) * 16u / (bs + 1u)) & 31u);
        return;
    }
    if (tl_is(pg->title, "SEQUENCER")) {
        uint32_t r, bar = ov_bar ? (uint32_t)ov_bar - 1u : song.playing ? idx / 16u : 0u, nb = 1u, fl = fxl_len(), s0 = bar * 16u;
        v->ov_bar = (uint8_t)(bar + 1u);
        for (r = 0; r < NTRK; r++) {
            uint32_t ln = trk[r].p[P_SLEN] ? (uint32_t)trk[r].p[P_SLEN] : 1u;
            v->ov_m[r] = (uint8_t)trk_mach(&trk[r]);
            nb = (ln + 15u) / 16u > nb ? (ln + 15u) / 16u : nb;
        }
        v->ov_nbar = (uint8_t)nb;
        v->ov_len_all = (uint8_t)len;
        v->ov_fxlen = (uint8_t)(fl > s0 ? (fl - s0 > 16u ? 16u : fl - s0) : 0u);
        for (i = 0; i < 16u && s0 + i < fl; i++)
            if ((fxl.trig >> (s0 + i)) & 1u)
                v->ov_fx |= (uint16_t)(1u << i);
        if (song.playing && fxl_pos >= s0 && fxl_pos < s0 + 16u)
            v->ov_fxpos = (uint8_t)(fxl_pos - s0 + 1u);
        for (r = 0; r < NTRK; r++) {
            const track_t *q = &trk[r];
            uint32_t ln = q->p[P_SLEN] ? (uint32_t)q->p[P_SLEN] : 1u, qi = q->seq_idx % ln, s0 = bar * 16u;
            v->ov_len[r] = (uint8_t)(ln > s0 ? (ln - s0 > 16u ? 16u : ln - s0) : 0u);
            if (song.playing && qi >= s0 && qi < s0 + 16u)
                v->ov_pos[r] = (uint8_t)(qi - s0 + 1u);
            for (i = 0; i < 16u && s0 + i < ln; i++)
                if (q->step[s0 + i].time == ST_NOTE && q->step[s0 + i].n)
                    v->ov_hit[r] |= (uint16_t)(1u << i);
        }
        return;
    }
    if (!tl_pattern_page(pg))
        return;
    at = pg->graph == GR_GBSEQ ? (uint32_t)ui.bank * 16u : tl_is(pg->title, "QUICK FILL") ? (song.playing ? idx : len - 1u) / 16u * 16u :
         song.playing ? idx / 16u * 16u : 0u;
    v->sq_len = (uint8_t)(len > at ? (len - at > 16u ? 16u : len - at) : 0u);
    v->sq_at = (uint8_t)at;
    if (song.playing && idx >= at && idx < at + 16u)
        v->sq_pos = (uint8_t)(idx - at + 1u);
    for (i = 0; i < 16u && at + i < NSTEP; i++) {
        const step_t *s = &t->step[at + i];
        uint32_t k, lo = 255u;
        if (s->time != ST_NOTE || !s->n)
            continue;
        v->sq_hit |= (uint16_t)(1u << i);
        if (s->flags & SF_ACCENT)
            v->sq_acc |= (uint16_t)(1u << i);
        for (k = 0; k < s->n && k < 4u; k++)
            lo = s->note[k] < lo ? s->note[k] : lo;
        v->sq_note[i] = (uint8_t)lo;
        v->sq_cn[i] = (uint8_t)(s->n > 4u ? 4u : s->n);
    }
}

static void tl_seq(const gb_view_t *v, uint16_t bgc)
{
    uint32_t i;
    for (i = 0; i < 16u; i++) {
        int32_t x = 14 + (int32_t)(i & 7u) * 27, y = TL_D0 + 4 + (int32_t)(i >> 3) * 36;
        int in = i < v->sq_len, hit = (v->sq_hit >> i) & 1u;
        if (!in)
            cv_rrect(x, y, 24, 28, 3, T_LINE, bgc);
        else if (hit)
            cv_rrect(x, y, 24, 28, 3, (v->sq_acc >> i) & 1u ? T_THEME : GB_W, bgc);
        else {
            cv_rrect(x, y, 24, 28, 3, GB_W, bgc);
            cv_rrect(x + 2, y + 2, 20, 24, 2, bgc, GB_W);
        }
        if (in && !(i & 3u) && !hit)
            cv_rect(x + 10, y + 12, 4, 4, T_LINE);
        if (v->sq_pos == i + 1u)
            cv_rect(x, y + 30, 24, 3, T_THEME);
    }
}

static void tl_roll(const gb_view_t *v, uint16_t bgc, int fill)
{
    uint32_t i, lo = 127u, hi = 0u, k;
    for (i = 0; i < 16u; i++)
        if ((v->sq_hit >> i) & 1u) {
            lo = v->sq_note[i] < lo ? v->sq_note[i] : lo;
            hi = v->sq_note[i] > hi ? v->sq_note[i] : hi;
        }
    tl_base(bgc);
    for (i = 0; i < 16u; i++) {
        int32_t x = 14 + (int32_t)i * 13;
        if (i >= v->sq_len)
            break;
        if (!(i & 3u))
            cv_rect(x, TL_D0 + 2, 1, TL_D1 - TL_D0 - 4, T_LINE);
        if (fill && v->sq_len >= 4u && i + 4u >= v->sq_len)
            cv_rect(x, TL_D1 - 6, 12, 3, T_THEME);
        if ((v->sq_hit >> i) & 1u) {
            int32_t y = TL_D1 - 14 - (hi > lo ? (int32_t)(v->sq_note[i] - lo) * 40 / (int32_t)(hi - lo) : 20);
            for (k = 0; k < v->sq_cn[i]; k++)
                cv_rect(x + 1, y - (int32_t)k * 5, 10, 3, k ? T_THEME : GB_W);
        }
        if (v->sq_pos == i + 1u)
            cv_rect(x, TL_D0 + 2, 12, 2, T_THEME);
    }
    if (!v->sq_hit)
        cv_text_in(0, (TL_D0 + TL_D1) / 2 - 8, 240, &AF_S, "NO NOTES YET", T_MID, bgc);
}

static void tl_euclid(const gb_view_t *v, uint16_t bgc)
{
    int32_t len = v->cur[0] < 1 ? 1 : v->cur[0] > 64 ? 64 : v->cur[0], hits = v->cur[1] < 0 ? 0 : v->cur[1] > len ? len : v->cur[1];
    int32_t rot = v->cur[2] < 0 ? 0 : v->cur[2], i, cx = 120, cy = (TL_D0 + TL_D1) / 2, r = 32, px, py;
    (void)bgc;
    for (i = 0; i < len; i++) {
        int32_t on = hits && ((i + len - rot % len) % len) * hits % len < hits;
        dial_pt(cx, cy, r, i * 360 / len, &px, &py);
        if (on)
            fxg_disc(px, py, len > 32 ? 3 : 4, i == 0 ? T_THEME : GB_W);
        else
            cv_rect(px - 1, py - 1, 3, 3, i == 0 ? T_THEME : T_LINE);
    }
    dial_pt(cx, cy, r - 10, 0, &px, &py);
    cv_line_t(cx, cy - r + 12, cx, cy - r + 6, T_THEME, 2);
}

static int32_t tl_keyx(uint32_t s, int *black)
{
    static const uint8_t W[12] = {0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6};
    static const uint8_t B[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
    uint32_t o = s / 12u, n = s % 12u;
    *black = B[n];
    return 15 + (int32_t)(o * 7u + W[n]) * 15 + (B[n] ? 15 : 7);
}
static void tl_keys(uint32_t mark, uint32_t root_mask, int chord, uint16_t bgc)
{
    uint32_t s;
    int bl;
    int32_t y0 = TL_D0 + 6, wh = 62, bh = 36;
    for (s = 0; s < 14u; s++) {
        cv_rect(15 + (int32_t)s * 15, y0, 15, wh, GB_W);
        cv_rect(16 + (int32_t)s * 15, y0 + 1, 13, wh - 2, T_SURF);
    }
    for (s = 0; s < 24u; s++) {
        int32_t x = tl_keyx(s, &bl);
        if (bl)
            cv_rect(x - 5, y0, 10, bh, GB_W);
    }
    for (s = 0; s < 24u; s++) {
        int32_t x = tl_keyx(s, &bl), y = bl ? y0 + bh - 12 : y0 + wh - 14;
        int on = (mark >> s) & 1u, rt = (root_mask >> s) & 1u;
        if (!on && !rt)
            continue;
        if (chord)
            fxg_disc(x, y, 5, rt ? (bl ? T_SURF : GB_W) : T_THEME);
        else
            fxg_disc(x, y, rt ? 5 : 3, rt ? T_THEME : (bl ? T_SURF : GB_W));
    }
    (void)bgc;
}
static void tl_chord(const gb_view_t *v, uint16_t bgc)
{
    int32_t root = v->cur[2] % 12, iv[CHORD_MAX + 1], r = root;
    uint32_t mode = (uint32_t)v->cur[0], sc = (uint32_t)v->cur[3], n = 0, i, m = 0;
    if (mode)
        n = chord_tones_cfg(mode, &r, iv, SCALE_MASK[sc % NELEM(SCALE_MASK)], root);
    r = (r % 12 + 12) % 12;
    for (i = 0; i < n; i++)
        m |= 1u << (uint32_t)((r + iv[i]) % 24);
    tl_keys(m, 1u << (uint32_t)r, 1, bgc);
}
static void tl_scale(const gb_view_t *v, uint16_t bgc)
{
    uint32_t root = (uint32_t)v->cur[0] % 12u, mask = SCALE_MASK[(uint32_t)v->cur[1] % NELEM(SCALE_MASK)], s, m = 0;
    for (s = 0; s < 24u; s++)
        if ((mask >> ((s + 12u - root) % 12u)) & 1u)
            m |= 1u << s;
    tl_keys(m, 1u << root | 1u << (root + 12u), 0, bgc);
}

static void tl_arp(const gb_view_t *v, uint16_t bgc)
{
    static const uint8_t ORD[6][8] = {{0, 1, 2, 3, 0, 1, 2, 3}, {3, 2, 1, 0, 3, 2, 1, 0}, {0, 1, 2, 3, 2, 1, 0, 1},
                                      {0, 1, 2, 3, 1, 2, 3, 0}, {2, 0, 3, 1, 1, 3, 0, 2}, {0, 2, 1, 3, 0, 2, 1, 3}};
    uint32_t k = (uint32_t)v->cur[1] % 6u, i, on = !(v->val[0][0] == 'O' && v->val[0][1] == 'F');
    int32_t px = 0, py = 0;
    tl_base(bgc);
    for (i = 0; i < 8u; i++) {
        int32_t x = 24 + (int32_t)i * 27, y = TL_D1 - 14 - (int32_t)ORD[k][i] * 16;
        if (i)
            cv_line(px, py, x, y, T_LINE);
        cv_rect(x - 6, y - 3, 12, 6, on ? T_THEME : T_LINE);
        px = x;
        py = y;
    }
    if (!on)
        cv_text_in(0, TL_D0 + 2, 240, &AF_S, "ARP OFF", T_MID, bgc);
}

static void tl_follow(const gb_view_t *v, uint16_t bgc)
{
    char me[4] = {'T', (char)('1' + v->sel % NTRK), 0, 0};
    int32_t y = (TL_D0 + TL_D1) / 2 - 13, off = v->val[0][0] == 'O';
    cv_rrect(16, y, 70, 26, 4, off ? T_LINE : GB_W, bgc);
    cv_rrect(18, y + 2, 66, 22, 3, bgc, off ? T_LINE : GB_W);
    cv_text_in(16, y + 4, 70, &AF_M, v->val[0], off ? T_MID : GB_W, bgc);
    cv_line_t(92, y + 13, 140, y + 13, off ? T_LINE : T_THEME, 3);
    cv_line_t(140, y + 13, 132, y + 5, off ? T_LINE : T_THEME, 3);
    cv_line_t(140, y + 13, 132, y + 21, off ? T_LINE : T_THEME, 3);
    cv_rrect(150, y, 76, 26, 4, GB_W, bgc);
    cv_text_in(150, y + 4, 76, &AF_M, me, bgc, GB_W);
    cv_text_in(92, y + 30, 48, &AF_S, "ROOT", T_MID, bgc);
}

static void tl_mix(const gb_view_t *v, uint16_t bgc)
{
    uint32_t j;
    const page_t *pg = cur_page();
    for (j = 0; j < 2u; j++) {
        uint32_t tk = (pg->id[2u * j] & ~MIX_PAN) % NTRK;
        int32_t x = 20 + (int32_t)j * 112, h = tl_r(v, 2u * j) * 60 / 1000, p = x + 30 + tl_r(v, 2u * j + 1u) * 56 / 1000;
        int mute = v->val[2u * j][0] == 'M' && v->val[2u * j][1] == 'U';
        cv_rect(x, TL_D0 + 6, 20, 62, T_LINE);
        cv_rect(x, TL_D0 + 68 - h, 20, h, mute ? T_LINE : gb_col[tk]);
        if (mute) {
            cv_line_t(x, TL_D0 + 6, x + 20, TL_D0 + 68, GB_W, 2);
            cv_line_t(x + 20, TL_D0 + 6, x, TL_D0 + 68, GB_W, 2);
        }
        cv_rect(x + 30, TL_D0 + 40, 56, 2, GB_W);
        cv_rect(x + 57, TL_D0 + 34, 1, 14, T_LINE);
        cv_rrect(p - 4, TL_D0 + 32, 9, 18, 3, T_THEME, bgc);
        {
            char nb[3] = {'T', (char)('1' + tk), 0};
            cv_text_on(x + 30, TL_D0 + 8, &AF_M, nb, GB_W, bgc);
        }
        if (tk == v->sel)
            cv_rect(x, TL_D0 + 2, 20, 2, T_THEME);
    }
}

static void tl_bank(const gb_view_t *v, uint16_t bgc)
{
    int16_t *vp;
    const param_desc_t *d = page_desc(cur_page(), 0, &vp);
    uint32_t i, n = d && d->names ? (uint32_t)(d->max - d->min + 1) : 0u;
    for (i = 0; i < n && i < 8u; i++) {
        int32_t x = 14 + (int32_t)(i & 1u) * 108, y = TL_D0 + 2 + (int32_t)(i >> 1) * 18;
        int on = (int32_t)i == v->cur[0] - d->min;
        if (on)
            cv_rrect(x - 2, y - 1, 104, 17, 3, GB_W, bgc);
        cv_text_on(x + 2, y + 1, &AF_S, d->names[i], on ? bgc : GB_W, on ? GB_W : bgc);
    }
}

static void tl_scenes(const gb_view_t *v, uint16_t bgc)
{
    uint32_t r, k, live = v->sc_live & 1u;
    for (r = 0; r < 5u; r++) {
        int32_t y = TL_D0 + 2 + (int32_t)r * 19, on = r < 4u && (int32_t)r == v->cur[0];
        uint32_t m = r < 4u ? v->sc_m[r] : ((~(uint32_t)v->tmute) & 63u) | ((v->sc_live & 2u) ? 64u : 0u);
        if (r == 4u && !live)
            break;
        if (r == 4u)
            cv_rect(12, y - 2, 216, 1, T_LINE);
        if (on)
            cv_rect(8, y + 1, 3, 14, T_THEME);
        cv_text_on(16, y + 1, &AF_S, r < 4u ? N_PHSCENE[r] : "LIVE", on || r == 4u ? GB_W : T_MID, bgc);
        for (k = 0; k <= NTRK; k++) {
            int32_t x = 90 + (int32_t)k * 20;
            int lit = k < NTRK ? (m >> k) & 1u : !((m >> 6) & 1u);
            uint16_t c = k < NTRK ? gb_col[k] : GB_RGB(120, 128, 140);
            if (lit)
                cv_rect(x, y + 1, 16, 14, c);
            else {
                cv_rect(x, y + 1, 16, 14, T_LINE);
                cv_rect(x + 1, y + 2, 14, 12, bgc);
            }
        }
    }
    for (k = 0; k <= NTRK; k++) {
        char nb[3] = {k < NTRK ? 'T' : 'F', k < NTRK ? (char)('1' + k) : 'X', 0};
        cv_text_in(88 + (int32_t)k * 20, TL_D1 - 13, 20, &AF_S, nb, T_MID, bgc);
    }
    cv_text_r(228, 38, &AF_S, live ? "KEYS: MUTE" : "KEYS: SCENE", T_THEME, bgc);
}

static void tl_acid(const gb_view_t *v, uint16_t bgc)
{
    const acid_t *a = acid_of(song.sel);
    uint32_t n = a->stages ? a->stages : 1u, sp = acid_sel[song.sel % NTRK] % ACID_N, i, p;
    int32_t w = 216 / (int32_t)(n < 8u ? 8u : n);
    (void)v;
    tl_base(bgc);
    for (i = 0; i < n && i < ACID_N; i++) {
        const acid_st_t *s = &a->st[i];
        int32_t x = 12 + (int32_t)i * w, h = 8 + ((int32_t)s->pitch + ((int32_t)s->oct - 3) * 4 + 12) * 40 / 36;
        h = h < 4 ? 4 : h > 56 ? 56 : h;
        cv_rect(x + 1, TL_D1 - 14 - h, w - 3, h, i == sp ? T_THEME : GB_W);
        for (p = 0; p < s->pulses && p < 8u; p++)
            cv_rect(x + 1 + (int32_t)p * 3, TL_D1 - 9, 2, 5, s->gate == AG_MUTE ? T_LINE : GB_W);
    }
}

static void tl_accent(const gb_view_t *v, uint16_t bgc)
{
    int32_t i, acc = v->val[0][0] == 'A', amt = tl_r(v, 1), sw = tl_r(v, 2) * 18 / 1000;
    tl_base(bgc);
    for (i = 0; i < 4; i++) {
        int32_t x = 30 + i * 52 + ((i & 1) ? sw : 0), h = 30, ac = (i == 0 || i == 2) && acc;
        if (ac)
            h += amt * 34 / 1000 + 4;
        cv_rect(x, TL_D1 - 2 - h, 16, h, ac ? T_THEME : GB_W);
    }
}

static void tl_rollfn(const gb_view_t *v, uint16_t bgc)
{
    int32_t n = v->val[0][0] == 'X' ? v->val[0][1] - '0' : 1, st = v->cur[1], i, w;
    int up = v->val[2][0] == 'U', dn = v->val[2][0] == 'D', last = v->val[3][0] == 'A';
    n = n < 1 ? 1 : n > 4 ? 4 : n;
    w = 200 / n;
    tl_base(bgc);
    cv_rect(20, TL_D0 + 4, 1, TL_D1 - TL_D0 - 6, T_LINE);
    cv_rect(220, TL_D0 + 4, 1, TL_D1 - TL_D0 - 6, T_LINE);
    for (i = 0; i < n; i++) {
        int32_t h = 20 + (up ? i * 8 : dn ? (n - 1 - i) * 8 : 6), off = st >= 0 ? st * i * 3 : -st * (n - 1 - i) * 3;
        int32_t y = TL_D1 - 2 - h - off;
        int a = last && i == n - 1;
        cv_rect(22 + i * w + 2, y, w - 6 < 8 ? 8 : w - 6, h, a ? T_THEME : GB_W);
    }
}

enum { TL_AN_NONE, TL_AN_BASS, TL_AN_JAZZ, TL_AN_TRAP, TL_AN_DICE, TL_AN_HAT };
#define TL_AN_MS_DICE 2200u
#define TL_AN_MS 1400u
#define TL_AN_STEP 50u
static uint8_t tl_an;
static uint32_t tl_an_t;
static const char *tl_an_msg;
static void tl_anim(uint32_t kind, const char *msg)
{
    tl_an = (uint8_t)kind;
    tl_an_t = fm1_ms;
    tl_an_msg = msg;
}
static const char *tl_an_goto;
static void tl_goto(const char *title);

#define TL_AN_READY 700u
static uint32_t tl_an_step(uint32_t k) { return k == TL_AN_HAT ? 60u : k == TL_AN_JAZZ ? 90u : k == TL_AN_BASS ? 100u : 80u; }
static uint32_t tl_an_ms(uint32_t k) { return k == TL_AN_DICE ? TL_AN_MS_DICE : 16u * tl_an_step(k) + TL_AN_READY; }
static void tl_anim_fill(gb_view_t *v)
{
    uint32_t e;
    if (!tl_an)
        return;
    e = fm1_ms - tl_an_t;
    if (e >= tl_an_ms(tl_an)) {
        tl_an = 0;
        if (tl_an_msg)
            ui_message(tl_an_msg);
        tl_an_msg = 0;
        if (tl_an_goto)
            tl_goto(tl_an_goto);
        tl_an_goto = 0;
        return;
    }
    v->an_k = tl_an;
    v->an_f = (uint8_t)(tl_an == TL_AN_DICE ? e / TL_AN_STEP + 1u : e < 16u * tl_an_step(tl_an) ? e / tl_an_step(tl_an) + 1u : 17u);
    v->bn = 0;
    v->ban = 0;
}

static const char *const TL_NOTE[12] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
static const char *const TL_NOTE_S[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static int tl_sharps(uint32_t root, uint32_t scale)
{
    static const int8_t REL[16] = {-1, 0, 3, 10, 5, 0, 3, 3, 8, 7, 1, 3, 3, -1, -1, -1};
    uint32_t m;
    if (REL[scale % 16u] < 0)
        return 0;
    m = (root + (uint32_t)REL[scale % 16u]) % 12u;
    return m == 7u || m == 2u || m == 9u || m == 4u || m == 11u;
}
static const char *tl_note(uint32_t pc, uint32_t root, uint32_t scale)
{
    return (tl_sharps(root, scale) ? TL_NOTE_S : TL_NOTE)[pc % 12u];
}

static void tl_die(int32_t x, int32_t y, int32_t s, uint32_t face, uint16_t bgc)
{
    static const uint16_t PIP[7] = {0, 0x10, 0x44, 0x54, 0x145, 0x155, 0x16D};
    uint32_t c;
    cv_rrect(x, y, s, s, 4, GB_W, bgc);
    cv_rrect(x + 1, y + 1, s - 2, s - 2, 3, GB_RGB(244, 238, 224), GB_W);
    for (c = 0; c < 9u; c++)
        if ((PIP[face % 7u] >> (8u - c)) & 1u)
            fxg_disc(x + s / 4 + (int32_t)(c % 3u) * s / 4, y + s / 4 + (int32_t)(c / 3u) * s / 4, s > 30 ? 3 : 2, GB_W);
}
static void tl_dice(const gb_view_t *v, uint16_t bgc, int32_t y0, int32_t h, uint32_t f)
{
    const uint16_t felt = GB_RGB(140, 26, 24), gold = GB_RGB(214, 170, 70);
    uint32_t var = (uint32_t)gen_var[song.sel % NTRK], i;
    int32_t r, s = h > 90 ? 40 : 28, top = y0 + h - s / 2 - 22;
    for (r = 0; r < s / 2 + 18; r++) {
        int32_t in = (s / 2 + 18 - r) / 3, yy = top + r;
        cv_rect(18 + in, yy, 204 - 2 * in, 1, r == 0 || r == s / 2 + 17 ? gold : felt);
        cv_rect(18 + in, yy, 2, 1, gold);
        cv_rect(220 - in, yy, 2, 1, gold);
    }
    for (i = 0; i < 3u; i++) {
        uint32_t face = (var * (3u + i * 2u) + i) % 6u + 1u;
        int32_t x = 120 - s / 2 + ((int32_t)i - 1) * (s + 16), y = top - s / 2 + 6;
        if (f && f < 22u) {
            int32_t up = (int32_t)(22u - f) * (h / 3) / 22, dx = tl_sin(f * 96u + i * 300u) * 8 / 256;
            face = (f * 5u + i * 2u) % 6u + 1u;
            x += dx;
            y -= up * (int32_t)((f + i) % 3u + 1u) / 3;
        }
        tl_die(x, y, s, face, bgc);
    }
    if (f >= 22u)
        cv_text_in(0, y0 + 10, 240, &AF_M, "NEUE VARIANTE", GB_W, bgc);
}

static void tl_vhs_shell(const gb_view_t *v, uint16_t bgc)
{
    static const uint16_t ST[4] = {GB_RGB(212, 72, 72), GB_RGB(80, 182, 186), GB_RGB(52, 104, 168), GB_RGB(98, 166, 96)};
    const uint16_t lab = GB_RGB(244, 238, 226);
    int32_t x0 = 12, y0 = TL_D0 + 2, w = 216, h = TL_D1 - TL_D0 - 6, y, i;
    (void)v;
    cv_rrect(x0, y0, w, h, 4, GB_W, bgc);
    cv_rrect(x0 + 2, y0 + 2, w - 4, h - 4, 3, lab, GB_W);
    for (y = y0 + 3; y < y0 + h - 3; y++)
        for (i = 0; i < 4; i++) {
            int32_t xs = x0 + 104 + (y0 + h - y) * 6 / 5 + i * 13;
            int32_t xe = xs + 13 > x0 + w - 3 ? x0 + w - 3 : xs + 13;
            if (xs < x0 + w - 3)
                cv_rect(xs, y, xe - xs, 1, ST[i]);
        }
    cv_rect(x0 + w - 10, y0 + h / 2 - 9, 8, 18, GB_W);
    cv_text_on(x0 + 8, y0 + 4, &AF_S, "TRAPSTATION", GB_W, lab);
    cv_text_on(x0 + 8, y0 + 18, &AF_L, "T-120", GB_W, lab);
    cv_rrect(x0 + 8, y0 + h - 20, 34, 16, 2, GB_W, lab);
    cv_rrect(x0 + 9, y0 + h - 19, 32, 14, 2, lab, GB_W);
    cv_text_in(x0 + 8, y0 + h - 19, 34, &AF_S, "VHS", GB_W, lab);
}

static void tl_scale1(const gb_view_t *v, uint16_t bgc)
{
    static const char *const LONG[16] = {"CHROMATIC", "MAJOR", "NATURAL MINOR", "DORIAN", "MIXOLYDIAN", "PENTATONIC",
                                         "MINOR PENTATONIC", "HARMONIC MINOR", "PHRYGIAN", "LYDIAN", "LOCRIAN",
                                         "MELODIC MINOR", "BLUES", "WHOLE TONE", "DIMINISHED HW", "DIMINISHED WH"};
    static const uint8_t WI[12] = {0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6}, BK[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
    uint32_t root = (uint32_t)v->cur[0] % 12u, sc = (uint32_t)v->cur[1] % NELEM(SCALE_MASK), mask = SCALE_MASK[sc], s, n = 0;
    int32_t kx = 18, kw = 29, ky = TL_D0 + 14, kh = 42, bw = 17, bh = 25;
    char t[24];
    str_cpy(t, tl_note(root, root, sc), sizeof t);
    str_cpy(t + str_len(t), " ", sizeof t - str_len(t));
    str_cpy(t + str_len(t), LONG[sc], sizeof t - str_len(t));
    cv_text_on(12, TL_D0, &AF_S, t, T_MID, bgc);
    cv_rect(kx - 1, ky - 1, 7 * kw + 2, kh + 2, GB_W);
    for (s = 0; s < 7u; s++)
        cv_rect(kx + (int32_t)s * kw + 1, ky, kw - 1, kh, GB_RGB(240, 232, 214));
    if (!BK[root])
        cv_rect(kx + (int32_t)WI[root] * kw + 1, ky + bh + 2, kw - 1, kh - bh - 2, T_THEME);
    for (s = 0; s < 12u; s++)
        if (BK[s]) {
            int32_t x = kx + ((int32_t)WI[s] + 1) * kw - bw / 2;
            cv_rect(x, ky, bw, bh, s == root ? T_THEME : GB_W);
        }
    for (s = 0; s < 12u; s++) {
        uint32_t in = (mask >> ((s + 12u - root) % 12u)) & 1u;
        if (!in || s == root)
            continue;
        if (BK[s])
            cv_rect(kx + ((int32_t)WI[s] + 1) * kw - 5, ky + bh - 8, 10, 3, T_THEME);
        else
            cv_rect(kx + (int32_t)WI[s] * kw + kw / 2 - 5, ky + kh - 10, 10, 3, T_THEME);
    }
    for (s = 0; s < 12u; s++)
        n += (mask >> s) & 1u;
    {
        int32_t step = n ? 216 / (int32_t)n : 0, x = 12;
        for (s = 0; s < 12u; s++) {
            uint32_t p = (root + s) % 12u;
            if (!((mask >> s) & 1u))
                continue;
            cv_text_in(x, ky + kh + 3, step, &AF_S, tl_note(p, root, sc), s ? GB_W : T_THEME, bgc);
            x += step;
        }
    }
}

static void tl_progression(const gb_view_t *v, uint16_t bgc)
{
    static const char *const SUF[CH_LAST + 1] = {"", "", "", "", "m", "7", "M7", "m7", "sus", "5", "m9", "m7b5", "M9", "m11", "m13",
                                                 "s2", "a9", "ma9"};
    static const char *const RN[7] = {"I", "II", "III", "IV", "V", "VI", "VII"};
    static const char *const RNL[7] = {"i", "ii", "iii", "iv", "v", "vi", "vii"};
    const track_t *t = TSEL;
    uint32_t k = song.sel % NTRK, b, len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP);
    gen_recipe_t r = {1, (uint8_t)chord_cfg[k][CG_STYLE], (uint8_t)chord_cfg[k][CG_RHYTHM], (uint8_t)(gen_var[k] - 1),
                      (int8_t)chord_cfg[k][CG_OCT], (uint8_t)t->p[P_ROOT], (uint8_t)t->p[P_SCALE], (uint8_t)t->p[P_CHRD],
                      (uint8_t)t->p[P_VOIC], (uint8_t)trk_vmode(t), (uint8_t)len, GEN_V2};
    uint32_t mask = gen_scale(&r), at = v->sq_pos ? gen_chord_at(&r, (uint32_t)v->sq_at + v->sq_pos - 1u) : 9u;
    if (chord_kit(t) || acid_on(t)) {
        cv_text_in(0, TL_D0 + 26, 240, &AF_S, "USE A SYNTH TRACK", T_MID, bgc);
        return;
    }
    cv_text_on(12, TL_D0, &AF_S, "PROGRESSION", T_MID, bgc);
    for (b = 0; b < 4u; b++) {
        uint32_t root = 0, mode = 0, pc, deg = 0, j, i = gen_chord_step(&r, b), minor;
        int32_t x = 12 + (int32_t)b * 55;
        char nm[10];
        if (i >= len || !gen_plan(&r, i, &root, &mode))
            continue;
        pc = root % 12u;
        str_cpy(nm, tl_note(pc, r.root, r.scale), sizeof nm);
        str_cpy(nm + str_len(nm), SUF[mode % (CH_LAST + 1u)], sizeof nm - str_len(nm));
        for (j = 0; j < ((pc + 12u - r.root % 12u) % 12u); j++)
            deg += (mask >> j) & 1u;
        minor = mode == CH_MIN || mode == CH_MIN7 || mode == CH_MIN9 || mode == CH_HALFDIM || mode == CH_MIN11 || mode == CH_MIN13 ||
                mode == CH_MADD9;
        if (b == at % 4u && at < 4u) {
            cv_rrect(x, TL_D0 + 18, 50, 26, 5, T_THEME, bgc);
            cv_text_in(x, TL_D0 + 22, 50, text_w(&AF_M, nm) <= 46 ? &AF_M : &AF_S, nm, GB_RGB(240, 232, 214), T_THEME);
        } else {
            cv_rrect(x, TL_D0 + 18, 50, 26, 5, GB_W, bgc);
            cv_rrect(x + 2, TL_D0 + 20, 46, 22, 4, bgc, GB_W);
            cv_text_in(x, TL_D0 + 22, 50, text_w(&AF_M, nm) <= 46 ? &AF_M : &AF_S, nm, GB_W, bgc);
        }
        if (b < 3u)
            cv_rect(x + 50, TL_D0 + 30, 5, 2, T_LINE);
        cv_text_in(x, TL_D0 + 50, 50, &AF_M, (minor ? RNL : RN)[deg % 7u], GB_W, bgc);
    }
}

#define TL_CARD T_ACCENT
#define TL_CARD_INK T_SURF
static void tl_card(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t bgc)
{
    cv_rrect(x, y, w, h, 8, TL_CARD, bgc);
    cv_rect(x + 4, y + 8, 4, h - 16, T_THEME);
}

static void tl_save(const gb_view_t *v, uint16_t bgc)
{
    d99_save(v, bgc);
}

static void tl_perf(const gb_view_t *v, uint16_t bgc)
{
    static const char *const FN[4] = {"1/16", "1/8", "BAR", "4BAR"};
    static const char *const RN[6] = {"1/8", "1/16", "1/32", "REV", "STOP", "1/64"};
    gb_view_t c = *v;
    uint32_t k;
    char nb[8];
    str_cpy(c.title, v->hn > 2u ? "ARP" : "PERF", sizeof c.title);
    tl_status(v);
    tl_title(&c, bgc);
    for (k = 0; k < 8u; k++) {
        int32_t x = 12 + (int32_t)(k & 3u) * 55, y = TL_D0 + (int32_t)(k >> 2) * 20, held = v->mac_held[k], w = 51;
        nb[0] = 'M'; nb[1] = (char)('1' + k); nb[2] = 0;
        cv_rrect(x, y, w, 18, 3, held ? T_THEME : GB_W, bgc);
        if (held) {
            int32_t f = (int32_t)v->fill[k] * (w - 4) / 26;
            cv_rect(x + 2, y + 2, w - 4, 14, T_THEME);
            cv_rect(x + 2 + f, y + 2, w - 4 - f > 0 ? w - 4 - f : 0, 14, gb_mix(T_THEME, 60));
            cv_text_in(x, y + 2, w, &AF_S, nb, T_SURF, T_THEME);
        } else {
            cv_rrect(x + 2, y + 2, w - 4, 14, 2, v->mac_n[k] ? gb_mix(bgc, 30) : bgc, GB_W);
            cv_text_in(x, y + 2, w, &AF_S, nb, GB_W, v->mac_n[k] ? gb_mix(bgc, 30) : bgc);
        }
    }
    {
        int32_t y = TL_D0 + 47, on = v->xf_a || v->xf_b, i;
        cv_text_on(12, y - 6, &AF_M, "A", GB_W, bgc);
        cv_text_r(228, y - 6, &AF_M, "B", GB_W, bgc);
        cv_rect(30, y, 180, 2, GB_W);
        for (i = 0; i <= 6; i++)
            cv_rect(30 + i * 30, y - 3, 1, 8, GB_W);
        cv_rect(30 + (on ? (int32_t)v->xf_x * 176 / 60 : 88), y - 6, 5, 14, on ? T_THEME : T_LINE);
    }
    cv_rect(12, TL_D0 + 56, 216, 1, T_LINE);
    for (k = 0; k < 4u; k++) {
        int32_t x = 12 + (int32_t)k * 55, on = v->fade == k + 1u;
        if (on)
            cv_rect(x + 2, TL_D0 + 58, 50, 15, T_THEME);
        cv_text_in(x, TL_D0 + 58, 54, &AF_S, FN[k], on ? T_SURF : GB_W, on ? T_THEME : bgc);
    }
    cv_rect(12, TL_D0 + 74, 216, 1, T_LINE);
    {
        static const uint8_t RB[3] = {0, 1, 2}, PB[3] = {3, 5, 4};
        static const char *const PN[3] = {"REV", "STUT", "STOP"};
        cv_text_in(12, TL_D0 + 76, 54, &AF_S, "REP", GB_W, bgc);
        for (k = 0; k < 3u; k++) {
            int32_t x = 67 + (int32_t)k * 54, on = (v->rpt >> RB[k]) & 1u;
            if (on)
                cv_rect(x + 2, TL_D0 + 76, 50, 15, T_THEME);
            cv_text_in(x, TL_D0 + 76, 54, &AF_S, RN[RB[k]], on ? T_SURF : GB_W, on ? T_THEME : bgc);
        }
        for (k = 0; k < 3u; k++) {
            int32_t x = 12 + (int32_t)k * 73, on = (v->rpt >> PB[k]) & 1u;
            if (on)
                cv_rect(x + 6, TL_D0 + 92, 62, 15, T_THEME);
            cv_text_in(x, TL_D0 + 92, 74, &AF_S, PN[k], on ? T_SURF : GB_W, on ? T_THEME : bgc);
        }
    }
    {
        uint32_t q, o = 0;
        char n2[8];
        c.mask = 15;
        str_cpy(c.lab[0], "FADE", sizeof c.lab[0]);
        str_cpy(c.lab[1], "FILT", sizeof c.lab[1]);
        str_cpy(c.lab[2], "COMB", sizeof c.lab[2]);
        str_cpy(c.lab[3], "TUNE", sizeof c.lab[3]);
        if (!v->rel)
            str_cpy(c.val[0], "-", sizeof c.val[0]);
        else if (v->rel >= 16u)
            str_cpy(c.val[0], "BAR", sizeof c.val[0]);
        else {
            uint32_t n = v->rel, d = 16u;
            while (!(n & 1u)) { n >>= 1; d >>= 1; }
            fmt_int(n2, (int32_t)n);
            for (q = 0; n2[q]; q++) c.val[0][o++] = n2[q];
            c.val[0][o++] = '/';
            fmt_int(n2, (int32_t)d);
            for (q = 0; n2[q]; q++) c.val[0][o++] = n2[q];
            c.val[0][o] = 0;
        }
        c.ratio[0] = (int16_t)(v->rel * 1000u / 16u);
        if (v->dj > 1 || v->dj < -1) {
            int32_t a2 = v->dj < 0 ? -v->dj : v->dj;
            c.val[1][0] = v->dj < 0 ? 'L' : 'H';
            c.val[1][1] = 'P';
            fmt_int(c.val[1] + 2, a2 * 100 / 64);
        } else
            str_cpy(c.val[1], "-", sizeof c.val[1]);
        c.ratio[1] = (int16_t)((v->dj + 64) * 1000 / 127);
        fmt_int(c.val[2], v->cmix * 100 / 127);
        c.ratio[2] = (int16_t)(v->cmix * 1000 / 127);
        fmt_int(c.val[3], v->ctune * 100 / 127);
        c.ratio[3] = (int16_t)(v->ctune * 1000 / 127);
        for (k = 0; k < 4u; k++)
            c.alt[k] = c.vic[k] = 0;
        c.hot = 0;
        tl_cols(&c, bgc);
    }
    tl_footer(v, bgc);
}

static void tl_smp_fill(gb_view_t *v, const page_t *pg)
{
    const page_t *o = 0;
    const smp_v_t *h = &smp_v[song.sel % NPART];
    uint32_t i, k, a = str_eq(pg->title, "OSC 1");
    for (i = 0; i < NPAGES; i++)
        if (str_eq(PAGES[i].title, a ? "OSC 2" : "OSC 1"))
            o = &PAGES[i];
    v->smp_row = (uint8_t)(a ? 1u : 2u);
    str_cpy(v->title, "SAMPLER", sizeof v->title);
    for (k = 0; o && k < 4u; k++) {
        int16_t *vp;
        const char *u;
        const param_desc_t *d = page_desc(o, k, &vp);
        if (!d || !vp || !d->label || d->label[0] == '-')
            continue;
        str_cpy(v->lab2[k], d->label, sizeof v->lab2[k]);
        param_format(d, *vp, v->val2[k], &u);
        if (vp >= TSEL->p && vp < TSEL->p + P_COUNT)
            v->vic2[k] = (uint8_t)smp_format((uint32_t)(vp - TSEL->p), *vp, v->val2[k]);
        v->mask2 |= (uint8_t)(1u << k);
    }
    if (h->owner && !h->done && h->gen == smp_gen && h->n && h->base) {
        int32_t at = (int32_t)(h->d - h->base) + h->dir * (h->cin >> 8);
        at = at < 0 ? 0 : at >= (int32_t)h->n ? (int32_t)h->n - 1 : at;
        v->wp = (uint8_t)(1u + (uint32_t)at * GB_WAVE / h->n);
    }
}
static const char *tl_smode(uint32_t ic)
{
    return ic == IC_LOOP ? "LOOP" : ic == IC_PING ? "PING" : ic == IC_REV ? "REV" : "ONCE";
}
static void tl_smp_row(const gb_view_t *v, int32_t y, int active, const char (*lab)[8], const char (*val)[12],
                       const uint8_t *vic, uint32_t mask, uint16_t bgc)
{
    uint32_t k;
    if (active)
        cv_rect(3, y + 2, 4, 40, T_THEME);
    for (k = 0; k < 4u; k++) {
        int32_t x = 12 + (int32_t)k * 56;
        const char *s = vic[k] ? tl_smode(vic[k]) : val[k];
        const aafont_t *f;
        if (!((mask >> k) & 1u))
            continue;
        cv_rect(x, y, 48, 1, T_LINE);
        cv_text_on(x, y + 3, &AF_S, lab[k], T_MID, bgc);
        f = large_face_has(s) && text_w(&AF_L, s) <= 52 ? &AF_L : text_w(&AF_M, s) <= 54 ? &AF_M : &AF_S;
        cv_text_on(x, f == &AF_L ? y + 15 : f == &AF_M ? y + 21 : y + 24, f, s, active && v->alt[k] ? T_THEME : GB_W, bgc);
        if (active && v->hot == k + 1u)
            cv_rect(x, y + 43, 44, 3, T_THEME);
    }
}
static void tl_sampler(const gb_view_t *v, uint16_t bgc)
{
    const uint16_t box = T_ACCENT, win = GB_RGB(76, 24, 22), out = GB_RGB(150, 150, 146), in = GB_RGB(240, 232, 214);
    int32_t x, y0 = 74, yc = 103, hh = 14, ma = v->wa, mb = v->wb;
    uint32_t sl = v->wkey & 63u, ic;
    const uint8_t *pk = 0;
    char t[24];
    tl_status(v);
    tl_title(v, bgc);
    str_cpy(t, "S", sizeof t);
    t[1] = (char)('0' + (sl + 1u) / 10u);
    t[2] = (char)('0' + (sl + 1u) % 10u);
    t[3] = 0;
    if (sl < smp_n && smp_tab[sl].name[0]) {
        str_cpy(t + 3, " / ", sizeof t - 3u);
        str_cpy(t + 6, smp_tab[sl].name, sizeof t - 6u);
    }
    cv_text_on(10, 60, &AF_S, v->wv == 1u ? "NO SAMPLE" : t, GB_W, bgc);
    ic = v->smp_row == 2u ? v->vic[0] : v->vic2[0];
    cv_text_r(230, 60, &AF_S, tl_smode(ic), GB_W, bgc);
    cv_rrect(6, y0, 228, 48, 4, box, bgc);
    if (v->wv == 2u && v->zoom) {
        uint32_t n = sl < smp_n ? smp_tab[sl].len : 0u, w = n / 6u, f = v->zoom == 1u ? v->za : v->zb;
        int32_t x0;
        w = w < GB_WAVE ? GB_WAVE : w;
        x0 = (int32_t)f - (int32_t)(w / 2u);
        pk = smp_zpeaks(sl, x0, w);
        ma = ((int32_t)v->za - x0) * (int32_t)GB_WAVE / (int32_t)w;
        mb = ((int32_t)v->zb - x0) * (int32_t)GB_WAVE / (int32_t)w;
    } else if (v->wv == 2u)
        pk = smp_peaks(sl);
    if (pk) {
        int32_t a = ma < 0 ? 0 : ma, b = mb >= (int32_t)GB_WAVE ? (int32_t)GB_WAVE - 1 : mb;
        if (b >= a)
            cv_rect(8 + a, y0 + 13, b - a + 1, 32, win);
        for (x = 0; x < (int32_t)GB_WAVE; x++) {
            int32_t h2 = (int32_t)pk[x] * hh / 256;
            h2 = h2 < 1 ? 1 : h2 > hh ? hh : h2;
            cv_rect(8 + x, yc - h2, 1, h2 * 2, x >= ma && x <= mb ? in : out);
        }
        for (x = 0; x < 2; x++) {
            int32_t mx = x ? mb : ma, r;
            if (mx < 0 || mx >= (int32_t)GB_WAVE)
                continue;
            cv_rect(8 + mx, y0 + 10, 1, 35, in);
            for (r = 0; r < 4; r++)
                cv_rect(8 + mx - 3 + r, y0 + 9 - (3 - r), 7 - 2 * r, 1, in);
            cv_text_in(8 + mx - 8, y0 - 1, 16, &AF_S, x ? "E" : "S", in, box);
        }
        if (v->wp && !v->zoom)
            cv_rect(8 + (int32_t)v->wp - 1, y0 + 10, 2, 35, GB_RGB(230, 40, 34));
        for (x = 0; x <= 8; x++)
            cv_rect(8 + x * 27, y0 + 45, 1, 2, out);
    } else if (v->wv == 1u)
        cv_text_in(6, y0 + 16, 228, &AF_M, "NO SAMPLE", in, box);
    if (v->smp_row == 1u) {
        tl_smp_row(v, 124, 1, v->lab, v->val, v->vic, v->mask, bgc);
        tl_smp_row(v, 169, 0, v->lab2, v->val2, v->vic2, v->mask2, bgc);
    } else {
        tl_smp_row(v, 124, 0, v->lab2, v->val2, v->vic2, v->mask2, bgc);
        tl_smp_row(v, 169, 1, v->lab, v->val, v->vic, v->mask, bgc);
    }
    tl_footer(v, bgc);
}

static uint16_t tl_tint(uint16_t bgc, uint32_t a)
{
    int32_t r = bgc >> 11, g = (bgc >> 5) & 63, b = bgc & 31, t = T_THEME;
    r += (((t >> 11) - r) * (int32_t)a) / 256;
    g += ((((t >> 5) & 63) - g) * (int32_t)a) / 256;
    b += (((t & 31) - b) * (int32_t)a) / 256;
    return (uint16_t)(r << 11 | g << 5 | b);
}

static void tl_curve(int32_t ty, int32_t xc, int32_t r, int32_t y0, int32_t yb, uint16_t bgc)
{
    int32_t x, py = yb, H = (yb - y0) * 3 / 5, top = 0;
    const uint16_t fill = tl_tint(bgc, 46);
    for (x = 12; x <= 228; x += 2) {
        int32_t d = x - xc, h, b = d < 0 ? -d : d, y;
        if (ty == 0)
            h = d <= 0 ? H : H - d * H / 46;
        else if (ty == 2)
            h = d >= 0 ? H : H + d * H / 46;
        else
            h = H - b * H / 52;
        if (b < 16)
            h += r * ((yb - y0) - H) / 1000 * (16 - b) * (16 - b) / 256;
        h = h < 0 ? 0 : h > yb - y0 ? yb - y0 : h;
        y = yb - h;
        if (h > 1)
            cv_rect(x, y + 1, 2, yb - y - 1, fill);
        if (x > 12)
            cv_line_t(x - 2, py, x, y, T_THEME, 2);
        if (x <= xc && x + 2 > xc)
            top = y;
        py = y;
    }
    cv_rect(12, yb, 216, 1, GB_W);
    if (top)
        fxg_disc(xc, top, 3, T_THEME);
}
static void tl_filter2(const gb_view_t *v, uint16_t bgc)
{
    static const char *const F[3] = {"100", "1k", "10k"};
    int32_t xc = 20 + v->ratio[0] * 196 / 1000, i;
    char t = v->val[2][0];
    int32_t ty = t == 'L' ? 0 : t == 'B' ? 1 : t == 'H' ? 2 : v->ratio[2] * 3 / 1001;
    for (i = 0; i < 3; i++) {
        int32_t x = 48 + i * 72;
        cv_rect(x, TL_D0 + 2, 1, TL_D1 - TL_D0 - 16, T_LINE);
        cv_text_in(x - 16, TL_D1 - 13, 32, &AF_S, F[i], T_MID, bgc);
    }
    tl_curve(ty, xc, v->ratio[1], TL_D0 + 2, TL_D1 - 15, bgc);
}

static void tl_dj2(const gb_view_t *v, uint16_t bgc)
{
    uint32_t i;
    int32_t a, px, py;
    for (i = 0; i <= NTRK; i++) {
        int32_t cx = 26 + (int32_t)i * 31, cy = TL_D0 + 12, val = v->dj_v[i], on = val > 1 || val < -1;
        uint16_t c = i < NTRK ? gb_col[i] : GB_W;
        char nb[3] = {i < NTRK ? 'T' : 'M', i < NTRK ? (char)('1' + i) : 0, 0};
        if (i == v->dj_sel)
            cv_rrect(cx - 15, TL_D0 - 2, 30, 40, 4, GB_W, bgc);
        fxg_disc(cx, cy, 11, i == v->dj_sel ? T_SURF : bgc);
        for (a = 0; a < 360; a += 6) {
            dial_pt(cx, cy, 11, a, &px, &py);
            cv_rect(px, py, 2, 2, (a >= 225 || a <= 135) ? c : bgc);
        }
        dial_pt(cx, cy, 9, (val * 135 / 64 + 360) % 360, &px, &py);
        cv_line_t(cx, cy, px, py, on ? T_THEME : GB_W, 2);
        cv_text_in(cx - 15, TL_D0 + 24, 30, &AF_S, nb, i == v->dj_sel ? bgc : (on ? T_THEME : T_MID), i == v->dj_sel ? GB_W : bgc);
    }
    {
        int32_t val = v->dj_v[v->dj_sel % (NTRK + 1u)], lp = val < -1, hp = val > 1;
        int32_t r = (val + 64) * 1000 / 127, xc = lp ? 20 + r * 400 / 1000 : hp ? 20 + (r - 500) * 400 / 1000 : 228;
        tl_curve(hp ? 2 : 0, lp || hp ? xc : 236, lp || hp ? 600 : 0, TL_D0 + 42, TL_D1 - 1, bgc);
        cv_text_r(228, TL_D0 + 42, &AF_S, lp ? "LOW-PASS" : hp ? "HIGH-PASS" : "OPEN", T_MID, bgc);
    }
}

static void tl_euclid2(const gb_view_t *v, uint16_t bgc)
{
    const uint16_t gold = GB_RGB(212, 175, 55), dgold = GB_RGB(150, 116, 30), dial = T_ACCENT;
    int32_t len = v->cur[0] < 1 ? 1 : v->cur[0] > 64 ? 64 : v->cur[0], hits = v->cur[1] < 0 ? 0 : v->cur[1] > len ? len : v->cur[1];
    int32_t rot = v->cur[2] < 0 ? 0 : v->cur[2], i, cx = 120, cy = (TL_D0 + TL_D1) / 2, px, py, qx, qy;
    (void)bgc;
    cv_rect(cx + 36, cy - 4, 6, 8, gold);
    cv_rect(cx + 41, cy - 3, 2, 6, dgold);
    fxg_disc(cx, cy, 37, dgold);
    fxg_disc(cx, cy, 35, gold);
    for (i = 0; i < 360; i += 10) {
        dial_pt(cx, cy, 33, i, &px, &py);
        cv_rect(px, py, 2, 2, dgold);
    }
    fxg_disc(cx, cy, 30, dial);
    for (i = 0; i < len; i++) {
        int32_t on = hits && ((i + len - rot % len) % len) * hits % len < hits, deg = i * 360 / len;
        dial_pt(cx, cy, 26, deg, &px, &py);
        if (i == 0) {
            cv_rect(px - 3, py - 3, 7, 2, on ? gold : dgold);
            cv_rect(px - 2, py - 1, 5, 2, on ? gold : dgold);
            cv_rect(px - 1, py + 1, 3, 2, on ? gold : dgold);
        } else if (on) {
            dial_pt(cx, cy, 21, deg, &qx, &qy);
            cv_line_t(qx, qy, px, py, gold, len > 32 ? 2 : 3);
        } else
            cv_rect(px, py, 2, 2, GB_RGB(120, 120, 116));
    }
    {
        dial_pt(cx, cy, 15, 300, &px, &py);
        cv_line_t(cx, cy, px, py, gold, 3);
        dial_pt(cx, cy, 21, 60, &px, &py);
        cv_line_t(cx, cy, px, py, gold, 2);
    }
    if (v->sq_pos) {
        int32_t st = ((int32_t)v->sq_at + (int32_t)v->sq_pos - 1) % len;
        dial_pt(cx, cy, 27, st * 360 / len, &px, &py);
        cv_line_t(cx, cy, px, py, GB_RGB(220, 40, 30), 1);
    }
    fxg_disc(cx, cy, 2, gold);
    {
        char nb[4];
        fmt_int(nb, hits);
        cv_rect(cx + 13, cy - 6, 14, 12, GB_RGB(244, 238, 226));
        cv_text_in(cx + 13, cy - 6, 14, &AF_S, nb, GB_RGB(18, 18, 16), GB_RGB(244, 238, 226));
    }
}

static void tl_menu(const gb_view_t *v, uint16_t bgc)
{
    uint32_t i, n = 0, at = 0, idx[24], first;
    int32_t y;
    (void)bgc;
    for (i = 0; i < NPAGES && n < 24u; i++)
        if (PAGES[i].fam == v->menu_f - 1u && page_visible(i)) {
            if (i == ui.page)
                at = n;
            idx[n++] = i;
        }
    if (!n)
        return;
    first = at > 3u ? at - 3u : 0u;
    if (n > 7u && first + 7u > n)
        first = n - 7u;
    tl_card(122, 26, 112, 8 + 22 * (int32_t)(n - first < 7u ? n - first : 7u), GB_K);
    for (i = first, y = 30; i < n && i < first + 7u; i++, y += 22) {
        int on = i == at;
        if (on)
            cv_rrect(130, y, 98, 20, 3, T_THEME, TL_CARD);
        cv_text_on(134, y + 3, &AF_S, PAGES[idx[i]].title, on ? T_SURF : TL_CARD_INK, on ? T_THEME : TL_CARD);
    }
    if (first)
        cv_rect(178, 27, 6, 2, TL_CARD_INK);
    if (first + 7u < n)
        cv_rect(178, 30 + 22 * 7 - 3, 6, 2, TL_CARD_INK);
}

static void tl_goto(const char *title)
{
    uint32_t i;
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].fam == FAM_TRK && str_eq(PAGES[i].title, title) && page_visible(i)) {
            ui.page = (uint8_t)i;
            ui.home = 0;
            ui.fam_last[FAM_TRK] = (uint8_t)i;
            page_entered();
            ui.force = 1;
            return;
        }
}
static const char *tl_result_page(const track_t *t)
{
    return MACHINES[trk_mach(t) % NMACH].drum ? "SEQUENCER" : "PIANO ROLL";
}

static void tl_comb2(const gb_view_t *v, uint16_t bgc)
{
    int32_t x, py = TL_D1 - 4, n = 3 + tl_r(v, 1) * 9 / 1000, fb = tl_r(v, 2), mx = tl_r(v, 0), dm = tl_r(v, 3), first = 0;
    const uint16_t fill = tl_tint(bgc, 46);
    int32_t yb = TL_D1 - 4, H = 18 + mx * 36 / 1000;
    for (x = 12; x <= 228; x += 2) {
        int32_t c = tl_sin((uint32_t)(x - 12) * (uint32_t)n * 512u / 214u + 256u), a = c < 0 ? -c : c, y;
        a = a * a / 256;
        if (fb > 400)
            a = a * a / 256;
        if (fb > 750)
            a = a * a / 256;
        a = a * (1000 - dm * (x - 12) / 260) / 1000;
        y = yb - 6 - a * H / 256;
        cv_rect(x, y, 2, yb - y, fill);
        if (x > 12)
            cv_line_t(x - 2, py, x, y, T_THEME, 2);
        if (!first && x > 20 && y < py && y <= yb - 6 - H * 9 / 10)
            first = x;
        py = y;
    }
    cv_rect(12, yb, 216, 1, GB_W);
    for (x = 12; x < 228; x += 8)
        cv_rect(x, yb - 6 - H + dm * (x - 12) * H / (260 * 1000), 4, 1, GB_W);
    {
        char b[12];
        int32_t t = v->cur[1] < 0 ? 0 : v->cur[1], hz = (int32_t)((44100u * 256u) / (uint32_t)mfx_delay(t));
        fmt_int(b, hz);
        str_cpy(b + str_len(b), " Hz", sizeof b - str_len(b));
        cv_text_r(228, TL_D0, &AF_S, b, T_MID, bgc);
    }
}

static void tl_clock2(const gb_view_t *v, uint16_t bgc)
{
    const uint16_t gold = GB_RGB(212, 175, 55), wood = T_ACCENT;
    int32_t r, cx = 52, base = TL_D1 - 2, top = TL_D0 + 2, i, ang, px, py;
    uint32_t q;
    for (r = 0; r <= base - top; r++) {
        int32_t half = 8 + r * 22 / (base - top);
        cv_rect(cx - half, top + r, 2 * half, 1, wood);
    }
    for (i = 0; i < 6; i++)
        cv_rect(cx - 3, top + 10 + i * 8, 6, 1, gold);
    ang = v->play ? tl_sin((uint32_t)v->clk_ph * 32u) * 24 / 256 : 0;
    dial_pt(cx, base - 10, 56, (ang + 360) % 360, &px, &py);
    cv_line_t(cx, base - 10, px, py, gold, 2);
    dial_pt(cx, base - 10, 38, (ang + 360) % 360, &px, &py);
    cv_rect(px - 4, py - 3, 9, 7, T_THEME);
    fxg_disc(cx, base - 10, 3, gold);
    for (q = 0; q < 4u; q++) {
        int32_t bx = 112 + (int32_t)q * 30, on = v->play ? q == v->clk_beat : q == 0u;
        fxg_disc(bx, TL_D0 + 12, 9, on ? T_THEME : T_LINE);
        if (!on)
            fxg_disc(bx, TL_D0 + 12, 7, bgc);
    }
    {
        int32_t sw = v->ratio[1] * 4 / 1000;
        for (q = 0; q < 16u; q++) {
            int32_t x = 100 + (int32_t)q * 8 + ((q & 1u) ? sw : 0), h = (q & 3u) == 0u ? 30 : (q & 1u) ? 12 : 20;
            cv_rect(x, TL_D1 - 2 - h, 3, h, (q & 1u) && sw ? T_THEME : GB_W);
        }
        cv_rect(98, TL_D1 - 2, 130, 1, T_LINE);
    }
}

#define BM_AUTO_MS 5000u
static uint8_t bm_sel, bm_kit, bm_touched;
static uint32_t bm_t0;
static const char *trap_kit_name(uint32_t k);
static void bm_start(void)
{
    bm_on = 1;
    bm_sel = 0;
    bm_kit = 0;
    bm_touched = 0;
    bm_t0 = fm1_ms;
    kb_boot_hold = 1;
    ui.force = 1;
}
static void bm_fill(gb_view_t *v)
{
    uint32_t k, e = fm1_ms - bm_t0;
    v->bm = bm_on;
    if (bm_on == 3u) {
        v->bm_left = (uint8_t)(e >= 800u ? 16u : e * 16u / 800u);
        return;
    }
    v->bm_sel = bm_sel;
    v->bm_kit = bm_kit;
    v->bm_left = (uint8_t)(bm_touched || bm_on != 1u ? 0u : e >= BM_AUTO_MS ? 1u : (BM_AUTO_MS - e) * 20u / BM_AUTO_MS + 1u);
    for (k = 0; k < 11u; k++)
        if (gbk_used(k))
            v->bm_used |= (uint16_t)(1u << k);
}
static void tl_bootmenu(const gb_view_t *v, uint16_t bgc)
{
    d99_bootmenu(v, bgc);
}

#include "trap_design98.c"
#include "trap_design98b.c"
#include "trap_design99.c"

static void tl_picture(const gb_view_t *v, uint16_t bgc)
{
    const char *t = v->title;
    const machine_t *m = &MACHINES[v->mach % NMACH];
    if (v->cross) {
        cv_line_t(84, TL_D0 + 2, 156, TL_D1 - 2, GB_W, 6);
        cv_line_t(156, TL_D0 + 2, 84, TL_D1 - 2, GB_W, 6);
        cv_text_in(0, TL_D1 - 18, 240, &AF_M, "NOT ON FX", GB_W, bgc);
        return;
    }
    if (v->wv) {
        tl_sample(v, bgc);
        return;
    }
    if (v->mixer) {
        d99_mix(v, bgc);
        return;
    }
    if (!v->fx && cur_page()->graph == GR_GBSEQ) {
        d99_seq(v, bgc);
        return;
    }
    if (!v->fx && cur_page()->graph == GR_ACID) {
        tl_acid(v, bgc);
        return;
    }
    {
        int syn = !v->fx && str_eq(m->pill, "FMSYN"), mono = !v->fx && str_eq(m->pill, "MONO");
        int eng = cur_page()->scope == SC_ENGINE;
        if (str_eq(t, "ENV") || str_eq(t, "FILTER ENV"))
            dz_adsr(v, bgc, t[0] == 'F');
        else if (str_eq(t, "LFO"))
            dz_lfo(v, bgc);
        else if (t[0] == 'L' && t[1] == 'F' && t[2] == 'O' && t[3] == ' ' && t[4] >= '1' && t[4] <= '3')
            tl_lfo(v, bgc);
        else if (str_eq(t, "FILTER"))
            dz_filter(v, bgc);
        else if (eng && str_eq(t, "OSC 1"))
            syn ? dz_fm_osc1(v, bgc) : mono ? dz_mono1(v, bgc) : m->drum ? dz_808a(v, bgc) : dz_knobs(v, bgc);
        else if (eng && str_eq(t, "OSC 2"))
            mono ? dz_mono2(v, bgc) : tl_lab(v, 0, "SWEEP") ? dz_808b(v, bgc) : dz_knobs(v, bgc);
        else if (str_eq(t, "CLOCK"))
            d99_clock(v, bgc);
        else if (str_eq(t, "VOL PAN"))
            dz_volpan(v, bgc);
        else if (str_eq(t, "AMP"))
            dz_amp(v, bgc);
        else if (str_eq(t, "ENV DEST"))
            dz_envdest(v, bgc);
        else if (str_eq(t, "SYNC"))
            dz_sync(v, bgc);
        else if (str_eq(t, "SENDS"))
            dz_sends(v, bgc);
        else if (str_eq(t, "DELAY"))
            d99_delay(v, bgc);
        else if (str_eq(t, "VERB"))
            d99_verb(v, bgc);
        else if (str_eq(t, "CRUSH"))
            dz_crush(v, bgc);
        else if (str_eq(t, "MOD C") || str_eq(t, "MOD D"))
            dz_op(v, bgc, t[4] == 'D');
        else if (str_eq(t, "TUNE"))
            dz_tune(v, bgc);
        else if (str_eq(t, "COLOR"))
            dz_color(v, bgc);
        else if (str_eq(t, "TAPE"))
            dz_tape(v, bgc);
        else if (str_eq(t, "VHS"))
            dz_vhs(v, bgc);
        else if (str_eq(t, "VINYL"))
            dz_vinyl(v, bgc);
        else if (str_eq(t, "COMB"))
            dz_comb(v, bgc);
        else if (str_eq(t, "DJ"))
            dz_dj(v, bgc);
        else if (str_eq(t, "ARP"))
            tl_arp(v, bgc);
        else if (str_eq(t, "CHORD"))
            tl_chord(v, bgc);
        else if (str_eq(t, "SCALE"))
            tl_scale1(v, bgc);
        else if (str_eq(t, "CHORD GEN"))
            dz_chordgen(v, bgc);
        else if (str_eq(t, "CHORD VAR"))
            dz_chordvar(v, bgc, DZ_Y, DZ_H, 0);
        else if (str_eq(t, "LOW ROLLER"))
            tl_automat(v, TL_AN_BASS, 14, DZ_Y, 212, DZ_H, 0);
        else if (str_eq(t, "GOLDEN CHORDS"))
            tl_automat(v, TL_AN_JAZZ, 14, DZ_Y, 212, DZ_H, 0);
        else if (str_eq(t, "LUCKY DRUMS"))
            tl_automat(v, TL_AN_TRAP, 14, DZ_Y, 212, DZ_H, 0);
        else if (str_eq(t, "ACE HATS")) {
            tl_automat(v, TL_AN_HAT, 14, DZ_Y, 212, DZ_H, 0);
            if (v->hg_trk >= NTRK) {
                cv_rrect(30, DZ_Y + 40, 180, 26, 5, T_THEME, T_ACCENT);
                cv_text_in(30, DZ_Y + 45, 180, &AF_S, "NO HIHAT TRACK", T_SURF, T_THEME);
            }
        } else if (str_eq(t, "QUICK FILL"))
            dz_quickfill(v, bgc);
        else if (str_eq(t, "EUCLID"))
            dz_euclid(v, bgc);
        else if (str_eq(t, "BANKS"))
            dz_banks(v, bgc);
        else if (str_eq(t, "SCENES"))
            d99_scenes(v, bgc);
        else if (str_eq(t, "ACCENT"))
            tl_accent(v, bgc);
        else if (str_eq(t, "ROLL"))
            tl_rollfn(v, bgc);
        else if (v->icon)
            gb_icon(96, TL_D0 + 25, 3, v->icon, gb_mix(bgc, 110));
    }
}

static void tl_page(const gb_view_t *v, uint16_t bgc)
{
    if (v->an_k) {
        tl_status(v);
        if (v->an_k == TL_AN_DICE) {
            tl_title(v, bgc);
            dz_chordvar(v, bgc, DZ_Y, DZ_H, v->an_f);
            tl_cols(v, bgc);
        } else
            tl_automat(v, v->an_k, 12, 30, 216, 178, v->an_f);
        tl_footer(v, bgc);
        return;
    }
    if (v->smp_row) {
        tl_sampler(v, bgc);
        return;
    }
    if (str_eq(v->title, "PIANO ROLL")) {
        tl_pianoroll(v, bgc);
        return;
    }
    if (str_eq(v->title, "SEQUENCER")) {
        tl_status(v);
        tl_title(v, bgc);
        tl_overview(v, bgc);
        tl_footer(v, bgc);
        return;
    }
    tl_status(v);
    tl_title(v, bgc);
    tl_picture(v, bgc);
    tl_cols(v, bgc);
    tl_footer(v, bgc);
}

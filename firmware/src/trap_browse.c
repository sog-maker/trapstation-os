/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION contributors */

enum { LB_DRUM, LB_808, LB_SYNTH, LB_SMPL, LB_NCAT };
static const char *const LB_CAT[LB_NCAT] = {"DRUMKITS", "808", "SYNTH", "SAMPLER"};
typedef struct { uint8_t kind, a, b, mach; } lb_t;

#define TRAP_808N 4u
static const ba_sound_t TRAP_808S[TRAP_808N] = {
    {"TRAP 808 LONG", "8BAS", {{P_E0, 64}, {P_E1, 118}, {P_E2, 8}, {P_E3, 40}, {P_E4, 14}, {P_E5, 10}, {P_E6, 55},
                               {P_LN0, 110}, {P_LN1, 0}, {P_DIST, 80}, {P_GLIDE, 45}, {P_TRANS, 0}, BA_END}},
    {"TRAP 808 PUNCH", "8BAS", {{P_E0, 64}, {P_E1, 72}, {P_E2, 40}, {P_E3, 70}, {P_E4, 45}, {P_E5, 18}, {P_E6, 85},
                                {P_LN0, 112}, {P_LN1, 0}, {P_DIST, 55}, {P_GLIDE, 20}, {P_TRANS, 0}, BA_END}},
    {"TRAP 808 SLIDE", "8BAS", {{P_E0, 64}, {P_E1, 108}, {P_E2, 12}, {P_E3, 55}, {P_E4, 20}, {P_E5, 12}, {P_E6, 100},
                                {P_LN0, 108}, {P_LN1, 0}, {P_DIST, 70}, {P_GLIDE, 85}, {P_TRANS, 0}, BA_END}},
    {"MEMPHIS 808 BOOM", "8BAS", {{P_E0, 64}, {P_E1, 112}, {P_E2, 25}, {P_E3, 32}, {P_E4, 30}, {P_E5, 26}, {P_E6, 70},
                                  {P_LN0, 108}, {P_LN1, 25}, {P_DIST, 60}, {P_GLIDE, 0}, {P_TRANS, 0}, BA_END}},
};
#define LB_NKIT (TRAP_NKITS * NTRK)
#define LB_NSRC (TRAP_808N + LB_NKIT + BANK_A_N + BANK_EXTRA_COUNT * BANK_EXTRA_N + NMACH)
static uint16_t lb_cur[NTRK];
static uint8_t gl_mode, gl_cat, gl_used;
static uint16_t gl_idx[LB_NCAT];
static uint16_t ov_i;

static uint32_t lb_mach(const char *pill)
{
    uint32_t m;
    for (m = 0; m < NMACH && !str_eq(MACHINES[m].pill, pill); m++)
        ;
    return m;
}
static int lb_syn(uint32_t m) { return m < NMACH && (str_eq(MACHINES[m].pill, "FMSYN") || str_eq(MACHINES[m].pill, "MONO") || str_eq(MACHINES[m].pill, "8BAS")); }
static int lb_808(uint32_t m) { return m < NMACH && str_eq(MACHINES[m].pill, "8BAS"); }
static int lb_smp(uint32_t m) { return m < NMACH && MACHINES[m].engine == ENGI_SAMPLER; }
static int lb_src(uint32_t j, lb_t *e)
{
    if (j < TRAP_808N) {
        e->kind = 5;
        e->a = 0;
        e->b = (uint8_t)j;
        e->mach = (uint8_t)lb_mach(TRAP_808S[j].machine);
        return 1;
    }
    j -= TRAP_808N;
    if (j < LB_NKIT) {
        e->kind = 0;
        e->a = (uint8_t)(j / NTRK);
        e->b = (uint8_t)(j % NTRK);
        e->mach = (uint8_t)lb_mach(TRAP_KITS[e->a].s[e->b].pill);
        return 1;
    }
    j -= LB_NKIT;
    if (j < BANK_A_N) {
        e->kind = 1;
        e->a = 0;
        e->b = (uint8_t)j;
        e->mach = (uint8_t)lb_mach(BANK_A[j].machine);
        return 1;
    }
    j -= BANK_A_N;
    if (j < BANK_EXTRA_COUNT * BANK_EXTRA_N) {
        e->kind = 2;
        e->a = (uint8_t)(j / BANK_EXTRA_N);
        e->b = (uint8_t)(j % BANK_EXTRA_N);
        e->mach = (uint8_t)lb_mach(BANK_EXTRA[e->a][e->b].machine);
        return 1;
    }
    j -= BANK_EXTRA_COUNT * BANK_EXTRA_N;
    e->kind = 3;
    e->a = e->mach = (uint8_t)j;
    return j < NMACH;
}
static uint32_t lb_cat_of(const lb_t *e)
{
    if (e->kind == 4u)
        return LB_SMPL;
    if (e->mach >= NMACH || lb_smp(e->mach) || (e->kind == 0u && lb_syn(e->mach)))
        return LB_NCAT;
    return lb_808(e->mach) ? LB_808 : lb_syn(e->mach) ? LB_SYNTH : LB_DRUM;
}

static int lb_fits(const track_t *t, const lb_t *e)
{
    uint32_t m = trk_mach(t), c = lb_cat_of(e);
    if (lb_smp(m))
        return c == LB_SMPL;
    if (lb_808(m))
        return c == LB_808;
    if (lb_syn(m))
        return c == LB_SYNTH;
    return c == LB_DRUM && MACHINES[e->mach % NMACH].icon == MACHINES[m].icon;
}
static int lb_get(uint32_t c, const track_t *fit, uint32_t i, lb_t *e, uint32_t *j)
{
    uint32_t s, n = 0;
    if (c == LB_SMPL || (fit && lb_smp(trk_mach(fit)))) {
        if (i >= smp_n)
            return 0;
        e->kind = 4;
        e->a = 0;
        e->b = (uint8_t)i;
        e->mach = (uint8_t)lb_mach("SMPL");
        *j = 0x8000u | i;
        return 1;
    }
    for (s = 0; s < LB_NSRC; s++)
        if (lb_src(s, e) && (fit ? lb_fits(fit, e) : lb_cat_of(e) == c) && n++ == i) {
            *j = s + 1u;
            return 1;
        }
    return 0;
}
static uint32_t lb_count(uint32_t c, const track_t *fit)
{
    lb_t e;
    uint32_t s, n = 0;
    if (c == LB_SMPL || (fit && lb_smp(trk_mach(fit))))
        return smp_n;
    for (s = 0; s < LB_NSRC; s++)
        if (lb_src(s, &e) && (fit ? lb_fits(fit, &e) : lb_cat_of(&e) == c))
            n++;
    return n;
}
static void lb_name(const lb_t *e, char *o, uint32_t n)
{
    uint32_t k, i;
    o[0] = 0;
    switch (e->kind) {
    case 0:
        for (k = 0; TRAP_KITS[e->a].name[k] && TRAP_KITS[e->a].name[k] != ' ' && k + 1u < n; k++)
            o[k] = TRAP_KITS[e->a].name[k];
        o[k] = 0;
        str_cpy(o + k, " ", n - k);
        str_cpy(o + str_len(o), TRAP_KITS[e->a].s[e->b].pill, n - str_len(o));
        for (i = 0, k = 0; i < e->b; i++)
            k += str_eq(TRAP_KITS[e->a].s[i].pill, TRAP_KITS[e->a].s[e->b].pill);
        if (k && str_len(o) + 2u < n) {
            str_cpy(o + str_len(o), " ", n - str_len(o));
            fmt_int(o + str_len(o), (int32_t)k + 1);
        }
        return;
    case 1:
        str_cpy(o, BANK_A[e->b].name, n);
        return;
    case 5:
        str_cpy(o, TRAP_808S[e->b % TRAP_808N].name, n);
        return;
    case 2:
        str_cpy(o, BANK_EXTRA[e->a][e->b].name, n);
        return;
    case 3:
        str_cpy(o, MACHINES[e->a % NMACH].pill, n);
        str_cpy(o + str_len(o), " INIT", n - str_len(o));
        return;
    default:
        o[0] = 'S';
        o[1] = (char)('0' + (e->b + 1u) / 10u);
        o[2] = (char)('0' + (e->b + 1u) % 10u);
        o[3] = ' ';
        str_cpy(o + 4, e->b < smp_n ? smp_tab[e->b].name : "", n - 4u);
        return;
    }
}
static void lb_load(track_t *t, const lb_t *e, uint32_t j, int say)

{
    char nm[20];
    switch (e->kind) {
    case 1:
        trap_bank_a_load(t, e->b);
        break;
    case 2:
        trap_bank_load(t, e->a, e->b);
        break;
    case 5:
        trap_sound_load(t, &TRAP_808S[e->b % TRAP_808N], 1);
        break;
    default:
        load_begin(t, UNDO_SOUND);
        panic_req |= (uint8_t)(1u << trk_index(t));
        if (e->kind == 0u)
            trap_snd_apply(t, &TRAP_KITS[e->a].s[e->b]);
        else
            machine_set(t, e->mach);
        if (e->kind == 4u) {
            fm1_irq_off();
            t->p[P_E5] = (int16_t)e->b;
            fm1_irq_on();
        }
        load_end(t);
        lb_name(e, nm, sizeof nm);
        if (say)
            ui_message(nm);
        break;
    }
    if (!say && (e->kind <= 2u || e->kind == 5u))
        ui.msg_t = 0;
    lastc[trk_index(t)] = 0;
    lb_cur[trk_index(t)] = (uint16_t)j;
    ui.force = 1;
}

static uint32_t lb_cur_of(const track_t *t)
{
    uint32_t j = lb_cur[trk_index(t)];
    lb_t e;
    if (!j)
        return 0;
    if (j & 0x8000u)
        return lb_smp(trk_mach(t)) && (uint32_t)t->p[P_E5] == (j & 0xFFu) ? j : 0u;
    return lb_src(j - 1u, &e) && e.mach == trk_mach(t) ? j : 0u;
}

static int gl_btn(uint32_t b)
{
    return FELUCCA_SIMPLE && ((fm1_in.buttons >> panel.btn[b]) & 1u) && !ui.confirm && !bm_on && !gbsv_keys_on() &&
           !gbs_held();
}
static int gl_held(void) { return gl_btn(B_HOME); }
static void gl_eat(void)
{
    ui.pg_down &= (uint16_t)~(1u << panel.btn[B_HOME]);
    ui.home_t0 |= 2u;
    if (ui.ly)
        layer_kill();
    tl_menu_f = 0;
}
static void ov_open(void)
{
    uint32_t n = fxl_sel ? 0u : lb_count(LB_NCAT, TSEL), i, j, c = fxl_sel ? 0u : lb_cur_of(TSEL);
    lb_t e;
    ov_i = 0xFFFFu;
    for (i = 0; c && i < n; i++)
        if (lb_get(LB_NCAT, TSEL, i, &e, &j) && j == c) {
            ov_i = (uint16_t)i;
            break;
        }
    gl_mode = 2;
}

static int gl_enc(uint32_t en, int32_t s)
{
    uint32_t n;
    if (en == EN_SELECT) {
        if (!gl_btn(B_OCTUP) || gl_mode == 1u)
            return 0;
        ou_eat = 1;
        if (gl_mode != 2u)
            ov_open();
        ui.force = 1;
        return 1;
    }
    if (!gl_held() || gl_mode == 2u)
        return 0;
    gl_eat();
    gl_used = 1;
    if (en == EN_PRESET) {
        if (gl_mode == 1u)
            gl_cat = (uint8_t)((gl_cat + (s > 0 ? 1u : LB_NCAT - 1u)) % LB_NCAT);
        gl_mode = 1;
    } else if (en == EN_ALGO) {
        n = lb_count(gl_cat, 0);
        if (gl_mode == 1u && n)
            gl_idx[gl_cat] = (uint16_t)clamp((int32_t)gl_idx[gl_cat] + accel(EN_ALGO, s, (int32_t)n), 0, (int32_t)n - 1);
        gl_mode = 1;
    }
    ui.force = 1;
    return 1;
}
static int gl_knob(uint32_t k, int32_t s)
{
    uint32_t n, j;
    lb_t e;
    if (gl_mode != 2u)
        return 0;
    if (k != 0u || fxl_sel || !(n = lb_count(LB_NCAT, TSEL)))
        return 1;
    if (ov_i >= n)
        ov_i = (uint16_t)(s > 0 ? 0u : n - 1u);
    else
        ov_i = (uint16_t)((ov_i + (s > 0 ? 1u : n - 1u)) % n);
    if (lb_get(LB_NCAT, TSEL, ov_i, &e, &j))
        lb_load(TSEL, &e, j, 0);
    return 1;
}
static void gl_poll(void)
{
    uint32_t j;
    lb_t e;
    if (!gl_mode || ((fm1_in.buttons >> panel.btn[gl_mode == 2u ? B_OCTUP : B_HOME]) & 1u))
        return;
    if (gl_mode == 1u) {
        if (fxl_sel)
            ui_message("FX TRACK: NO SOUND");
        else if (gl_idx[gl_cat] < lb_count(gl_cat, 0) && lb_get(gl_cat, 0, gl_idx[gl_cat], &e, &j))
            lb_load(TSEL, &e, j, 1);
        else
            ui_message(gl_cat == LB_SMPL ? "NO SAMPLES" : "NO SOUND");
    }
    gl_mode = 0;
    gl_used = 0;
    ui.force = 1;
}
static void gl_fill(gb_view_t *v)
{
    v->gl_mode = gl_mode;
    v->gl_cat = gl_cat;
    v->gl_idx = gl_mode == 2u ? ov_i : gl_idx[gl_cat % LB_NCAT];
    v->gl_cur = fxl_sel ? 0u : (uint16_t)lb_cur_of(TSEL);
}

static void gl_text(int32_t x, int32_t y, int32_t w, const aafont_t *f, const char *s, uint16_t fg, uint16_t bg)
{
    char b[24];
    uint32_t n;
    str_cpy(b, s, sizeof b);
    for (n = str_len(b); n > 1u && text_w(f, b) > w; n--)
        b[n - 1u] = 0;
    cv_text_on(x, y, f, b, fg, bg);
}
#define GL_CREAM GB_RGB(244, 238, 226)
#define GL_INK GB_RGB(18, 18, 16)
static void tl_browser(const gb_view_t *v)
{
    const int32_t x = 92, w = 144, y0 = 24, rh = 16, nr = 5;
    uint32_t c = v->gl_cat % LB_NCAT, n = lb_count(c, 0), i, j, first, at = v->gl_idx;
    int32_t y, h = 6 + (int32_t)LB_NCAT * rh + 6 + nr * rh + 18;
    lb_t e;
    char nm[20], nb[12];
    tl_card(x, y0, w, h, GB_K);
    for (i = 0, y = y0 + 4; i < LB_NCAT; i++, y += rh) {
        int on = i == c;
        if (on)
            cv_rrect(x + 6, y, w - 12, rh - 1, 3, T_THEME, TL_CARD);
        cv_text_on(x + 12, y + 1, &AF_S, LB_CAT[i], on ? T_SURF : TL_CARD_INK, on ? T_THEME : TL_CARD);
        if (on)
            cv_text_r(x + w - 10, y + 1, &AF_S, ">", T_SURF, T_THEME);
    }
    y += 2;
    cv_rect(x + 6, y, w - 12, 1, TL_CARD_INK);
    y += 4;
    if (!n) {
        cv_text_on(x + 12, y + 1, &AF_S, c == LB_SMPL ? "NO SAMPLES" : "NO SOUND", TL_CARD_INK, TL_CARD);
        return;
    }
    first = at > 2u ? at - 2u : 0u;
    if (n > (uint32_t)nr && first + (uint32_t)nr > n)
        first = n - (uint32_t)nr;
    for (i = first; i < n && i < first + (uint32_t)nr; i++, y += rh) {
        int on = i == at;
        if (!lb_get(c, 0, i, &e, &j))
            break;
        lb_name(&e, nm, sizeof nm);
        if (on)
            cv_rrect(x + 4, y, w - 8, rh - 1, 3, GL_CREAM, TL_CARD);
        gl_text(x + 9, y + 1, w - 18, &AF_S, nm, on ? GL_INK : TL_CARD_INK, on ? GL_CREAM : TL_CARD);
    }
    y = y0 + h - 17;
    fmt_int(nb, (int32_t)(at < n ? at + 1u : 0u));
    str_cpy(nb + str_len(nb), "/", sizeof nb - str_len(nb));
    fmt_int(nb + str_len(nb), (int32_t)n);
    cv_text_on(x + 9, y, &AF_S, nb, GL_CREAM, TL_CARD);
    cv_text_r(x + w - 8, y, &AF_S, "GLO UP: LOAD", TL_CARD_INK, TL_CARD);
}
static void tl_trkover(const gb_view_t *v)
{
    const int32_t x = 14, y0 = 26, w = 212, h = 182;
    uint32_t t = v->sel % NTRK, n = v->fx ? 0u : lb_count(LB_NCAT, &trk[t]), i, j;
    uint16_t col = v->fx ? GB_RGB(120, 128, 140) : gb_col[t];
    lb_t e;
    char nm[20], tb[8];
    tl_card(x, y0, w, h, GB_K);
    cv_rrect(x + 10, y0 + 10, 76, 76, 6, col, TL_CARD);
    if (!v->fx)
        gb_icon(x + 10 + 14, y0 + 10 + 14, 3, MACHINES[v->tm[t] % NMACH].icon, GB_W);
    if (v->fx)
        str_cpy(tb, "FX", sizeof tb);
    else {
        tb[0] = 'T';
        tb[1] = (char)('1' + t);
        tb[2] = 0;
    }
    cv_text_on(x + 98, y0 + 10, &AF_L, tb, TL_CARD_INK, TL_CARD);
    if (!v->fx)
        cv_text_on(x + 98, y0 + 46, &AF_M, MACHINES[v->tm[t] % NMACH].pill, T_THEME, TL_CARD);
    cv_rect(x + 10, y0 + 94, w - 20, 1, TL_CARD_INK);
    if (v->fx) {
        cv_text_on(x + 12, y0 + 104, &AF_S, "THE FX TRACK HAS NO SOUND", TL_CARD_INK, TL_CARD);
        return;
    }
    if (!v->gl_cur)
        str_cpy(nm, "OWN SOUND", sizeof nm);
    else if (v->gl_idx < n && lb_get(LB_NCAT, &trk[t], v->gl_idx, &e, &j))
        lb_name(&e, nm, sizeof nm);
    else
        str_cpy(nm, "OWN SOUND", sizeof nm);
    for (i = 0; i < 3u; i++) {
        int32_t y = y0 + 100 + (int32_t)i * 22, on = i == 1u;
        uint32_t at;
        char b[20];
        if (on) {
            cv_rrect(x + 8, y - 1, w - 16, 21, 4, T_THEME, TL_CARD);
            gl_text(x + 16, y + 1, w - 32, &AF_M, nm, T_SURF, T_THEME);
            continue;
        }
        if (n < 2u || v->gl_idx >= n)
            continue;
        at = i == 0u ? (v->gl_idx + n - 1u) % n : (v->gl_idx + 1u) % n;
        if (!lb_get(LB_NCAT, &trk[t], at, &e, &j))
            continue;
        lb_name(&e, b, sizeof b);
        gl_text(x + 16, y + 2, w - 32, &AF_S, b, TL_CARD_INK, TL_CARD);
    }
    {
        char nb[16] = "K1 SOUND  ";
        uint32_t l = str_len(nb);
        if (v->gl_idx < n) {
            fmt_int(nb + l, (int32_t)v->gl_idx + 1);
            str_cpy(nb + str_len(nb), "/", sizeof nb - str_len(nb));
        } else
            str_cpy(nb + l, "-/", sizeof nb - l);
        fmt_int(nb + str_len(nb), (int32_t)n);
        cv_text_on(x + 12, y0 + h - 18, &AF_S, nb, TL_CARD_INK, TL_CARD);
    }
}
static void tl_gl(const gb_view_t *v)
{
    if (v->gl_mode == 1u)
        tl_browser(v);
    else if (v->gl_mode == 2u)
        tl_trkover(v);
}

static void gh_browser(const gb_view_t *v)
{
    static const char *const TAB[LB_NCAT] = {"DRUM", "808", "SYNTH", "SMPL"};
    uint32_t c = v->gl_cat % LB_NCAT, n = lb_count(c, 0), i, j, first, at = v->gl_idx;
    lb_t e;
    char nm[24];
    int32_t y0 = 61, rh = 21, rs = 23, tx = 11;

    for (i = 0; i < LB_NCAT; i++) {
        uint16_t col = i == c ? GH_RED : GH_DARK;
        cv_rrect(4 + 59 * (int32_t)i, 34, 55, 22, 2, col, GH_BG);
        gh_t(31 + 59 * (int32_t)i, 49, 9, TAB[i], GH_WHITE, col, GA_M);
    }
    if (!n) {
        cv_rrect(4, y0, 232, rh, 2, GH_DARK, GH_BG);
        gh_tc(tx, y0 + 14, 10, c == LB_SMPL ? "NO USER SAMPLES" : "NO SOUND", GH_WHITE, GH_DARK, 220);
    }
    first = at > 2u ? at - 2u : 0u;
    if (n > 5u && first + 5u > n)
        first = n - 5u;
    for (i = first; i < n && i < first + 5u; i++) {
        int32_t y = y0 + rs * (int32_t)(i - first);
        uint16_t col = i == at ? GH_RED : GH_DARK;
        if (!lb_get(c, 0, i, &e, &j))
            break;
        lb_name(&e, nm, sizeof nm);
        cv_rrect(4, y, 232, rh, 2, col, GH_BG);
        gh_tc(tx, y + 15, 10, nm, GH_WHITE, col, 220);
    }
    if (!n && c == LB_SMPL)
        gh_hint("Keine belegten Sample-Slots", "HOME + PRESETS");
    else if (c == LB_808)
        gh_hint("PRESETS: Kategorie", "ALGORITHM: Sound");
    else
        gh_hint("PRESETS: Typ", "ALGORITHM: Eintrag / loslassen: laden");
}

static void gh_trkover(const gb_view_t *v)
{
    uint32_t t = v->sel % NTRK, n = v->fx ? 0u : lb_count(LB_NCAT, &trk[t]), j;
    lb_t e;
    char nm[24], tb[4] = {'T', (char)('1' + t), 0, 0};
    gh_t(GX(3), GY(14), 10, "TRACK / SOUND", GH_BG, GH_BG, GA_S);
    cv_rrect(GX(0), GY(25), GX(66) - GX(0), GY(90) - GY(25), 3, GH_RED, GH_BG);
    gh_t(GX(33), GY(67), 32, v->fx ? "FX" : tb, GH_WHITE, GH_RED, GA_M);
    gh_t(GX(79), GY(47), 19, v->fx ? "FX" : MACHINES[v->tm[t] % NMACH].pill, GH_WHITE, GH_BG, GA_S);
    if (!v->gl_cur || v->gl_idx >= n || !lb_get(LB_NCAT, &trk[t], v->gl_idx, &e, &j))
        str_cpy(nm, "OWN SOUND", sizeof nm);
    else
        lb_name(&e, nm, sizeof nm);
    gh_tc(GX(79), GY(68), 13, nm, GH_BG, GH_BG, GX(217) - GX(79));
    cv_rect(GX(78), GY(79), GX(217) - GX(78), 1, GH_P62);
    gh_t(GX(3), GY(112), 10, "K1 < SOUND >   OCT+ RELEASE", GH_GREY, GH_BG, GA_S);
    gh_hint("K1: Sound", "OCT+ loslassen: schlie\xdf" "en");
}
static void gh_gl(const gb_view_t *v)
{
    static const char *const BT[LB_NCAT] = {"PRESET BROWSER / DRUMKITS", "PRESET BROWSER / 808", "PRESET BROWSER / SYNTH",
                                            "PRESET BROWSER / SAMPLER"};
    char tt[28];
    str_cpy(tt, "TRACK OVERLAY / ", sizeof tt);
    str_cpy(tt + 16, v->fx ? "FX" : gh_pill(v->tm[v->sel % NTRK]), sizeof tt - 16u);
    gh_status(v, 0);
    gh_title(v, v->gl_mode == 2u ? tt : (v->gl_cat % LB_NCAT == LB_SMPL && !lb_count(LB_SMPL, 0) ?
             "PRESET BROWSER / EMPTY" : BT[v->gl_cat % LB_NCAT]), GG_DAW);
    gh_footer(v, GG_DAW);
    if (v->gl_mode == 2u)
        gh_trkover(v);
    else
        gh_browser(v);
}

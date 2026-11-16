/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

static void draw_menu(void);
static int name_on(void);
static uint64_t lock_held(void);
static void name_draw(void);

#define COL_W CARD_W
#define COL_H CARD_H

static int32_t batt_level(void)
{
    return song.batt_raw >= 591 ? 3 : song.batt_raw >= 561 ? 2 : song.batt_raw >= 531 ? 1 : 0;
}

static int32_t batt_shown(void)
{
    if (usb.config && !usb.suspended)
        return 4;
    return batt_level();
}

static void draw_rec_mark(int32_t x, uint16_t bg)
{
    if (song.rec)
        cv_icon_mid(x, H_HEAD / 2, 16, (song.rec >> song.sel) & 1u ? ICON_X_REC : ICON_X_REC_O, T_REC, bg);
}

static uint32_t batt_icon(int32_t lvl)
{
    static const uint8_t I[5] = {ICON_X_BAT0, ICON_X_BAT1, ICON_X_BAT3, ICON_X_BAT4, ICON_X_BAT_CHG};
    return I[clamp(lvl, 0, 4)];
}
static void draw_battery(int32_t bx)
{
    int32_t lvl = batt_shown();
    cv_icon_mid(bx, H_HEAD / 2, 24, batt_icon(lvl), lvl == 1 ? T_ACCENT : lvl == 0 ? T_MID : T_THEME, T_BG);
}

#define ROLL_FRAMES 9u
#define ROLL_SNAP 3u
#define ROLL_BPM 4u
#define ROLL_Y 18
#define ROLL_H 17
static const uint8_t ROLL_EASE[17] = {0, 45, 84, 118, 147, 172, 193, 210, 223, 234, 242, 247, 251, 253, 254, 255, 255};

static int roll_digit(char c) { return c >= '0' && c <= '9'; }

static int roll_dir(const char *a, const char *b)
{
    uint32_t i;
    int d = 0, neg = 0;
    for (i = 0; a[i] || b[i]; i++) {
        if (i + 1u >= sizeof ui.roll[0].from || roll_digit(a[i]) != roll_digit(b[i]))
            return 0;
        if (!roll_digit(a[i]) && (a[i] != b[i] || (a[i] != '-' && a[i] != '+' && a[i] != '.')))
            return 0;
        neg |= a[i] == '-';
        if (!d && a[i] != b[i])
            d = b[i] > a[i] ? 1 : -1;
    }
    return neg ? -d : d;
}

static void roll_note(uint32_t k, const char *a, const char *b, int snap)
{
    uint32_t el = (uint8_t)(ui.frame - ui.roll[k].t0);
    int d = roll_dir(a, b);
    ui.roll[k].t0 = (uint8_t)ui.frame;
    ui.roll[k].from[0] = 0;
    if (snap || !d || el < ROLL_SNAP || (ui_prefs & PREF_ANIM_OFF))
        return;
    str_cpy(ui.roll[k].from, a, sizeof ui.roll[k].from);
    ui.roll[k].dir = (int8_t)d;
}

static int32_t roll_text(uint32_t k, int32_t x, int32_t y, const char *s, uint16_t fg)
{
    const aafont_t *f = &AF_M;
    uint32_t e = (uint8_t)(ui.frame - ui.roll[k].t0) + 1u, i, p = 0;
    const char *a = ui.roll[k].from;
    int32_t cy = k < ROLL_BPM ? ROLL_Y : 0, h = k < ROLL_BPM ? ROLL_H : H_HEAD;
    uint16_t bg = k < ROLL_BPM ? T_SURF : T_BG;
    char t[8], ch[2] = {0, 0};
    if (e >= ROLL_FRAMES)
        ui.roll[k].from[0] = 0;
    if (!a[0])
        return cv_text_on(x, y, f, s, fg, bg);
    str_cpy(t, s, sizeof t);
    for (i = 0; s[i]; i++)
        p += (uint32_t)roll_digit(s[i]);
    cv_cy0 = (int16_t)(cy + cv_oy);
    cv_cy1 = (int16_t)(cy + cv_oy + h);
    cv_scroll = 1;
    for (i = 0; s[i]; i++) {
        int32_t px, ny = y;
        t[i] = 0;
        px = x + text_w(f, t);
        t[i] = s[i];
        p -= (uint32_t)roll_digit(s[i]);
        if (s[i] != a[i]) {
            int32_t o = 0, dir = ui.roll[k].dir;
            if (e > p)
                o = (int32_t)((ROLL_EASE[((e - p) * 32u / (ROLL_FRAMES - p) + 1u) >> 1] * (uint32_t)h + 128u) >> 8);
            ch[0] = a[i];
            cv_text_on(px, y - dir * o, f, ch, fg, bg);
            ny = y + dir * (h - o);
        }
        ch[0] = s[i];
        cv_text_on(px, ny, f, ch, fg, bg);
    }
    cv_cy0 = 0;
    cv_cy1 = (int16_t)cv_h;
    cv_scroll = 0;
    return x + text_w(f, s);
}

#define HEAD_SY CAP_IN(S, H_HEAD)
#define HEAD_MY CAP_IN(M, H_HEAD)
#define HEAD_TRK_X 4
#define HEAD_REC_X 20
#define HEAD_SLOT_X 23
#define HEAD_BAT_X 214
#define HEAD_GRP_R 208
#define HEAD_MSG_W 166
#define HEAD_GAP 4
#define HEAD_PLAY_GAP 6

typedef struct { int32_t bx, b[4], mx0, mx1, sx0, sx1, px0, px1, gx0, gx1; } head_geo_t;
static void head_geo(head_geo_t *g, const char *bpm)
{
    int32_t ib[4], lb[4], pb[4], c = 16, pw, lw = 0;
    uint32_t sz = 16;
    char z[8] = "0000000";
    z[str_len(bpm) < 7u ? str_len(bpm) : 7u] = 0;
    text_ink(&AF_M, z, g->b);
    icon_ink(16, ICON_TEMPO, ib);
    if (icon_cell(&sz, ICON_TEMPO) >= 0)
        c = (int32_t)sz;
    icon_ink(16, ICON_X_STOP, pb);
    pw = pb[2] - pb[0];
    icon_ink(16, ICON_X_PLAY, pb);
    if (pb[2] - pb[0] > pw)
        pw = pb[2] - pb[0];
    icon_ink(16, ICON_X_LOCK, lb);
    if (ui_prefs & PREF_BPM_LOCK)
        lw = HEAD_GAP + lb[2] - lb[0];
    g->gx0 = HALF_UP(240 - (pw + HEAD_PLAY_GAP + (ib[2] - ib[0]) + HEAD_GAP + (g->b[2] - g->b[0]) + lw));
    g->px0 = g->gx0;
    g->px1 = g->px0 + pw;
    g->mx0 = g->px1 + HEAD_PLAY_GAP;
    g->mx1 = g->mx0 + (ib[2] - ib[0]);
    g->bx = g->mx1 + HEAD_GAP - g->b[0];
    g->gx1 = g->bx + g->b[2] + lw;
    g->sx0 = g->mx1 + (ib[0] > c - ib[2] ? ib[0] : c - ib[2]);
    g->sx1 = g->bx + g->b[2] + HEAD_GAP - lb[0];
}

static void head_metro(int32_t x0, int32_t beat)
{
    int32_t ib[4], c = 16;
    uint32_t sz = 16;
    icon_ink(16, ICON_TEMPO, ib);
    if (icon_cell(&sz, ICON_TEMPO) >= 0)
        c = (int32_t)sz;
    cv_flip = (uint8_t)(beat > 0 && (beat & 1));
    cv_icon_mid(x0 - (cv_flip ? c - ib[2] : ib[0]), H_HEAD / 2, 16, ICON_TEMPO, beat == 0 ? T_ACCENT : T_MID, T_BG);
    cv_flip = 0;
}

static int32_t head_hint_w(const char *s, int32_t maxw)
{
    uint32_t n, i;
    int32_t id = kc_tag(s, &n), w = 0;
    char b[48];
    for (i = 0; id < 0 && i < 7u && s[i] && s[i] != ' '; i++)
        ;
    if (id < 0 && i && s[i] == ' ' && (id = kc_tag(s + i + 1u, &n)) >= 0) {
        char wd[8] = {0};
        uint32_t j;
        for (j = 0; j < i; j++)
            wd[j] = s[j];
        w = text_w(&AF_S, wd) + KH_GAP;
        s += i + 1u;
    }
    if (id >= 0 && kc_w((uint32_t)id) + KH_GAP < maxw - w) {
        w += kc_w((uint32_t)id) + KH_GAP;
        s += n;
        while (*s == ' ')
            s++;
        if (!*s)
            return w - KH_GAP;
    }
    text_fit(b, sizeof b, s, &AF_S, maxw - w);
    return w + text_w(&AF_S, b);
}

static int head_is_hint(const char *s)
{
    uint32_t n, i;
    if (kc_tag(s, &n) >= 0)
        return 1;
    for (i = 0; i < 7u && s[i] && s[i] != ' '; i++)
        ;
    return i && s[i] == ' ' && kc_tag(s + i + 1u, &n) >= 0;
}

static void head_msg(int32_t x, int32_t maxw)
{
    const char *m = ui.msg_t ? ui.msg : "";
    int lock = 0;
    int icon = m[0] == MSG_NOFILE[0];
    int32_t lw = lock ? 15 : 0, iw = icon ? 16 + KH_GAP : 0;
    int big = !head_is_hint(m + icon) && text_w(&AF_M, m + icon) <= maxw - lw - iw;
    if (x < 0)
        x = 120 - HALF_UP(iw + (big ? text_w(&AF_M, m + icon) : head_hint_w(m + icon, maxw - lw - iw)) + lw);
    if (icon)
        x += cv_icon_mid(x, H_HEAD / 2, 16, ICON_X_NOFILE, T_ACCENT, T_BG) + KH_GAP;
    if (big) {
        GFX_HOOK_ALIGN(0, 0, 0, H_HEAD, AL_V, "header text on its middle");
        x = cv_text(x, HEAD_MY, &AF_M, m + icon, T_TEXT);
    } else {
        x = cv_free_hint(x, HEAD_SY, m + icon, T_TEXT, T_BG, maxw - lw - iw);
    }
    if (lock)
        cv_icon_mid(x + 6, H_HEAD / 2, 16, ICON_X_LOCK, T_THEME, T_BG);
}

static int head_slot(char *b)
{
    uint32_t k = song.sel % NTRK;
    b[1] = b[2] = 0;
    if (!pat_from[k] || pat_from[k] > 4u)
        return 0;
    b[0] = (char)('A' + pat_from[k] - 1u);
    if (pat_from_sig(TSEL) != pat_from_h[k])
        b[1] = '*';
    return 1;
}

static void head_group(void)
{
    char b[8];
    int32_t x;
    {
        str_cpy(b, song.octave > 0 ? "+" : "", sizeof b);
        fmt_int(b + str_len(b), song.octave);
        x = HEAD_GRP_R - (text_w(&AF_S, "OCT") + 4 + text_w(&AF_M, b));
        GFX_HOOK_ALIGN(0, 0, 0, H_HEAD, AL_V, "header text on its middle");
        x = cv_text(x, HEAD_SY, &AF_S, "OCT", T_MID);
    }
    GFX_HOOK_ALIGN(0, 0, 0, H_HEAD, AL_V, "header text on its middle");
    cv_text(x + 4, HEAD_MY, &AF_M, b, T_THEME);
}
static void draw_head(void)
{
    static int16_t metro_x = -1;
    static int8_t metro_beat;
    char b[16];
    head_geo_t g;
    int32_t beat = -1;
    uint32_t rec = (song.rec >> song.sel) & 1u ? 2u : song.rec != 0u;
    char sl[3];
    int slot = head_slot(sl);
    uint32_t sig = (uint32_t)song.playing * 3u + rec * 5u + (uint32_t)(song.octave + 8) * 11u + song.sel * 13131u +
                   (ui.msg_t ? str_hash(7u, ui.msg) : 0u) + (uint32_t)song.g[G_BPM] * 101u + (ui.bpm_t != 0) * 31u +
                   (uint32_t)seq_counting() * 7u + (ui_prefs & PREF_BPM_LOCK) * 4099u +
                   (uint32_t)batt_shown() * 7777u + 0u +
                   (slot ? str_hash(11u, sl) : 0u) + (uint32_t)midi_lit(song.sel) * 15485863u;
    int msg = ui.msg_t;
    int centre = !msg || ui.bpm_t == LAYER_GLO;
    int32_t rx = HEAD_REC_X;
    if (song.g[G_BPM] != ui.roll_bpm) {
        char a[8];
        fmt_int(a, ui.roll_bpm);
        fmt_int(b, song.g[G_BPM]);
        roll_note(ROLL_BPM, a, b, ui.force);
        ui.roll_bpm = song.g[G_BPM];
    } else if (ui.force) {
        ui.roll[ROLL_BPM].from[0] = 0;
    }
    if (!centre)
        ui.roll[ROLL_BPM].from[0] = 0;
    fmt_int(b, song.g[G_BPM]);
    head_geo(&g, b);
    if (!ui.force && sig == ui.head_sig) {
        if (ui.roll[ROLL_BPM].from[0] && centre) {
            cv_begin((uint32_t)(g.sx1 - g.sx0), H_HEAD, T_BG);
            roll_text(ROLL_BPM, g.bx - g.sx0, HEAD_MY, b, ui.bpm_t ? T_ACCENT : T_THEME);
            cv_blit((uint32_t)g.sx0, Y_HEAD);
        }
        if (metro_x >= 0 && beat != metro_beat) {
            int32_t cx = g.mx0 - (g.sx0 - g.mx1);
            metro_beat = (int8_t)beat;
            cv_begin((uint32_t)(g.sx0 - cx), H_HEAD, T_BG);
            head_metro(g.mx0 - cx, beat);
            cv_blit((uint32_t)cx, Y_HEAD);
        }
        return;
    }
    ui.head_sig = sig;
    metro_x = -1;
    cv_begin(240, H_HEAD, T_BG);
    cv_icon_mid(HEAD_TRK_X, H_HEAD / 2, 16, trk_icon(song.sel, 1), midi_lit(song.sel) ? T_TEXT : T_ACCENT, T_BG);

    if (slot && (centre || !msg)) {
        char l[2] = {sl[0], 0};
        GFX_HOOK_ALIGN(0, 0, 0, H_HEAD, AL_V, "header text on its middle");
        rx = cv_text(HEAD_SLOT_X, HEAD_MY, &AF_M, l, sl[0] == '-' ? T_MID : T_THEME);
        if (sl[1])
            rx = cv_text(rx, HEAD_MY, &AF_M, "*", T_MID);
        rx += 1;
    }
    draw_rec_mark(rx, T_BG);
    if (centre) {
        int32_t ib[4], lx = g.bx + g.b[2];
        uint32_t id = song.playing ? ICON_X_PLAY : seq_counting() ? ICON_X_PLAY : ICON_X_STOP;
        icon_ink(16, id, ib);

        cv_icon_mid(g.px1 - ib[2], H_HEAD / 2, 16, id,
                    song.playing ? T_THEME : seq_counting() ? T_ACCENT : T_MID, T_BG);
        head_metro(g.mx0, beat);
        metro_x = (int16_t)g.mx0;
        metro_beat = (int8_t)beat;
        if (!ui.roll[ROLL_BPM].from[0])
            GFX_HOOK_ALIGN(0, 0, 0, H_HEAD, AL_V, "header text on its middle");
        roll_text(ROLL_BPM, g.bx, HEAD_MY, b, ui.bpm_t ? T_ACCENT : T_THEME);
        if (ui_prefs & PREF_BPM_LOCK) {
            icon_ink(16, ICON_X_LOCK, ib);
            cv_icon_mid(lx + HEAD_GAP - ib[0], H_HEAD / 2, 16, ICON_X_LOCK, T_MID, T_BG);
            lx += HEAD_GAP + ib[2] - ib[0];
        }
        if (msg) {
            int32_t x = lx + 9 > 142 ? lx + 9 : 142;
            head_msg(x, 236 - x);
        }
    } else {
        head_msg(-1, HEAD_MSG_W);
    }
    if (!msg && (song.octave))
        head_group();
    if (!(msg && centre))
        draw_battery(HEAD_BAT_X);
    cv_blit(0, Y_HEAD);
}

#define RULE_X0 CARD_X(0)
#define RULE_W (CARD_X(3) + CARD_W - CARD_X(0))

#ifndef CARD_HOOK
#define CARD_HOOK(v, u) ((void)0)
#endif

static uint32_t card_mot_mask[(MOTION_N + 31u) / 32u];
static uint8_t card_mot_next, card_mot;

typedef struct card_span {
    void (*fn)(const struct card_span *sp, int32_t v, char *val);
    const void *p;
    int32_t lo, hi, ctx;
} card_span_t;
static card_span_t card_span;

#ifndef LARGE_HOOK
#define LARGE_HOOK(s, why) ((void)0)

#endif
#define LG_MY 4
#define LG_MH 18
#define LG_LY 24
#define LG_VB 49
#define LG_UY 73
#define LG_GY 92

#define SPL_MIN_MS (TRAP_UX_TEST ? 120u : 400u)
#define SPL_END_MS (TRAP_UX_TEST ? 480u : 1840u)

static uint8_t spl_phase;
static void draw_splash(void)
{
    lcd_fill(0,0,240,240,0);lcd_sync();spl_phase=255u;ws_cached=~0u;
}
static int splash_step(uint32_t t)
{
    uint32_t phase=TRAP_UX_TEST ? (t>=300u?2u:t>=150u?1u:0u) : (t>=1600u?20u:t/80u);
    if(spl_phase==255u || phase>spl_phase) {
        for(uint32_t y=0;y<240u;y+=GB_STRIP) {
            cv_begin(240,GB_STRIP,0);cv_oy=-(int32_t)y;

            for(uint32_t f=0;f<=phase;f++)ws_image(WS_BOOT_00+f,0,0);
            cv_oy=0;cv_blit(0,y);lcd_sync();
        }
        spl_phase=(uint8_t)phase;
    }
    return t>=SPL_END_MS;
}

static void draw_uboot(void)
{
    static uint8_t shown;
    char d[4] = {(char)('0' + ui.uboot % 10u), 0, 0, 0};
    if (!ui.force && shown == ui.uboot)
        return;
    shown = ui.uboot;
    lcd_fill(0, H_HEAD, 240, 240 - H_HEAD, T_BG);
    ui.head_sig = ~0u;
    draw_text_box(0, 76, 240, &AF_M, "UPDATE MODE IN", T_TEXT, 1);
    draw_text_box(0, 102, 240, &AF_L, d, T_THEME, 1);
    {
        int32_t w = kc_w(KC_OCTDN) + 3 + kh_ink(KC_OCTUP, "LET GO TO CANCEL"), x = HALF_UP(240 - w);
        cv_begin(240, KC_H + 2, T_BG);
        GFX_HOOK_ALIGN(0, 0, 240, 0, AL_H | AL_N(3), "update-mode keys centred");
        x = cv_keycap(x, 1, KC_OCTDN, T_KEY, T_INK, T_BG) + 3;
        cv_key_hint(x, 1, KC_OCTUP, "LET GO TO CANCEL", 1, T_BG);
        cv_blit(0, 149);
        lcd_sync();
    }
    ui.force = 0;
}

#define CI_Y 88
#define CI_H 72

#define DLG_X 16
#define DLG_Y 62
#define DLG_W 208
#define DLG_H 116
static void confirm_text(char *a, char *b)
{
    uint32_t k = ui.confirm_trk;
    b[0] = 0;
    switch (ui.confirm) {
    case CF_TRAPGEN:
        str_cpy(a, "REPLACE T1-T5 STEPS?", 24);
        str_cpy(b, "MAKE TRAP PATTERN", 24);
        break;
    case CF_HATGEN:
        str_cpy(a, "REPLACE T1 STEPS?", 24); a[9] = (char)('1' + k % NTRK);
        str_cpy(b, "MAKE HIHAT PATTERN", 24);
        break;
    case CF_EUCLID:
        str_cpy(a,"REPLACE T1 STEPS?",24);a[9]=(char)('1'+k%NTRK);
        str_cpy(b,"MAKE EUCLID PATTERN",24);break;
    case CF_PHONK:
        str_cpy(a, "REPLACE T1 STEPS?",24); a[9]=(char)('1'+k%NTRK);
        str_cpy(b,ph_request==PH_FMAKE?"FILL: LAST FOUR ONLY":"MAKE PHONK PATTERN",24);
        break;
    case CF_CHORD_GEN:
        str_cpy(a, "REPLACE T1 STEPS?", 24); a[9] = (char)('1' + k % NTRK);
        str_cpy(b, "MAKE CHORD PATTERN", 24);
        break;
    case CF_CLEAR_TRK:
        str_cpy(a, "CLEAR TRACK 1?", 24);
        a[12] = (char)('1' + k % NTRK);
        break;
    case CF_INIT_SOUND:
        str_cpy(a, "INITIALIZE SOUND?", 24);
        break;
    case CF_CLEAN_SOUND:
        str_cpy(a, "CLEAN INIT SOUND?", 24);
        str_cpy(b, "NO FX, NO MOD", 24);
        break;
    case CF_CLEAR_MOTION:
        str_cpy(a, "CLEAR T1 AUTOMATION?", 24); a[7] = (char)('1' + k % NTRK);
        break;
    default:
        str_cpy(a, "CLEAR T1 SEQUENCE?", 24);
        a[7] = (char)('1' + k % NTRK);
        break;
    }
}
static void draw_confirm(void)
{
    char a[24], b[24];
    const aafont_t *tf = &AF_M;
    confirm_text(a, b);
    lcd_fill(0, H_HEAD, 240, 240 - H_HEAD, T_BG);
    ui.head_sig = ~0u;
    cv_begin(DLG_W, DLG_H, T_BG);
    if (ux.style)
        cv_rrect(0, 0, DLG_W, DLG_H, 8, T_RULE, T_BG);
    cv_rrect(!!ux.style, !!ux.style, DLG_W - 2 * !!ux.style, DLG_H - 2 * !!ux.style, 8 - !!ux.style, T_SURF,
             ux.style ? T_RULE : T_BG);
    GFX_HOOK_ALIGN(0, 0, DLG_W, 0, AL_H, "dialog icon centred");
    cv_icon_in(0, 12, DLG_W, 0, 16, ui.confirm == CF_CLEAR_MOTION ? ICON_X_MOTION_DEL : ICON_X_WARN, T_ACCENT, T_SURF);
    if (text_w(tf, a) > DLG_W - 16)
        tf = &AF_S;
    GFX_HOOK_ALIGN(0, 0, DLG_W, 0, AL_H, "dialog question centred");
    cv_text_in(0, 34, DLG_W, tf, a, T_TEXT, T_SURF);
    if (b[0]) {
        char f[32];
        text_fit(f, sizeof f, b, &AF_M, DLG_W - 16);
        GFX_HOOK_ALIGN(0, 0, DLG_W, 0, AL_H, "dialog detail centred");
        cv_text_flags(ink_in(&AF_M, f, DLG_W), 55, &AF_M, f, T_MID, T_SURF, 8u | (f[str_len(f) - 1u] == ELLIPSIS));
    }
    cv_rrect(10, 80, 90, 26, 6, T_RAISE, T_SURF);
    GFX_HOOK_ALIGN(10, 80, 100, 106, AL_HV | AL_N(2), "dialog button NO");
    cv_key_hint(10 + HALF_UP(90 - kh_ink(KC_OCTDN, "NO")), 80 + HALF_UP(26 - KC_H), KC_OCTDN, "NO", 1, T_RAISE);
    cv_rrect(108, 80, 90, 26, 6, T_THEME, T_SURF);
    {
        int32_t x = 108 + HALF_UP(90 - kh_ink(KC_OCTUP, "YES"));
        GFX_HOOK_ALIGN(108, 80, 198, 106, AL_HV | AL_N(2), "dialog button YES");
        x = cv_keycap(x, 80 + HALF_UP(26 - KC_H), KC_OCTUP, T_INK, T_THEME, T_THEME);
        cv_text_on(x + KH_GAP, 80 + HALF_UP(26 - KC_H) - 1, &AF_S, "YES", T_INK, T_THEME);
    }
    cv_blit(DLG_X, DLG_Y);
}

static void style_apply(void)
{
    if (ui_style > ST_LINE)
        ui_style = ui_style == 2u ? ST_LINE : ST_FLAT;
    if (ux.style != ui_style) {
        ux.style = ui_style;
        palette_apply();
        ui.force = 1;
    }
}

static void ui_draw_page(uint32_t counting);
static void ui_draw(void)
{
    static uint8_t counting;
    style_apply();
    ui.frame++;
    if (counting != (uint8_t)seq_counting()) {
        counting = (uint8_t)seq_counting();
        ui.force = 1;
    }
    if (!scr_frame())
        return;
    ui_draw_page(counting);
    scr_shown();
}
static void ui_draw_page(uint32_t counting)
{
    (void)counting;
    if (ui.uboot) {
        draw_uboot();
        draw_head();
        return;
    }
    if (ui.confirm) {
        (void)draw_confirm;
        gb_draw();
        ui.force = 0;
        return;
    }
    if (ui.home) {
        open_family(FAM_EDIT);
        ui.force = 1;
    }
    cursor_fix();
    if (!page_visible(ui.page))
        ui.page = (uint8_t)page_first(PAGES[ui.page % NPAGES].fam);
    gb_draw();
    if (ui.hot_t)
        ui.hot_t--;
    if (ui.bpm_t)
        ui.bpm_t--;
    if (ui.msg_t)
        ui.msg_t--;
    ui.force = 0;
}

/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION contributors */

static uint8_t sg_mv[NTRK + 1];
static uint32_t sg_meter(int32_t pk)
{
    uint32_t b = 0, a = (uint32_t)(pk < 0 ? 0 : pk);
    while (a >>= 1)
        b++;
    return b < 10u ? 0u : b - 9u > 6u ? 6u : b - 9u;
}
static void sg_fill(gb_view_t *v)
{
    uint32_t k, n = 0, on = sg_keys | sg_lat, mx = 0;
    v->stg = 1;
    v->stg_tgt = sg_tgt;
    for (k = 0; k < NTRK; k++) {
        uint32_t m = sg_meter(trk[k].peak);
        trk[k].peak = 0;
        sg_mv[k] = (uint8_t)(m > sg_mv[k] ? m : sg_mv[k] ? sg_mv[k] - 1u : 0u);
        v->stg_lvl[k] = sg_mv[k];
        mx = sg_mv[k] > mx ? sg_mv[k] : mx;
        v->stg_trk[n++] = (uint8_t)k;
        if (trk[k].p[P_MUTE])
            v->stg_mute |= (uint8_t)(1u << k);
    }
    v->stg_lvl[NTRK] = (uint8_t)mx;
    v->stg_n = (uint8_t)n;
    v->stg_on = (uint8_t)on;
    v->stg_lat = sg_lat;
    for (k = 0; k < SG_N; k++) {
        v->stg_ch[k] = sg_fx_ch[k];
        if ((on >> k) & 1u) {
            v->stg_chfx |= (uint8_t)(sg_mask(sg_fx_ch[k]) | (sg_fx_ch[k] >= NTRK ? 1u << NTRK : 0u));
            v->stg_ramp[k] = (uint8_t)(k == SG_SWEEP || k == SG_WASH || k == SG_RISE ? sg_ramp(k) / 10u : 0u);
        }
    }
    {
        static const char *const LB[4] = {"FILTER", "TUNE", "FDBK", "DLY"};
        static const uint8_t GK[2] = {G_CRATE, G_CDEPTH};
        int32_t f = (sg_dj_m >> sg_tgt) & 1u ? sg_dj_b[sg_tgt] : sg_dj_get(sg_tgt);
        const char *u;
        v->mask = 15;
        for (k = 0; k < 4u; k++)
            str_cpy(v->lab[k], LB[k], sizeof v->lab[k]);
        if (f < -1 || f > 1) {
            str_cpy(v->val[0], f < 0 ? "LP " : "HP ", sizeof v->val[0]);
            fmt_int(v->val[0] + 3, f < 0 ? -f : f);
        } else
            str_cpy(v->val[0], "OPEN", sizeof v->val[0]);
        v->ratio[0] = (int16_t)((f + 64) * 1000 / 127);
        for (k = 1; k < 3u; k++) {
            uint32_t g = GK[k - 1u];
            int32_t x = k == 1u && sg_tune_m ? sg_cb_tune : song.g[g];
            param_format(&GP[g], x, v->val[k], &u);
            v->ratio[k] = (int16_t)RATIO(&GP[g], x);
        }
        {
            int32_t x = sg_dly_get(sg_tgt);
            param_format(&TP[P_DLY], x, v->val[3], &u);
            v->ratio[3] = (int16_t)RATIO(&TP[P_DLY], x);
        }
    }
    v->kmode = 0;
}

static void sg_chname(char *b, uint32_t ch, const gb_view_t *v)
{
    const char *p;
    if (ch >= NTRK) {
        str_cpy(b, "MASTER", 8);
        return;
    }
    p = MACHINES[v->tm[ch] % NMACH].pill;
    if (p[0] == '8')
        p++;
    else if (p[0] == 'F' && p[1] == 'M')
        p += 2;
    b[0] = 'T';
    b[1] = (char)('1' + ch);
    b[2] = ' ';
    str_cpy(b + 3, p, 6);
}
static void sg_meter_draw(int32_t x, int32_t y, uint32_t lvl, uint16_t on, uint16_t off)
{
    uint32_t i;
    for (i = 0; i < 6u; i++)
        cv_rect(x + (int32_t)i * 5, y + 8 - (int32_t)i, 3, 3 + (int32_t)i, i < lvl ? on : off);
}
static void sg_cell(const gb_view_t *v, uint32_t ch, int32_t x, int32_t y, int32_t w)
{
    int pick = v->stg_tgt == ch, mute = ch < NTRK && ((v->stg_mute >> ch) & 1u), fx = (v->stg_chfx >> (ch < NTRK ? ch : NTRK)) & 1u;
    uint16_t c = pick ? SD_RED : SD_PANEL, ink = mute ? SD_MID : SD_WHITE;
    char b[12];
    sg_chname(b, ch, v);
    cv_rrect(x, y, w, 15, 3, c, SD_BLACK);
    cv_text_on(x + 5, y, &AF_S, b, ink, c);
    if (mute)
        cv_text_r(x + w - 6, y, &AF_S, "M", SD_LIGHT, c);
    else
        sg_meter_draw(x + w - 34, y + 2, v->stg_lvl[ch < NTRK ? ch : NTRK], pick ? SD_WHITE : SD_RED, pick ? GB_RGB(150, 34, 44) : SD_DARK);
    if (fx)
        cv_rect(x + 4, y + 13, w - 8, 2, SD_WHITE);
}
static void tl_stage(const gb_view_t *v, uint16_t bg)
{
    uint32_t i, k;
    char b[16];
    (void)bg;
    cv_rect(0, 60, 240, 154, SD_BLACK);
    sd_header(v);
    b[0] = '>';
    b[1] = ' ';
    sg_chname(b + 2, v->stg_tgt, v);
    cv_rrect(140, 46, 90, 13, 3, SD_RED, SD_BLACK);
    cv_text_in(140, 46, 90, &AF_S, b, SD_WHITE, SD_RED);
    if (!v->stg_n)
        cv_text_in(6, 82, 228, &AF_S, "NO ACTIVE TRACKS", SD_MID, SD_BLACK);
    for (i = 0; i < v->stg_n && i < NTRK; i++)
        sg_cell(v, v->stg_trk[i], 6 + (int32_t)(i & 1u) * 116, 63 + (int32_t)(i >> 1) * 17, 112);
    sg_cell(v, NTRK, 6, 114, 228);
    cv_rect(6, 133, 228, 1, GB_RGB(82, 82, 82));
    for (k = 0; k < SG_N; k++) {
        int32_t x = 6 + (int32_t)(k & 3u) * 57, y = 137 + (int32_t)(k >> 2) * 17, on = (v->stg_on >> k) & 1u;
        uint16_t c = on ? SD_RED : SD_PANEL;
        b[0] = (char)('1' + k);
        b[1] = ' ';
        str_cpy(b + 2, SG_NAME[k], sizeof b - 2u);
        if ((v->stg_lat >> k) & 1u) {
            cv_rrect(x, y, 54, 15, 3, SD_WHITE, SD_BLACK);
            cv_rrect(x + 1, y + 1, 52, 13, 2, c, SD_WHITE);
        } else
            cv_rrect(x, y, 54, 15, 3, c, SD_BLACK);
        cv_text_in(x, y, 54, &AF_S, b, on ? SD_WHITE : SD_LIGHT, c);
        if (on && v->stg_ramp[k])
            cv_rect(x + 3, y + 13, (int32_t)v->stg_ramp[k] * 48 / 100, 1, SD_WHITE);
    }
    sd_controls(v, 1);
    tl_footer(v, SD_BLACK);
    cv_text_on(10, 221, &AF_S, "1-8 HOLD  9-16 LATCH", SD_MID, SD_BLACK);
}

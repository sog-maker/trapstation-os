/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION contributors
 * Approved 09.11.2026: native 240 px adaptations inside the final cassette frame.
 * Display only; no effect parameters, preset IDs or DSP state are changed here. */
static void d99_delay(const gb_view_t *v, uint16_t bgc)
{
    int32_t c, r, y = DZ_Y, head = v->echo_ph % 12u;
    dz_panel(10, y, 220, DZ_H, 0, bgc);
    cv_text_on(21, y + 5, &AF_S, "ECHO MATRIX", DZ_CREAM, DZ_PANEL);
    cv_rect(20, y + 23, 148, 66, DZ_DGOLD);
    for (r = 0; r < 6; r++)
        for (c = 0; c < 12; c++) {
            int32_t gap = 1 + tl_r(v, 0) * 3 / 1001;
            int32_t col = (head + r * gap) % 12;
            uint16_t ink = DZ_SCREEN;
            if (v->play && c == col) {
                int32_t energy = 255;
                int32_t k;
                for (k = 0; k < r; k++) energy = energy * (250 + tl_r(v, 1) * 700 / 1000) / 1000;
                ink = GB_RGB(energy, energy * (650 + tl_r(v, 2) / 3) / 1000, energy / 3);
            }
            cv_rect(21 + c * 12, y + 24 + r * 10, 11, 9, ink);
        }
    cv_rect(21, y + 91, 146, 1, DZ_DGOLD);
    for (c = 0; c < 12; c++)
        cv_rect(24 + c * 12, y + 100 - (12 - c) / 2, 2, (12 - c) / 2 + 1, DZ_GOLD);
    for (r = 0; r < 3; r++) {
        dz_knob(184, y + 21 + r * 30, 8, v->ratio[r], v->hot == (uint32_t)r + 1u);
        cv_text_on(198, y + 15 + r * 30, &AF_S, v->lab[r], DZ_CREAM, DZ_PANEL);
    }
}

static void d99_scenes(const gb_view_t *v, uint16_t bgc)
{
    uint32_t r, k, live = v->sc_live & 1u;
    dz_panel(10, DZ_Y, 220, DZ_H, 0, bgc);
    for (k = 0; k <= NTRK; k++) {
        char b[3] = {k < NTRK ? 'T' : 'F', k < NTRK ? (char)('1' + k) : 'X', 0};
        cv_text_in(74 + (int32_t)k * 20, DZ_Y + 5, 20, &AF_S, b, DZ_CREAM, DZ_PANEL);
    }
    for (r = 0; r < 5u; r++) {
        int32_t y = DZ_Y + 23 + (int32_t)r * 15;
        int on = r < 4u && (int32_t)r == v->cur[0];
        uint32_t m = r < 4u ? v->sc_m[r] : ((~(uint32_t)v->tmute) & 63u) | ((v->sc_live & 2u) ? 64u : 0u);
        uint16_t rc = on ? DZ_OX : DZ_CREAM;
        if (r == 4u) cv_rect(19, y - 2, 202, 1, DZ_GOLD);
        cv_rect(19, y, 52, 13, rc);
        cv_text_in(19, y, 52, &AF_S, r < 4u ? N_PHSCENE[r] : "LIVE", on ? DZ_CREAM : DZ_INK, rc);
        for (k = 0; k <= NTRK; k++) {
            int lit = k < NTRK ? (m >> k) & 1u : !((m >> 6) & 1u);
            if (r == 4u && !live) lit = 0;
            cv_rect(75 + (int32_t)k * 20, y, 18, 13, on ? DZ_LED : DZ_DGOLD);
            cv_rect(76 + (int32_t)k * 20, y + 1, 16, 11,
                    lit ? k < NTRK ? gb_col[k] : GB_RGB(138, 144, 150) : DZ_CREAM);
        }
    }
    cv_text_r(228, 38, &AF_S, live ? "KEYS: MUTE" : "KEYS: SCENE", T_THEME, bgc);
}

static void d99_mix(const gb_view_t *v, uint16_t bgc)
{
    const page_t *pg = cur_page();
    uint32_t j;
    char title[12] = "MIX 1-2";
    title[4] = (char)('1' + ((pg->id[0] & ~MIX_PAN) % NTRK));
    title[6] = (char)(title[4] + 1);
    dz_panel(10, DZ_Y, 220, DZ_H, 0, bgc);
    cv_rect(23, DZ_Y + 6, 194, 15, DZ_SCREEN);
    cv_text_in(23, DZ_Y + 6, 194, &AF_S, title, DZ_GOLD, DZ_SCREEN);
    for (j = 0; j < 2u; j++) {
        uint32_t tk = (pg->id[2u*j] & ~MIX_PAN) % NTRK;
        int32_t x = 22 + (int32_t)j * 102, y = DZ_Y + 25;
        int32_t fy = y + 51 - tl_r(v, j * 2u) * 32 / 1000;
        int32_t px = x + 37 + tl_r(v, j * 2u + 1u) * 44 / 1000;
        char label[4] = {'T', (char)('1' + tk), 0, 0};
        cv_rrect(x, y, 96, 72, 3, tk == v->sel ? DZ_LED : DZ_DGOLD, DZ_PANEL);
        cv_rect(x + 2, y + 2, 92, 68, DZ_CREAM);
        cv_text_on(x + 7, y + 3, &AF_S, label, DZ_INK, DZ_CREAM);
        cv_text_r(x + 89, y + 3, &AF_S, v->val[j * 2u], DZ_INK, DZ_CREAM);
        cv_rect(x + 13, y + 22, 3, 40, DZ_INK);
        cv_rect(x + 7, fy, 15, 7, gb_col[tk]);
        cv_rect(x + 37, y + 40, 45, 2, DZ_DGOLD);
        cv_rect(x + 59, y + 35, 1, 12, DZ_INK);
        cv_rect(px - 3, y + 36, 7, 10, DZ_OX);
        cv_text_in(x + 31, y + 50, 60, &AF_S, v->val[j * 2u + 1u], DZ_INK, DZ_CREAM);
    }
}

static void d99_hat(const gb_view_t *v, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t f)
{
    int32_t i, cx = x + w / 2, ry = y + h * 59 / 100;
    int32_t radius = h * 19 / 100, mh = h / 4, cy = y + h / 5;
    const uint16_t wood = GB_RGB(86, 38, 26);
    const uint32_t stop[3] = {14, 19, 24};
    cv_rrect(x + 5, y + 6, w - 10, h - 7, h / 4, DZ_DGOLD, T_BG);
    cv_rrect(x + 8, y + 9, w - 16, h - 13, h / 4, DZ_CREAM, DZ_DGOLD);
    for (i = 0; i < 2; i++) {
        int32_t xx = i ? x + w - 24 : x + 12, j;
        cv_rrect(xx, y + h/3, 14, h * 3/5, 5, wood, DZ_CREAM);
        for (j = 0; j < 3; j++) cv_rect(xx + 3 + j*3, y + h/3 + 3, 1, h/2, DZ_GOLD);
    }
    cv_rrect(x + 29, y + 12, w - 58, mh, 10, DZ_GOLD, DZ_CREAM);
    cv_rrect(x + 32, y + 14, w - 64, mh - 4, 9, DZ_OX, DZ_GOLD);
    cv_text_in(x + 30, y + 15, w - 60, &AF_S, "HIHAT JACKPOT", DZ_CREAM, DZ_OX);
    fxg_disc(cx, y + 8, 7, DZ_GOLD);
    fxg_disc(cx, y + 8, 5, DZ_OX);
    cv_rect(cx - 4, y + 7, 9, 2, DZ_GOLD);
    for (i = 0; i < 3; i++) {
        int32_t rx = cx + (i - 1) * (radius * 2 + 8);
        fxg_disc(rx, ry, radius + 2, DZ_DGOLD);
        fxg_disc(rx, ry, radius, DZ_GOLD);
        fxg_disc(rx, ry, radius - 2, DZ_CREAM);
        if (f && f < stop[i]) {
            int32_t dy = (int32_t)((f * 5u + (uint32_t)i * 3u) % (uint32_t)(radius * 2 - 10)) - radius + 5;
            cv_rect(rx - radius/2, ry + dy, radius, 3, DZ_INK);
            cv_rect(rx - radius/2, ry - dy, radius, 2, DZ_DGOLD);
        } else dz_reel_sym(TL_AN_HAT, (uint32_t)i, rx, ry, h > 140 ? 3 : 1, v, DZ_CREAM);
    }
    cv_rrect(cx - w/3, y + h - 18, w*2/3, 11, 5, wood, DZ_CREAM);
    for (i = 0; i < 5; i++) fxg_disc(cx - 24 + i*12, y + h - 13, 2, (f/3u)%5u == (uint32_t)i ? DZ_GOLD : DZ_CREAM);
    (void)cy;
}

static void d99_clock(const gb_view_t *v, uint16_t bgc)
{
    int32_t i;
    char b[12];
    dz_panel(10, DZ_Y, 220, DZ_H, "MASTER CLOCK", bgc);
    cv_rect(25, DZ_Y + 24, 116, 42, GB_RGB(192, 150, 63));
    fmt_int(b, v->bpm);
    cv_text_in(26, DZ_Y + 26, 114, &AF_L, b, DZ_INK, GB_RGB(192, 150, 63));
    cv_text_in(26, DZ_Y + 51, 114, &AF_S, "BPM", DZ_INK, GB_RGB(192, 150, 63));
    for (i = 0; i < 4; i++) {
        int on = v->play && (uint32_t)i == v->clk_beat;
        fxg_disc(154 + i*17, DZ_Y + 34, 5, on ? DZ_LED : DZ_GRID);
    }
    cv_text_on(150, DZ_Y + 50, &AF_S, v->play ? "RUN" : "STOP", DZ_GOLD, DZ_PANEL);
    cv_text_on(23, DZ_Y + 72, &AF_S, "SWG", DZ_CREAM, DZ_PANEL);
    cv_rect(58, DZ_Y + 85, 155, 1, DZ_GOLD);
    for (i = 0; i < 16; i++) cv_rect(58 + i*10 + ((i&1) ? tl_r(v,1)*3/1000 : 0), DZ_Y+77, 2, (i&3) ? 6 : 10, DZ_GOLD);
}

static void d99_seq(const gb_view_t *v, uint16_t bgc)
{
    uint32_t i;
    dz_panel(10, DZ_Y, 220, DZ_H, 0, bgc);
    for (i = 0; i < 16; i++) {
        int32_t x = 21 + (int32_t)(i&7u)*25, y = DZ_Y + 9 + (int32_t)(i>>3)*46;
        int in = i < v->sq_len, hit = (v->sq_hit>>i)&1u;
        uint16_t c = !in ? DZ_GRID : hit ? ((v->sq_acc>>i)&1u ? DZ_LED : DZ_GOLD) : DZ_CREAM;
        char n[4]; fmt_int(n, v->sq_at + (int32_t)i + 1);
        cv_rrect(x-1,y-1,23,33,3, v->sq_pos == i+1u ? DZ_LED : DZ_DGOLD,DZ_PANEL);
        cv_rrect(x,y,21,31,3,c,DZ_DGOLD);
        cv_text_in(x,y+7,21,&AF_S,n,in ? DZ_INK : DZ_CREAM,c);
        if (v->sq_pos == i+1u) cv_rect(x,y+35,21,3,DZ_LED);
    }
}

static void d99_verb(const gb_view_t *v, uint16_t bgc)
{
    int32_t x, y = DZ_Y + 24, w = 136, h = 63;
    dz_panel(10,DZ_Y,220,DZ_H,"REFLECTION ROOM",bgc);
    cv_rect(23,y,w,h,DZ_SCREEN);
    cv_line_t(24,y+1,24+w-2,y+h-2,DZ_GRID,1);
    cv_line_t(24,y+h-2,24+w-2,y+1,DZ_GRID,1);
    for (x = 0; x < 12; x++) {
        int32_t hh = (h-6)*(12-x)/12*(1000-tl_r(v,1)*x/16)/1000;
        int32_t xx = 27 + x*(5+tl_r(v,0)*5/1000);
        if (xx < 156) cv_rect(xx,y+h-3-hh,2,hh,x ? DZ_GOLD : DZ_CREAM);
    }
    for (x = 0; x < 3; x++) {
        dz_knob(179,DZ_Y+29+x*28,8,v->ratio[x],v->hot==(uint32_t)x+1);
        cv_text_on(193,DZ_Y+23+x*28,&AF_S,v->lab[x],DZ_CREAM,DZ_PANEL);
    }
    if (v->val[0][0]=='F') cv_text_in(24,y+22,134,&AF_S,"FREEZE",DZ_CREAM,DZ_SCREEN);
}

static void d99_bootmenu(const gb_view_t *v, uint16_t bgc)
{
    static const char *const opt[3] = {"NEW", "LOAD", "RESUME"};
    gb_view_t c = *v;
    uint32_t i;
    cv_rect(0,0,240,TL_TOP,T_ACCENT);
    cv_rect(0,TL_TOP,240,3,T_THEME);
    cv_text_on(8,4,&AF_S,"TRAPSTATION",T_SURF,T_ACCENT);
    cv_text_r(232,4,&AF_S,FELUCCA_VERSION,T_SURF,T_ACCENT);
    str_cpy(c.title,v->bm==2u ? "LOAD KIT" : v->bm==3u ? "NEW PROJECT" : "START",sizeof c.title);
    tl_title(&c,bgc);
    if (v->bm==3u) {
        dz_panel(14,66,212,138,"MEMPHIS 808",bgc);
        for(i=0;i<16;i++) {
            int32_t x=37+(int32_t)(i%4u)*43,y=91+(int32_t)(i/4u)*25;
            uint16_t col = i < v->bm_left ? DZ_GOLD : DZ_GRID;
            cv_rrect(x,y,36,20,3,col,DZ_PANEL);
        }
        cv_text_in(14,191,212,&AF_S,v->bm_left>=16u ? "READY" : "STARTING BANK",DZ_CREAM,DZ_PANEL);
    } else if(v->bm==2u) {
        dz_panel(12,65,216,143,0,bgc);
        for(i=0;i<11;i++) {
            int32_t x=20+(int32_t)(i/6u)*103,y=73+(int32_t)(i%6u)*21;
            int on=i==v->bm_kit, used=(v->bm_used>>i)&1u;
            char b[16]; const char *nm=trap_kit_name(i);
            if(!nm) { str_cpy(b,"KIT ",sizeof b); fmt_int(b+4,(int32_t)i+1); nm=b; }
            cv_rect(x,y,98,19,on ? DZ_OX : DZ_CREAM);
            cv_text_in(x,y+2,98,&AF_S,used ? nm : "-",on ? DZ_CREAM : DZ_INK,on ? DZ_OX : DZ_CREAM);
        }
    } else {
        dz_panel(12,65,216,143,"STUDIO CONSOLE",bgc);
        for(i=0;i<3;i++) {
            int32_t y=94+(int32_t)i*34; int on=i==v->bm_sel;
            cv_rrect(25,y,190,29,4,on ? DZ_OX : DZ_CREAM,DZ_PANEL);
            fxg_disc(37,y+14,3,on ? DZ_LED : DZ_DGOLD);
            cv_text_on(49,y+3,&AF_L,opt[i],on ? DZ_CREAM : DZ_INK,on ? DZ_OX : DZ_CREAM);
        }
    }
    cv_rect(10,TL_FY,220,1,T_LINE);
    if(v->bm==1u && v->bm_left) {
        cv_rect(10,222,220,4,T_LINE); cv_rect(10,222,(int32_t)v->bm_left*11,4,T_THEME);
    } else cv_text_in(0,221,240,&AF_S,v->bm==2u ? "SELECT OCT+ LOAD OCT- BACK" : v->bm==3u ? "" : "SELECT OCT+ OK",T_MID,bgc);
}

static void d99_save(const gb_view_t *v, uint16_t bgc)
{
    uint32_t i;
    tl_status(v);
    cv_text_on(10,27,&AF_L,v->sv==1u ? "SAVE?" : "LOAD?",GB_W,bgc);
    cv_rect(10,56,26,3,T_THEME);
    cv_text_r(230,34,&AF_M,GB_POOL_NAME[gb_pool(v->mach)],T_MID,bgc);
    dz_panel(12,DZ_Y,216,DZ_H,0,bgc);
    if(v->flash) {
        if(v->flash==1u) {
            cv_text_in(22,DZ_Y+26,196,&AF_M,v->msg[0],DZ_CREAM,DZ_PANEL);
            cv_text_in(22,DZ_Y+53,196,&AF_M,v->msg[1],DZ_GOLD,DZ_PANEL);
        }
    } else {
        cv_text_on(23,DZ_Y+7,&AF_S,"SOUND / WHITE KEYS",DZ_CREAM,DZ_PANEL);
        for(i=0;i<16u;i++) {
            int32_t x=23+(int32_t)i*12; uint16_t col=(v->su>>i)&1u ? DZ_GOLD : DZ_CREAM;
            cv_rect(x,DZ_Y+27,10,21,col);
        }
        cv_text_on(23,DZ_Y+54,&AF_S,"KIT / BLACK KEYS",DZ_CREAM,DZ_PANEL);
        for(i=0;i<11u;i++) {
            int32_t x=23+(int32_t)i*18; int used=(v->ku>>i)&1u;
            cv_rrect(x,DZ_Y+74,15,16,2,used ? DZ_OX : DZ_GRID,DZ_PANEL);
            if(used) cv_rect(x+3,DZ_Y+79,9,2,DZ_CREAM);
        }
    }
    cv_text_in(0,TL_CY+5,240,&AF_S,v->sv==1u ? "HOLD SAVE + A KEY" : "TAP A KEY TO LOAD",GB_W,bgc);
    cv_text_in(0,TL_CY+25,240,&AF_S,"GOLD / RED = USED",T_MID,bgc);
    tl_footer(v,bgc);
}

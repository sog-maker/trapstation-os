/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Blair (blairysbeatlab), BBL JAM BOX */

#define GB_STRIP 60

#define GB_W T_TEXT
#define GB_K T_BG
#define GB_HDR 44
#define GB_RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))

static uint16_t gb_col[NTRK] = {GB_RGB(255, 42, 42), GB_RGB(253, 207, 61), GB_RGB(30, 155, 255), GB_RGB(68, 200, 147),
                                GB_RGB(138, 144, 150), GB_RGB(196, 2, 8)};

typedef struct {
    uint8_t perf, mixer, mach, sel, ring, page, npages, mask, fade, rpt, hn, sv, flash;
    char msg[2][16];
    int16_t bpm;
    uint8_t tm[NTRK];
    uint8_t mac_n[8], mac_held[8];
    uint8_t xf_a, xf_b, xf_x;
    int16_t ratio[4];
    uint8_t alt[4];
    int16_t dj, cmix, ctune;
    uint8_t fill[8], rel, bpm_alt;
    uint8_t kmode, pk, fx, cross;

    uint8_t spg, snp;
    uint8_t nud;
    uint8_t rec;
    uint8_t thit, tmute;
    uint16_t brd;
    uint8_t icon, picon, hit, an;
    uint8_t vic[4];
    uint8_t wv, wa, wb;
    uint16_t wkey;
    uint8_t sl_pg, sl_n, sl_pn, sl_hd, sl_last;

    uint8_t sl_c[17], sl_p[17];
    uint8_t zoom;
    uint32_t za, zb;
    uint8_t tmod;
    int8_t tm_amt;
    uint8_t tm_src;
    uint8_t temp;
    uint8_t fillm;
    uint8_t hint;
    char note[24];
    uint8_t ban;
    uint8_t bn, bpg;
    char bt[8][20];
    char tm_name[8];
    char pname[8];
    char title[16], lab[4][8], val[4][12];
    uint8_t fxg, fxang;
    uint8_t echo_ph;
    uint8_t play, hot;
    int16_t cur[4];
    uint16_t sq_hit, sq_acc;
    uint8_t sq_pos, sq_len, sq_at;
    uint8_t sq_note[16], sq_cn[16];
    uint16_t ov_hit[NTRK];
    uint8_t ov_pos[NTRK], ov_len[NTRK], ov_bar;
    uint8_t smp_row, mask2, wp, vic2[4];
    char lab2[4][8], val2[4][12];
    int8_t dj_v[NTRK + 1];
    uint8_t dj_sel, menu_f;
    uint8_t pr_note[16][4], pr_n[16];
    uint16_t pr_tie;
    uint8_t pr_pos, pr_bar, pr_len;
    uint8_t pr_lv[8], pr_ln;
    uint8_t clk_beat, clk_ph, clk_step;
    uint8_t bm, bm_sel, bm_kit, bm_left;
    uint16_t bm_used;
    uint8_t an_k, an_f;
    uint16_t su, ku;
    uint8_t ov_m[NTRK], ov_nbar, ov_len_all;
    uint16_t ov_fx;
    uint8_t ov_fxlen, ov_fxpos;
    uint8_t pr_mode, sig_step;
    uint8_t pr_sel, pr_selm;
    uint8_t sc_live, sc_m[4];
    uint8_t hg_trk;
    uint8_t cfm;
    char cfa[24], cfb[24];
    uint8_t stg, stg_tgt, stg_n, stg_trk[NTRK];

    uint8_t stg_lvl[NTRK + 1], stg_mute;
    uint8_t stg_on, stg_lat, stg_chfx;
    uint8_t stg_ch[8], stg_ramp[8];
    uint8_t qf_at, qf_n, qf_type;
    uint8_t gl_mode, gl_cat;
    uint16_t gl_idx, gl_cur;
} gb_view_t;
static void tl_fill(gb_view_t *v, const page_t *pg);
static void sg_fill(gb_view_t *v);
static void confirm_text(char *a, char *b);

static uint8_t gen_amount[4] = {65,65,65,65};
static uint32_t gen_index(const page_t *pg)
{
    if (pg->scope == SC_TRAPGEN) return 0;
    if (pg->scope == SC_HATGEN) return 1;
    if (pg->scope == SC_PHONK && pg->id[3] == PH_BMAKE) return 2;
    if (pg->scope == SC_PHONK && pg->id[3] == PH_JMAKE) return 3;
    return 4;
}
static uint8_t pr_single;
static uint8_t pr_sel, pr_selm;
static uint32_t qf_at(void);
static void gl_fill(gb_view_t *v);
static void tl_gl(const gb_view_t *v);
static int gl_enc(uint32_t en, int32_t s);
static int gl_knob(uint32_t k, int32_t s);
static void gl_poll(void);
static uint8_t ou_eat;
static uint8_t tl_menu_f, tl_menu_b;
static uint8_t bm_on;
static void bm_fill(gb_view_t *v);
static gb_view_t gb_v, gb_last;
static uint8_t gb_have;

static int name_on(void);
static uint64_t gbs_held(void);
static int acid_pop(void);
static uint32_t gbsv_leds(void);
static int gbs_used(uint32_t mach, uint32_t slot);
static int gbk_used(uint32_t k);
static int gbs_save_sound(uint32_t slot);
static int gbs_load_sound(uint32_t slot);
static int gbk_save(uint32_t k);
static int gbk_load(uint32_t k);
static void gb_store_poll(void);
static void jam_poll(void);
static uint64_t lock_held(void);
static int kit_shown(int16_t *vp, int16_t *cur);
static const uint8_t *smp_peaks(uint32_t sl);
static const uint8_t *smp_zpeaks(uint32_t sl, int32_t x0, uint32_t w);

static const uint8_t GB_SPEED[5] = {0, 1, 2, 3, 10};
static uint32_t gb_speed(int32_t div)
{
    uint32_t i;
    for (i = 0; i < 5u; i++)
        if (GB_SPEED[i] == div)
            return i;
    return 2u;
}
static int32_t gb_own(const step_t *s)
{
    return s->probability && s->probability < COND_FIRST ? (int32_t)step_chance(s) : -1;
}
static uint32_t gb_trig(const step_t *s) { return gb_tc(s); }
static void gb_trig_name(char *b, uint32_t c)
{
    static const char *const NM[8] = {"FILL", "!FILL", "PRE", "!PRE", "NEI", "!NEI", "1ST", "!1ST"};
    uint32_t a, n;
    if (!c) {
        str_cpy(b, "-", 4);
    } else if (c <= GB_NAB) {
        gb_tc_ab(c, &a, &n);
        b[0] = (char)('0' + a);
        b[1] = ':';
        b[2] = (char)('0' + n);
        b[3] = 0;
    } else {
        str_cpy(b, NM[(c - GB_TC_FILL) % 8u], 8);
    }
}
static uint32_t gb_mach(uint32_t k) { return trk_mach(&trk[k % NTRK]); }

static uint32_t gb_page_icon(const page_t *pg, uint32_t mach)
{
    const track_t *t = TSEL;
    if (pg->graph == GR_PERF)
        return IC_MACRO;
    if (str_eq(pg->title, "COMB"))
        return IC_COMB;
    if (str_eq(pg->title, "DJ"))
        return IC_DJ;
    if (str_eq(pg->title, "CRUSH"))
        return eng_idx(t->eng_req) == ENGI_SAMPLER ? MACHINES[mach % NMACH].icon : IC_AGE;
    if (str_eq(pg->title, "DELAY"))
        return IC_DELAY;
    if (str_eq(pg->title, "VERB"))
        return IC_VERB;
    if (str_eq(pg->title, "SENDS"))
        return IC_DELAY;
    if (pg->id[0] == G_F1RATE || pg->id[0] == G_F2RATE || pg->id[0] == G_F3RATE)
        return IC_SINE + (uint32_t)(song.g[pg->id[1]] % 6 < 3 ? song.g[pg->id[1]] % 6 : song.g[pg->id[1]] % 6 - 1);
    if (str_eq(pg->title, "AMP"))
        return IC_NONE;
    if (pg->fam == FAM_FLT)
        return IC_LP + (flt_blend(t) ? (uint32_t)(clamp(t->p[P_LN1], 0, 127) + 32) / 64u
                                                         : (uint32_t)t->p[P_FTYPE] % 3u);
    if (pg->fam == FAM_LFO)
        return IC_SINE + (uint32_t)t->p[P_LWAVE] % 5u;
    if (pg->fam == FAM_TRK)
        return IC_NONE;
    return MACHINES[mach % NMACH].icon;
}

static uint32_t smp_format(uint32_t id, int32_t cur, char *b)
{
    char nb[8];
    uint32_t o = 0, q;
    if (id == P_E0) {
        int32_t t10 = (cur - 64) * 30 / 8, a = t10 < 0 ? -t10 : t10;
        b[o++] = t10 < 0 ? '-' : '+';
        fmt_int(nb, a / 10);
        for (q = 0; nb[q]; q++) b[o++] = nb[q];
        b[o++] = '.';
        b[o++] = (char)('0' + a % 10);
        b[o] = 0;
    } else if (id == P_E1) {
        uint32_t x = ((uint32_t)SMP_STRETCH[cur & 127] * 100u + 2048u) >> 12;
        fmt_int(nb, (int32_t)(x / 100u));
        for (q = 0; nb[q]; q++) b[o++] = nb[q];
        b[o++] = '.';
        b[o++] = (char)('0' + x / 10u % 10u);
        b[o++] = (char)('0' + x % 10u);
        b[o++] = 'x';
        b[o] = 0;
    } else if (id == P_E4) {
        str_cpy(b, N_SMODE[cur & 3], 8);
        return IC_ONCE + (uint32_t)(cur & 3);
    } else if (id == P_E5) {
        cur = (int32_t)smp_slot(cur);
        b[0] = 'S';
        b[1] = (char)('0' + (cur + 1) / 10);
        b[2] = (char)('0' + (cur + 1) % 10);
        b[3] = 0;
    } else if (id == P_DIST) {
        if (!cur)
            str_cpy(b, "OFF", 8);
    } else if (id == P_LN2) {
        uint32_t n = (uint32_t)cur, d = 16u;
        if (!n) {
            str_cpy(b, "KEY", 8);
        } else if (n >= 16u) {
            str_cpy(b, "BAR", 8);
        } else {
            while (!(n & 1u)) { n >>= 1; d >>= 1; }
            fmt_int(nb, (int32_t)n);
            for (q = 0; nb[q]; q++) b[o++] = nb[q];
            b[o++] = '/';
            fmt_int(nb, (int32_t)d);
            for (q = 0; nb[q]; q++) b[o++] = nb[q];
            b[o] = 0;
        }
    }
    return 0;
}

#define GB_BAN_W 212
#define GB_BAN_MS 2000u
static const char *const TM_SHORT[6] = {"LFO", "ENV", "FENV", "LFO 1", "LFO 2", "LFO 3"};
static int ban_fits(const char *s) { return large_face_has(s) && text_w(&AF_L, s) <= GB_BAN_W; }
static void ban_put(gb_view_t *v, const char *a, const char *b)
{
    if (v->bn >= 4u)
        return;
    str_cpy(v->bt[2u * v->bn], a, sizeof v->bt[0]);
    str_cpy(v->bt[2u * v->bn + 1u], b, sizeof v->bt[0]);
    v->bn++;
}
static void ban_msg(gb_view_t *v, const char *m)
{
    uint32_t i, n = str_len(m), best = 0, bd = 999u;
    char a[20];
    if (ban_fits(m) || n >= 2u * sizeof a) {
        ban_put(v, m, "");
        return;
    }
    for (i = 1; i + 1u < n; i++)
        if (m[i] == ' ' && (i > n / 2u ? i - n / 2u : n / 2u - i) < bd) {
            bd = i > n / 2u ? i - n / 2u : n / 2u - i;
            best = i;
        }
    if (!best) {
        ban_put(v, m, "");
        return;
    }
    str_cpy(a, m, best + 1u);
    ban_put(v, a, m + best + 1u);
}
static void gb_ban_fill(gb_view_t *v)
{
    static uint32_t t0;
    static char first[20];
    char a[24], b[12];
    v->bn = 0;
    if (gbsv || gbsv_t) {
    } else if (v->tmod == 1u) {
        static const char *const BTN[3] = {"LFO", "ENV", "FILTER"};
        const char *src = TM_NAME[v->tm_src % 6u];
        if (v->tm_name[0]) {
            str_cpy(a, TM_SHORT[v->tm_src % 6u], sizeof a);
            str_cpy(a + str_len(a), " > ", sizeof a - str_len(a));
            str_cpy(a + str_len(a), v->tm_name, sizeof a - str_len(a));
            fmt_int(b + (v->tm_amt > 0), v->tm_amt);
            if (v->tm_amt > 0)
                b[0] = '+';
            ban_put(v, a, b);
            ban_put(v, "SELECT", "= AMOUNT");
            ban_put(v, "OCT-", "= REMOVE");
        } else {
            str_cpy(a, src, sizeof a);
            str_cpy(a + str_len(a), " DEST", sizeof a - str_len(a));
            if (ban_fits(a))
                ban_put(v, a, "WIGGLE KNOB");
            else {
                ban_put(v, src, "DEST");
                ban_put(v, "WIGGLE", "KNOB");
            }
            str_cpy(a, "TAP ", sizeof a);
            str_cpy(a + 4, BTN[v->tm_src < 3u ? v->tm_src : 5u - v->tm_src % 6u], sizeof a - 4u);
            ban_put(v, a, "= CANCEL");
        }
    } else if (v->fillm) {
        ban_put(v, "FILL MODE", "ENTER TRIGS");
    } else if (v->temp == 2u) {
        char a[12];
        str_cpy(a, "MODE ", sizeof a);
        str_cpy(a + 5, N_SMODE[TSEL->p[P_E4] & 3], sizeof a - 5u);
        ban_put(v, a, "RELEASE: BACK");
    } else if (v->temp) {
        ban_put(v, "RELEASE TO", "REVERT");
    } else if (v->hint == 2u) {
        ban_put(v, "TEMP CHANGE", "WIGGLE KNOB");
    } else if (cla_st == 2u) {
        ban_put(v, "CLEAR ALL?", "PRESS OCT+");
    } else if (v->hint == 3u) {
        ban_put(v, "PASTE TRIG", "PRESS SAVE");
    } else if (acid_kon() && acid_pop()) {
        const acid_st_t *st = &acid_of(song.sel)->st[acid_sel[song.sel % NTRK] % ACID_N];
        char l2[12];
        str_cpy(l2, "TIME ", sizeof l2);
        fmt_int(l2 + 5, st->stime ? st->stime : 40);
        ban_put(v, st->flags & AF_SLIDE ? "SLIDE ON" : "SLIDE OFF", l2);
    } else if (v->hint == 1u) {
        ban_put(v, "WHITE KEY", "= LOCKS");
        ban_put(v, "WIGGLE KNOB", "= PARAM");
        ban_put(v, "OCT+", "= ACID");
        ban_put(v, "TAP SEQ", "= CANCEL");
    } else if (v->note[0]) {
        ban_msg(v, v->note);
    }
    v->ban = (uint8_t)(v->bn != 0u);
    if (v->bn && !str_eq(first, v->bt[0])) {
        str_cpy(first, v->bt[0], sizeof first);
        t0 = fm1_ms;
    } else if (!v->bn) {
        first[0] = 0;
    }
    v->bpg = (uint8_t)(v->bn > 1u ? (uint32_t)clamp((int32_t)(fm1_ms - t0), 0, 1 << 30) / GB_BAN_MS % v->bn : 0u);
}

static void gb_fill(gb_view_t *v)
{
    const page_t *pg = cur_page();
    uint32_t k, i, n = 0, at = 0, smp;
    uint64_t lk;
    char *p = (char *)v;
    for (i = 0; i < sizeof *v; i++)
        p[i] = 0;
    v->perf = pg->graph == GR_PERF && !fxt_x(pg);
    v->mixer = pg->graph == GR_TRK;
    v->sel = (uint8_t)song.sel;
    v->bpm = song.g[G_BPM];
    v->play = (uint8_t)(song.playing != 0);
    if (v->play && str_eq(pg->title, "DELAY"))
        v->echo_ph = (uint8_t)(fm1_ms / 100u % 12u);
    v->hot = (uint8_t)(ui.hot_t ? ui.hot_col % 4u + 1u : 0u);
    v->temp = (uint8_t)(FELUCCA_SIMPLE && mom.n != 0u);
    for (i = 0; v->temp && i < mom.n; i++)
        if (mom.vp[i] == &TSEL->p[P_E4] && eng_idx(TSEL->eng_req) == ENGI_SAMPLER)
            v->temp = 2u;
    v->fillm = (uint8_t)(FELUCCA_SIMPLE && seq_fill);
    v->hint = gb_hint;
    if (ui.msg_t && ui.msg[0] > 1)
        str_cpy(v->note, ui.msg, sizeof v->note);
    if (tm_dn) {
        uint32_t d = tm_dest();
        v->tmod = (uint8_t)(tm_up && d ? 2u : 1u);
        v->tm_src = (uint8_t)(fxl_sel ? 3u + tm_fxk() : (tm_dn - 1u) % 3u);
        v->tm_amt = (int8_t)(d ? *tm_amt() : 0);
        if (d)
            str_cpy(v->tm_name, fxl_sel ? N_FXDST[d % NELEM(N_FXDST)] : tm_name(TSEL, d), sizeof v->tm_name);
    }
    if (!bm_on) gb_ban_fill(v);
    v->bpm_alt = (uint8_t)(pm_altered(TSEL, &song.g[G_BPM]) != 0);
    if ((song.rec >> song.sel) & 1u) {
        uint32_t bs = beat_samples(), ph = ((beat_n & 1u) * bs + (beat_pos < bs ? beat_pos : bs)) * 32u / (2u * bs + 1u);
        v->rec = (uint8_t)(1u + (song.playing ? (ph < 16u ? 15u - ph : ph - 16u) : 15u));
    }
    str_cpy(v->title, pg->title, sizeof v->title);
    if (ui.confirm) {
        v->cfm = ui.confirm;
        confirm_text(v->cfa, v->cfb);
    }
    if (bm_on) {
        bm_fill(v);
        return;
    }
    if (tl_menu_f && !((fm1_in.buttons >> panel.btn[tl_menu_b]) & 1u))
        tl_menu_f = 0;
    v->menu_f = tl_menu_f;
    gl_fill(v);
    v->pr_mode=pr_single;v->sig_step=(uint8_t)(TSEL->seq_idx%16u);
    tl_fill(v, pg);
    v->fxg = (uint8_t)(str_eq(pg->title, "TAPE") ? 1u : str_eq(pg->title, "VINYL") ? 2u : str_eq(pg->title, "VHS") ? 3u : 0u);
    if (v->fxg && song.playing)
        v->fxang = (uint8_t)(fm1_ms / (v->fxg == 2u ? 50u : 90u) % 36u);
    v->kmode = (uint8_t)(ui.sgrid == 1u ? 1u : ui.sgrid == 2u ? 2u : ui.sgrid == 3u ? 4u : ui.sgrid == 4u && !fxl_sel ? 5u : 3u);
    if (v->kmode == 1u) {
        v->snp = (uint8_t)((sq_len() + 15u) / 16u);
        v->spg = ui.bank;
    }
    if (gb_nd_t && (int32_t)(fm1_ms - gb_nd_t) < (int32_t)TM_SHOW_MS && gbs_held()) {
        uint64_t h = gbs_held();
        uint32_t i = 0;
        while (!((h >> i) & 1u))
            i++;
        v->nud = (uint8_t)(step_nudge(&TSEL->step[i]) + 9);
    }
    v->pk = (uint8_t)(par_on() && par_id != 0xFFu && par_pg == ui.page && !ui.home ? par_k + 1u : 0u);
    if (par_on() && par_id != 0xFFu)
        str_cpy(v->pname, (par_id < P_COUNT ? track_desc(TSEL, par_id) : motion_desc(TSEL, par_id))->label, sizeof v->pname);
    v->fx = fxl_sel;
    for (k = 0; k < NTRK; k++)
        v->tm[k] = (uint8_t)gb_mach(k);
    v->mach = v->tm[song.sel % NTRK];
    v->picon = v->fx ? IC_NONE : MACHINES[v->mach % NMACH].icon;
    v->an = (uint8_t)(!v->fx && MACHINES[v->mach % NMACH].engine == ENGI_ANALOG);
    v->icon = (uint8_t)gb_page_icon(pg, v->mach);
    for (k = 0; k < NTRK; k++) {
        uint32_t h = trk_hit_ms[k];
        if (h && fm1_ms - h < 90u)
            v->thit |= (uint8_t)(1u << k);
        if (trk[k].p[P_MUTE])
            v->tmute |= (uint8_t)(1u << k);
    }
    if (song.playing) {
        const track_t *t = &trk[song.sel % NTRK];
        uint32_t len = t->p[P_SLEN] ? (uint32_t)t->p[P_SLEN] : 1u, idx = t->seq_idx % len;
        uint32_t cl = step_samples(t, div_samples((uint32_t)t->p[P_SDIV]), idx), fs = 0;
        if (t->seq_pos < cl)
            fs = t->seq_pos / (cl / 256u + 1u);
        v->brd = (uint16_t)(1u + (idx * 256u + fs) * 960u / (len * 256u));
        if (acid_on(t) && acid_rt[song.sel % NTRK].go == 2u) {
            const acid_t *a = acid_of(song.sel);
            uint32_t i2, at = 0, all = 0, cur = acid_rt[song.sel % NTRK].st;
            for (i2 = 0; i2 < a->stages; i2++) {
                uint32_t pu = a->st[i2].flags & AF_SKIP ? 0u : a->st[i2].pulses;
                at += i2 < cur ? pu : i2 == cur ? acid_rt[song.sel % NTRK].pul : 0u;
                all += pu;
            }
            v->brd = (uint16_t)(1u + at * 960u / (all ? all : 1u));
        }
    }
    {
        uint32_t h = trk_hit_ms[song.sel % NTRK];
        v->hit = (uint8_t)(MACHINES[v->mach % NMACH].drum && h && fm1_ms - h < 90u);
    }
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].fam == pg->fam && page_visible(i)) {
            if (i == ui.page)
                at = n;
            n++;
        }
    v->page = (uint8_t)at;
    v->npages = (uint8_t)n;
    if (gbsv || gbsv_t) {
        v->sv = gbsv ? gbsv : 1u;
        for (i = 0; i < 16u; i++)
            v->su |= (uint16_t)(gbs_used(gb_pool(v->mach), i) ? 1u << i : 0u);
        for (i = 0; i < 11u; i++)
            v->ku |= (uint16_t)(gbk_used(i) ? 1u << i : 0u);
        if (gbsv_t) {
            v->flash = (uint8_t)((uint32_t)clamp((int32_t)(fm1_ms - gbsv_t), 0, 99999) / 150u & 1u ? 2u : 1u);
            memcpy(v->msg, gbsv_msg, sizeof v->msg);
        }
        return;
    }
    if (fxt_x(pg)) {
        v->cross = 1;
        v->icon = IC_NONE;
        return;
    }
    if (pg->graph == GR_STAGE && !page_over) {
        sg_fill(v);
        return;
    }
    if (v->perf) {
        for (i = 0; i < PM_NM; i++) {
            uint32_t j;
            v->mac_n[i] = (uint8_t)pm_mac[i].n;
            for (j = 0; j < pm_h.n; j++)
                if (pm_h.mac[j] == i)
                    v->mac_held[i] = 1;
        }
        v->fade = (uint8_t)pm_fade;
        v->hn = (uint8_t)pm_h.n;
        v->rel = pm_rel;
        v->dj = song.g[G_MIDI];
        v->cmix = song.g[G_SYNC];
        v->ctune = song.g[G_CRATE];
        if (pm_h.n && pm_ui_c < PM_NM) {
            uint32_t f = (uint32_t)pm_ui_u * 26u / 1024u;
            if (f > 26u)
                f = 26u;
            if (pm_h.n == 2u && pm_ui_a < PM_NM)
                v->fill[pm_ui_a] = (uint8_t)(26u - f);
            v->fill[pm_ui_c] = (uint8_t)f;
        }
        v->rpt = (uint8_t)(perf_held & 63u);
        v->xf_a = xf.a;
        v->xf_b = xf.b;
        v->xf_x = (uint8_t)(xf.pos * 60u / 127u);
        return;
    }
    if (v->mixer) {
        v->ring = 4;
        for (k = 0; k < 4u; k++) {
            uint32_t tk = pg->id[k] & ~MIX_PAN, id = pg->id[k] & MIX_PAN ? P_PAN : P_LEVEL;
            const char *nm;
            const track_t *t;
            if (tk >= NTRK)
                continue;
            nm = id == P_PAN ? "PAN" : MACHINES[v->tm[tk] % NMACH].pill;
            t = &trk[tk];
            if (tk == song.sel && id == P_LEVEL)
                v->ring = (uint8_t)k;
            const param_desc_t *d = &TP[id];
            const char *u;
            for (i = 0; i < 4u && nm[i]; i++)
                v->lab[k][i] = nm[i];
            if (t->p[P_MUTE] && id == P_LEVEL)
                str_cpy(v->val[k], "MUTE", sizeof v->val[k]);
            else
                param_format(d, t->p[id], v->val[k], &u);
            v->ratio[k] = (int16_t)(RATIO(d, t->p[id]) < 0 ? 0 : RATIO(d, t->p[id]));
            v->mask |= (uint8_t)(1u << k);
        }
        return;
    }
    if (pg->graph == GR_GBSEQ && fxl_sel) {
        str_cpy(v->lab[0], "LEN", sizeof v->lab[0]);
        fmt_int(v->val[0], (int32_t)fxl_len());
        v->ratio[0] = (int16_t)((fxl_len() - 1u) * 1000u / (FXN - 1u));
        v->mask = 1u;
        return;
    }
    if (pg->graph == GR_GBSEQ) {
        static const char *const SPN[5] = {"x1/4", "x1/2", "x1", "x2", "x4"};
        static const char *const LB[4] = {"LEN", "SPD", "CHNC", "TRIG"};
        const track_t *t = TSEL;
        uint64_t held = gbs_held();
        uint32_t p = 0, sp = gb_speed(t->p[P_SDIV]);
        int32_t ch = t->p[P_APROB] > 100 ? 100 : t->p[P_APROB], tr = -1;
        while (held && !((held >> p) & 1u))
            p++;
        if (held) {
            char nb[4];
            fmt_int(nb, (int32_t)p + 1);
            str_cpy(v->title, "STEP ", sizeof v->title);
            str_cpy(v->title + 5, nb, sizeof v->title - 5u);
            ch = gb_own(&t->step[p]);
            tr = (int32_t)gb_trig(&t->step[p]);
        }
        for (k = 0; k < 4u; k++) {
            str_cpy(v->lab[k], LB[k], sizeof v->lab[k]);
            v->mask |= (uint8_t)(1u << k);
        }
        fmt_int(v->val[0], t->p[P_SLEN]);
        v->ratio[0] = (int16_t)((t->p[P_SLEN] - 1) * 1000 / (NSTEP - 1));
        str_cpy(v->val[1], SPN[sp], sizeof v->val[1]);
        v->ratio[1] = (int16_t)(sp * 250u);
        if (ch < 0) {
            str_cpy(v->val[2], "TRK", sizeof v->val[2]);
            v->ratio[2] = 0;
        } else {
            fmt_int(v->val[2], ch);
            for (i = 0; v->val[2][i]; i++)
                ;
            v->val[2][i] = '%';
            v->val[2][i + 1u] = 0;
            v->ratio[2] = (int16_t)(ch * 10);
        }
        gb_trig_name(v->val[3], (uint32_t)(tr < 0 ? 0 : tr));
        v->ratio[3] = (int16_t)((tr < 0 ? 0 : tr) * 1000 / (int32_t)GB_NTC);
        return;
    }
    if (pg->graph == GR_ACID) {
        static const char *const LB[8] = {"PITCH", "PULSE", "GATE", "OCT", "LEN", "DIR", "SPD", "GATE"};
        static const char *const GN[AG_N] = {"MUTE", "FIRST", "EVERY", "HOLD"};
        static const char *const DN[AD_N] = {"FWD", "REV", "PING", "RAND", "BRWN"};
        static const char *const SPN[5] = {"x1/4", "x1/2", "x1", "x2", "x4"};
        const acid_t *a = acid_of(song.sel);
        uint32_t sp = acid_sel[song.sel % NTRK] % ACID_N, pn = acid_pg(pg), x, pop = acid_pop();
        const acid_st_t *st = &a->st[sp];
        char nb[4];
        if (pn == 0u || pop) {
            fmt_int(nb, (int32_t)sp + 1);
            str_cpy(v->title, "STAGE ", sizeof v->title);
            str_cpy(v->title + 6, nb, sizeof v->title - 6u);
        }
        if (pop) {
            x = st->stime ? st->stime : 40u;
            str_cpy(v->lab[0], "SLIDE", sizeof v->lab[0]);
            str_cpy(v->val[0], st->flags & AF_SLIDE ? "ON" : "OFF", sizeof v->val[0]);
            v->ratio[0] = (int16_t)(st->flags & AF_SLIDE ? 1000 : 0);
            str_cpy(v->lab[1], "TIME", sizeof v->lab[1]);
            fmt_int(v->val[1], (int32_t)x);
            v->ratio[1] = (int16_t)(x * 1000u / 127u);
            v->mask = 3u;
            return;
        }
        for (k = 0; k < 4u; k++) {
            int32_t r = 0;
            char *o = v->val[k];
            str_cpy(v->lab[k], LB[pn * 4u + k], sizeof v->lab[k]);
            v->mask |= (uint8_t)(1u << k);
            switch (pn * 4u + k) {
            case 0:
                x = acid_note(a, st);
                str_cpy(o, N_NOTE[x % 12u], sizeof v->val[k]);
                fmt_int(nb, (int32_t)x / 12 - 1);
                str_cpy(o + str_len(o), nb, sizeof v->val[k] - str_len(o));
                r = st->pitch * 1000 / 12;
                break;
            case 1: fmt_int(o, st->pulses); r = (st->pulses - 1) * 1000 / 7; break;
            case 2: str_cpy(o, GN[st->gate % AG_N], sizeof v->val[k]); r = st->gate % AG_N * 333; break;
            case 3:
                if (st->oct > 3u)
                    o[0] = '+';
                fmt_int(o + (st->oct > 3u), (int32_t)st->oct - 3);
                r = st->oct * 1000 / 6;
                break;
            case 4: fmt_int(o, a->stages); r = (a->stages - 1) * 1000 / 15; break;
            case 5: str_cpy(o, DN[a->dir % AD_N], sizeof v->val[k]); r = a->dir % AD_N * 250; break;
            case 6:
                x = gb_speed(TSEL->p[P_SDIV]);
                str_cpy(o, SPN[x], sizeof v->val[k]);
                r = (int32_t)x * 250;
                break;
            default:
                fmt_int(o, a->gate * 100 / 127);
                str_cpy(o + str_len(o), "%", sizeof v->val[k] - str_len(o));
                r = a->gate * 1000 / 127;
                break;
            }
            v->ratio[k] = (int16_t)r;
        }
        return;
    }
    lk = lock_held();
    smp = !v->fx && eng_idx(TSEL->eng_req) == ENGI_SAMPLER && (pg->scope == SC_ENGINE || pg->scope == SC_SLICE);
    if (smp && str_eq(pg->title, "CRUSH"))
        str_cpy(v->title, "GATE", sizeof v->title);
    else if (smp) {
        const track_t *t = TSEL;
        int16_t st = t->p[P_E2], ln = t->p[P_E3];
        uint32_t sl = smp_slot(t->p[P_E5]), a, b, s0 = 0;
        if (lk) {
            while (!((lk >> s0) & 1u))
                s0++;
            (void)motion_lock_get(t, s0, P_E2, &st);
            (void)motion_lock_get(t, s0, P_E3, &ln);
        }
        smp_window(GB_WAVE, st, ln, &a, &b, 1u);
        b = a + b - 1u;
        if (sl < smp_n && ui.hot_t && pg->scope == SC_ENGINE && pg_kid(pg, ui.hot_col) == P_E2 + (ui.hot_col == 3u)
            && (ui.hot_col == 2u || ui.hot_col == 3u)) {
            uint32_t n = smp_tab[sl].len;
            v->zoom = (uint8_t)(ui.hot_col - 1u);
            smp_window(n, st, ln, &v->za, &v->zb, 65u);
            v->zb = v->za + v->zb - 1u;
        }
        v->wv = smp_peaks(sl) ? 2u : 1u;
        v->wa = (uint8_t)a;
        v->wb = (uint8_t)(b > GB_WAVE - 1u ? GB_WAVE - 1u : b);
        v->wkey = (uint16_t)(sl | smp_gen << 6);
        {
            const smp_sl_t *ss = &smp_sl[song.sel % NPART];
            uint16_t pp[SMP_SLN + 1u];
            uint32_t j, c;
            int32_t hd;
            v->sl_n = ss->n;
            for (j = 0; j <= ss->n && ss->n; j++)
                v->sl_c[j] = (uint8_t)(((uint32_t)ss->pos[j] * (GB_WAVE - 1u)) >> 16);
            v->sl_last = smp_sl_last[song.sel % NPART];
            if (pg->scope == SC_SLICE) {
                v->sl_pg = 1;
                c = sl_cfg[SL_MAN] ? sl_manual(pp) : sl_auto(pp);
                if (sl_cfg[SL_MAN]) {
                    c = 0;
                    for (j = 0; j < SMP_SLN; j++)
                        if ((sl_mm >> j) & 1u)
                            pp[c++] = sl_mk[j];
                    v->sl_pn = (uint8_t)c;
                } else
                    v->sl_pn = (uint8_t)(c ? c + 1u : 0u);
                for (j = 0; j < v->sl_pn; j++)
                    v->sl_p[j] = (uint8_t)(((uint32_t)pp[j] * (GB_WAVE - 1u)) >> 16);
                hd = sl_keys_on() ? smp_head(song.sel) : -1;
                v->sl_hd = (uint8_t)(hd >= 0 ? 1u + (((uint32_t)hd * (GB_WAVE - 1u)) >> 16) : 0u);
            }
        }
    }
    for (k = 0; k < 4u; k++) {
        int16_t *vp, cur;
        const param_desc_t *d = page_desc(pg, k, &vp);
        const char *u;
        int32_t r;
        if (!d || !d->label || d->label[0] == '-')
            continue;
        for (i = 0; i < 7u && d->label[i]; i++)
            v->lab[k][i] = d->label[i];
        cur = *vp;
        v->alt[k] = (uint8_t)(pm_altered(TSEL, vp) != 0);
        if (lk && v->fx) {
            uint32_t g = vp >= song.g && vp < song.g + G_COUNT ? motion_gid((uint32_t)(vp - song.g)) : 0xFFu, st = 0;
            while (!((lk >> st) & 1u))
                st++;
            v->alt[k] = 0;
            if (g != 0xFFu && ((fxl.lk[st] >> (g - P_COUNT)) & 1u)) {
                cur = fxl.v[st][g - P_COUNT];
                v->alt[k] = 1;
            }
        } else if (lk && ((vp >= TSEL->p && vp < TSEL->p + P_COUNT) ||
                   (vp >= song.g && vp < song.g + G_COUNT && motion_gid((uint32_t)(vp - song.g)) != 0xFFu))) {

            uint32_t id = vp >= TSEL->p && vp < TSEL->p + P_COUNT ? (uint32_t)(vp - TSEL->p) : motion_gid((uint32_t)(vp - song.g)), st = 0;
            int16_t lv;
            while (!((lk >> st) & 1u))
                st++;
            v->alt[k] = 0;
            if (motion_lock_get(TSEL, st, id, &lv)) {
                cur = lv;
                v->alt[k] = 1;
            } else if (motion_param(id)) {
                cur = motion_base_value(TSEL, id);
            }
        } else if (FELUCCA_SIMPLE && !v->fx && kit_on()) {
            int16_t kv = (int16_t)cur;
            if (kit_shown(vp, &kv))
                v->alt[k] = 1;
            cur = kv;
        }
        v->cur[k] = cur;
        param_format(d, cur, v->val[k], &u);
        if (d->names == N_MDST)
            str_cpy(v->val[k], tm_name(TSEL, (uint32_t)cur % MD_N), sizeof v->val[k]);
        if (smp && vp >= TSEL->p && vp < TSEL->p + P_COUNT)
            v->vic[k] = (uint8_t)smp_format((uint32_t)(vp - TSEL->p), cur, v->val[k]);
        if (vp == &song.g[G_RSIZE] && cur >= 127)
            str_cpy(v->val[k], "FREEZE", sizeof v->val[k]);
        if (str_eq(v->val[k], "CYM"))
            str_cpy(v->val[k], "CY", sizeof v->val[k]);
        r = RATIO(d, enum_rank(d, cur));
        v->ratio[k] = (int16_t)(r < 0 ? 0 : r);
        v->mask |= (uint8_t)(1u << k);
    }
    { uint32_t gi=gen_index(pg);
      if(gi<4u){str_cpy(v->lab[0],"Gamble",sizeof v->lab[0]);fmt_int(v->val[0],gen_amount[gi]);
        uint32_t nn=str_len(v->val[0]);v->val[0][nn]='%';v->val[0][nn+1]=0;v->ratio[0]=gen_amount[gi]*10u;v->mask|=1u;}
    }
    if (str_eq(pg->title, "CLOCK")) {
        uint32_t c = song.cpu_q8 * 100u / 256u;
        c = c > 100u ? 100u : c;
#if FELUCCA_CPU2
        str_cpy(v->lab[3], fm1_c1_on ? "2CPU" : "CPU", sizeof v->lab[3]);
#else
        str_cpy(v->lab[3], "CPU", sizeof v->lab[3]);
#endif
        fmt_int(v->val[3], (int32_t)c);
        for (i = 0; v->val[3][i]; i++)
            ;
        v->val[3][i] = '%';
        v->val[3][i + 1u] = 0;
        v->ratio[3] = (int16_t)(c * 10u);
        v->mask |= 8u;
    }
    if (str_eq(pg->title, "COMB") && !v->cross)
        v->dj_sel = cb_sel < NTRK ? cb_sel : NTRK;
    if (str_eq(pg->title, "QUICK FILL") && !v->cross) {
        uint32_t q = qf_at();
        v->qf_at = (uint8_t)q;
        v->qf_n = (uint8_t)(q >> 8);
    }
    if (str_eq(pg->title, "DJ") && !v->cross) {
        const char *u;
        for (k = 0; k < NTRK; k++)
            v->dj_v[k] = dj_trk[k];
        v->dj_v[NTRK] = (int8_t)song.g[G_MIDI];
        v->dj_sel = dj_sel;
        if (dj_sel < NTRK) {
            param_format(&GP[G_MIDI], dj_trk[dj_sel], v->val[0], &u);
            v->ratio[0] = (int16_t)((dj_trk[dj_sel] + 64) * 1000 / 127);
            v->cur[0] = dj_trk[dj_sel];
            v->alt[0] = 0;
            v->lab[0][0] = 'T';
            v->lab[0][1] = (char)('1' + dj_sel);
            str_cpy(v->lab[0] + 2, " DJ", sizeof v->lab[0] - 2u);
        } else
            str_cpy(v->lab[0], "M DJ", sizeof v->lab[0]);
    }
}

static uint16_t gb_bg;
static uint16_t gb_hc;
static uint32_t gb_isqrt(uint32_t n)
{
    uint32_t r = 0;
    while ((r + 1u) * (r + 1u) <= n)
        r++;
    return r;
}
static int32_t gb_ymin = -32768;
static void gb_oval(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t col)
{
    int32_t r = h / 2, j;
    for (j = 0; j < h; j++) {
        if (y + j < gb_ymin)
            continue;
        int32_t d2 = 2 * j + 1 - h;
        int32_t dx = (int32_t)gb_isqrt((uint32_t)(r * r * 4 - d2 * d2) + 0u) ;
        int32_t in = r - (dx + 1) / 2;
        cv_rect(x + in, y + j, w - 2 * in, 1, col);
    }
}
static void gb_pill(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c, int on)
{
    gb_oval(x, y, w, h, c);
    if (!on)
        gb_oval(x + 2, y + 2, w - 4, h - 4, gb_bg);
}

static void gb_clip(int32_t y0, int32_t y1)
{
    int32_t a = y0 + cv_oy, b = y1 + cv_oy;
    cv_cy0 = (int16_t)(a < 0 ? 0 : a);
    cv_cy1 = (int16_t)(b > GB_STRIP ? GB_STRIP : b < a ? a : b);
}
static void gb_unclip(void) { cv_cy0 = 0; cv_cy1 = GB_STRIP; }

static void gb_icon(int32_t x, int32_t y, int32_t s, uint32_t icon, uint16_t col)
{
    const uint16_t *ic = icon < IC_COUNT ? GB_ICONS[icon] : 0;
    int32_t j, i, r;
    if (!ic)
        return;
    for (j = 0; j < 16; j++) {
        if (y + (j + 1) * s + cv_oy <= 0 || y + j * s + cv_oy >= GB_STRIP)
            continue;
        for (i = 0; i < 16; i = r) {
            for (r = i; r < 16 && ((ic[j] << r) & 0x8000u); r++)
                ;
            if (r > i)
                cv_rect(x + i * s, y + j * s, (r - i) * s, s, col);
            else
                r++;
        }
    }
}

static uint16_t gb_mix(uint16_t bg, uint32_t a)
{
    int32_t r = bg >> 11, g = (bg >> 5) & 63, b = bg & 31, w = GB_W;
    r += (((w >> 11) - r) * (int32_t)a) / 256;
    g += ((((w >> 5) & 63) - g) * (int32_t)a) / 256;
    b += (((w & 31) - b) * (int32_t)a) / 256;
    return (uint16_t)(r << 11 | g << 5 | b);
}

#include "trap_fxgfx.c"
#include "trap_layout.c"
#include "trap_ui_specialist.c"
#include "trap_gh.c"
#include "trap_gh_pages.c"
static void gb_scene(const gb_view_t *v, uint16_t bgc)
{
    cv_bg = gb_bg = GH_BG;
    (void)bgc;
    gh_scene(v, GH_BG);
}

static void gb_popup(const gb_view_t *v, uint16_t bgc)
{
    const int32_t cx = 120, cy = 124;
    const uint16_t ink = TL_CARD_INK, bk = TL_CARD;
    int32_t a, px, py, amt = v->tm_amt, on = amt * 135 / 64;
    char nb[8], hd[24];
    const aafont_t *f;
    str_cpy(hd, TM_SHORT[v->tm_src % 6u], sizeof hd);
    str_cpy(hd + str_len(hd), " > ", sizeof hd - str_len(hd));
    str_cpy(hd + str_len(hd), v->tm_name, sizeof hd - str_len(hd));
    f = large_face_has(hd) && text_w(&AF_L, hd) <= 200 ? &AF_L : &AF_M;
    (void)bgc;
    tl_card(12, 30, 216, 178, GB_K);
    cv_text_in(16, f == &AF_L ? 46 : 52, 208, f, hd, ink, bk);
    for (a = 0; a < 360; a++) {
        int in = on >= 0 ? (a <= on) : (a >= 360 + on || a == 0);
        dial_pt(cx, cy, 30, a, &px, &py);
        cv_rect(px - 1, py - 1, 3, 3, ink);
        if (amt && in) {
            dial_pt(cx, cy, 37, a, &px, &py);
            cv_rect(px - 2, py - 2, 5, 5, T_THEME);
        }
    }
    dial_pt(cx, cy, 24, 360 + on, &px, &py);
    cv_line_t(cx, cy, px, py, ink, 4);
    fmt_int(nb + (amt > 0 ? 1 : 0), amt);
    if (amt > 0)
        nb[0] = '+';
    cv_text_in(14, 162, 212, &AF_L, nb, ink, bk);
}

static void gb_nudge(const gb_view_t *v, uint16_t bgc)
{
    const uint16_t ink = TL_CARD_INK, bk = TL_CARD;
    int32_t n = (int32_t)v->nud - 9, i, x;
    char nb[8];
    (void)bgc;
    tl_card(12, 30, 216, 178, GB_K);
    cv_text_in(14, 46, 212, &AF_L, "NUDGE", ink, bk);
    cv_rect(28, 118, 184, 3, ink);
    for (i = -8; i <= 8; i++) {
        int32_t h = i == 0 ? 36 : (i & 3) == 0 ? 20 : 10;
        x = 120 + i * 11;
        cv_rect(x - (i == 0 ? 1 : 0), 120 - h / 2, i == 0 ? 3 : 1, h, ink);
    }
    x = 120 + n * 11;
    cv_rect(x - 5, 92, 11, 14, T_THEME);
    cv_rect(x - 2, 106, 5, 6, T_THEME);
    fmt_int(nb + (n > 0 ? 1 : 0), n);
    if (n > 0)
        nb[0] = '+';
    cv_text_in(14, 150, 212, &AF_L, nb, ink, bk);
}

static const aafont_t *gb_big(const char *s, int32_t w)
{
    return large_face_has(s) && text_w(&AF_L, s) <= w ? &AF_L : &AF_M;
}
static void gb_banner(uint16_t bgc, const char *a, const char *b)
{
    const aafont_t *fa = gb_big(a, 204), *fb = gb_big(b, 204);
    (void)bgc;
    if (fa == &AF_M && text_w(&AF_M, a) > 206)
        fa = &AF_S;
    if (fb == &AF_M && text_w(&AF_M, b) > 206)
        fb = &AF_S;
    tl_card(8, 160, 224, 74, GB_K);
    if (!b[0]) {
        cv_text_in(14, fa == &AF_L ? 183 : 189, 216, fa, a, TL_CARD_INK, TL_CARD);
        return;
    }
    cv_text_in(14, fa == &AF_L ? 164 : 170, 216, fa, a, TL_CARD_INK, TL_CARD);
    cv_text_in(14, fb == &AF_L ? 198 : 205, 216, fb, b, TL_CARD_INK, TL_CARD);
}

static void gb_ovr(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c, int direct)
{
    if (!w || !h)
        return;
    if (direct)
        lcd_fill(x, y, w, h, c);
    else
        cv_rect((int32_t)x, (int32_t)y, (int32_t)w, (int32_t)h, c);
}
#define GB_BRD 5u
static void gb_overlay(const gb_view_t *v, uint16_t bgc, int direct)
{
    uint32_t f[4], i, k, q = v->brd;
    for (i = 0; i < 4u; i++)
        f[i] = q > i * 240u ? (q - i * 240u < 240u ? q - i * 240u : 240u) : 0u;
    const uint32_t hh = tl_on(v) ? TL_TOP : GB_HDR;
    const uint16_t hc = tl_on(v) ? T_ACCENT : gb_hc;
    gb_ovr(f[0], 0, 240 - f[0], GB_BRD, hc, direct);
    if (f[1] < hh)
        gb_ovr(240 - GB_BRD, f[1], GB_BRD, hh - f[1], hc, direct);
    gb_ovr(240 - GB_BRD, f[1] > hh ? f[1] : hh, GB_BRD, 240 - (f[1] > hh ? f[1] : hh), bgc, direct);
    gb_ovr(0, 240 - GB_BRD, 240 - f[2], GB_BRD, bgc, direct);
    gb_ovr(0, 0, GB_BRD, 240 - f[3] < hh ? 240 - f[3] : hh, hc, direct);
    if (240 - f[3] > hh)
        gb_ovr(0, hh, GB_BRD, 240 - f[3] - hh, bgc, direct);
    gb_ovr(0, 0, f[0], GB_BRD, GB_W, direct);
    gb_ovr(240 - GB_BRD, 0, GB_BRD, f[1], GB_W, direct);
    gb_ovr(240 - f[2], 240 - GB_BRD, f[2], GB_BRD, GB_W, direct);
    gb_ovr(0, 240 - f[3], GB_BRD, f[3], GB_W, direct);
    for (k = 0; !v->ban && !v->bm && !v->stg && k < NTRK; k++) {

        uint32_t x = tl_on(v) ? TL_SQX + k * TL_SQS : 62u + k * 20u, y = tl_on(v) ? TL_SQY : 214u;
        uint32_t e = k == v->sel ? 3u : 1u, m = (v->tmute >> k) & 1u;
        uint16_t in = (v->thit >> k) & 1u ? GB_W : m ? 0u : gb_col[k];
        gb_ovr(x, y, 16, 16, GB_W, direct);
        gb_ovr(x + e, y + e, 16 - 2u * e, 16 - 2u * e, in, direct);
        if (m)
            for (i = 3; i < 13u; i++) {
                gb_ovr(x + i, y + i, 2, 1, GB_W, direct);
                gb_ovr(x + 14u - i, y + i, 2, 1, GB_W, direct);
            }
    }
}

static void gb_draw(void)
{
    uint32_t i, s, mono, light, nstrip;
    uint16_t c;
    typeof(ux) sd_saved;
    const uint8_t *a, *b;
    gb_fill(&gb_v);
    c = gb_v.fx ? GB_RGB(120, 128, 140) : gb_col[gb_v.sel % NTRK];
    a = (const uint8_t *)&gb_v;
    b = (const uint8_t *)&gb_last;
    for (i = 0; i < sizeof gb_v && a[i] == b[i]; i++)
        ;
    if (!ui.force && gb_have && i == sizeof gb_v)
        return;

    {
        gb_view_t t = gb_v;
        t.hit = gb_last.hit;
        t.thit = gb_last.thit;

        if (t.rec && gb_last.rec)
            t.rec = gb_last.rec;
        nstrip = !ui.force && gb_have && !memcmp(&t, &gb_last, sizeof t) ? 1u : 240u / GB_STRIP;
    }
    gb_last = gb_v;
    gb_have = 1;
    sd_saved = ux;
    gh_theme();
    mono = ux.mono;
    light = ux.light;
    if (mono || light) {
        ux.mono = 0;
        ux.light = 0;
        ux.gen++;
    }
    gb_hc = c;
    for (s = 0; s < nstrip; s++) {
        cv_begin(240, GB_STRIP, GB_K);
        cv_oy = -(int32_t)(s * GB_STRIP);
        gb_scene(&gb_v, GB_K);
        if (gb_v.tmod == 2u)
            gb_popup(&gb_v, c);
        else if (gb_v.nud)
            gb_nudge(&gb_v, c);

        (void)gb_banner;
        (void)tl_menu;
        (void)tl_gl;

        cv_oy = 0;

        cv_blit(0, s * GB_STRIP);
    }
    (void)gb_overlay;
    if (mono || light) {
        ux.mono = (typeof(ux.mono))mono;
        ux.light = (typeof(ux.light))light;
        ux.gen++;
    }
    ux = sd_saved;
}

/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

static int project_save(uint32_t slot);
static void panel_setup(void);
static void project_load(uint32_t slot);
static int project_used(uint32_t slot);
static int up_used(uint32_t k);
static int up_load(uint32_t k);
static uint32_t up_count(void);
static uint32_t up_nth(uint32_t n);
static uint32_t up_rank(uint32_t slot);
static uint32_t up_engine(uint32_t k);
static void up_name(uint32_t k, char *b);
static void up_slot_label(char *b, uint32_t k);
static void up_ui(uint32_t op, uint32_t k);
static int up_has_pat(uint32_t k);
static uint32_t up_pat_count(void);
static uint32_t up_pat_nth(uint32_t n);
static uint32_t up_pat_rank(uint32_t slot);
static void up_pat_load(track_t *t, uint32_t k);
static void up_auto_name(char *b, uint32_t e, uint32_t k);
static void up_ui_named(uint32_t op, uint32_t k, const char *name, uint32_t cat);
static uint32_t up_cat(uint32_t k);
static int project_save_as(uint32_t slot, const char *name);
static int project_name(uint32_t slot, char *b);
static int project_info(uint32_t slot, char *b, uint16_t *len48, uint32_t *den, uint8_t *n);
static int project_rename(uint32_t slot, const char *name);
static int project_erase(uint32_t slot);
static void project_cur_name(char *b);
static uint32_t up_gen;
static uint8_t ui_pref_bytes[32];

#define ui_prefs (ui_pref_bytes[30])
#define PREF_LATCH 1u
#define PREF_ANIM_OFF 2u
#define PREF_ACCEL 4u
#define PREF_USB_FIXED 8u
#define PREF_BPM_LOCK 16u
#define PREF_LARGE 32u

#define PREF_SERIAL_OFF 64u

#define PREF_RESTORE_OFF 128u

#define fx_latch ui_prefs

#define ui_rec_prefs (ui_pref_bytes[28])
#define PREF_REC 0xFF00u
#define PREF_SCALE_LEDS (0x40u << 8)
#define PREF_MIDI_FOLLOW (0x80u << 8)

#define ui_prefs2 (ui_pref_bytes[24])
#define PREF_X 0xFF0000u
#define PREF_SCOPE_MIX (0x01u << 16)
#define PREF_PREVIEW (0x02u << 16)
#define PREF_CHORD_ADD (0x04u << 16)
#define PREF_HELP (0x08u << 16)
#define PREF_SEL_PAGE (0x10u << 16)
#define PREF_WAKE_PANEL (0x20u << 16)
#define PREF_SEL_HI (0x40u << 16)
#define PREF_CPU2_ON (0x80u << 16)

enum { SEL_BPM, SEL_PAGE, SEL_MODW, SEL_VEL, SEL_N };

#define ui_home_view (ui_pref_bytes[26])

#define ui_leds_inv (ui_pref_bytes[25])
static uint32_t leds_inv(void) { return settings_leds == LEDS_INV && ui_leds_inv - 1u < 2u ? ui_leds_inv : 0u; }
enum { HV_SCOPE, HV_TRACKS, HV_LEVELS, HV_COUNT };
static int home_levels(void) { return ui_home_view == HV_LEVELS; }
enum { RP_CLICK, RP_LEVEL, RP_COUNTIN };

#define ui_style (ui_pref_bytes[29])
static void draw_gaps(uint32_t y, uint32_t h);
static int dial_page(void);

static uint8_t sync_reload;

#define ACC T_THEME
#define VAL(c) ((c) == ui.hot_col && ui.hot_t ? T_ACCENT : T_THEME)
#define RATIO(d, v) ((d)->max > (d)->min ? ((int32_t)(v) - (d)->min) * 1000 / ((d)->max - (d)->min) : -1)

#define Y_HEAD 0
#define H_HEAD 24
#define Y_LABEL 28
#define Y_SEP_END 72
#define CARD_W 57
#define CARD_H 44
#define CARD_X(c) (3 + 59 * (int32_t)(c))
#define Y_GRAPH 76
#define H_GRAPH 122
#define Y_FOOT 202
#define H_FOOT 38

#define LG_CARD_H 104
#define LG_Y_GRAPH 136
#define LG_H_GRAPH 62

static struct {
    uint8_t home;
    uint8_t page;
    uint8_t fam_last[FAM_COUNT];
    uint8_t bank;
    uint8_t cursor;
    uint8_t entry_open;
    uint8_t sgrid;

    uint8_t lane;
    uint8_t plk_src;

    uint8_t plk_lane, plk_trk;
    uint64_t plk_rm, plk_acc;

    uint8_t hot_col, hot_t;
    uint8_t menu;
    uint8_t menu_sel;
    uint8_t menu_row[5];
    uint16_t menu_scroll;
    uint32_t menu_sig, home_t0;
    uint8_t force;
    uint8_t msg_t;
    uint8_t help_t;
    uint8_t bpm_t;
    uint8_t act;
    uint8_t tl_clean;
    uint32_t rec_t0;
    uint32_t save_t0;
    uint8_t confirm;
    uint8_t confirm_trk;
    uint8_t uslot;
    uint8_t ppick;
    uint8_t song_row;
    uint8_t song_trk;
    uint16_t ev_row;

    uint8_t ev_step, ev_id;

    uint16_t ev_keep;
    uint8_t ev_vel;
    uint8_t uboot;
    uint32_t ly_t0;
    uint8_t ly;
    uint8_t layer;
    uint8_t lock;
    uint16_t pg_down;
    uint32_t layer_sig;
    char msg[24];
    char msg2[24];
    uint32_t enc_t[NE];

    char col[4][32];
    uint32_t graph_sig, head_sig, foot_sig, frame;

    struct {
        char from[7];
        uint8_t t0;
        int8_t dir;
        uint8_t sig;
    } roll[5];
    int16_t roll_bpm;
    struct {

        uint8_t on;
        uint8_t pick, trk, id;
        volatile uint8_t io[2];
    } ml;
    volatile uint8_t mpc[NTRK];

} ui;

enum { CF_NONE, CF_CLEAR_SEQ, CF_CLEAR_TRK, CF_OVR_PROJ, CF_OVR_USER, CF_LOAD_PAT,
       CF_DEL_ROW, CF_CLEAR_SONG, CF_INIT_SOUND, CF_CLEAR_MOTION, CF_ERASE_USER, CF_ERASE_PROJ, CF_CLEAN_SOUND, CF_CHORD_GEN, CF_PHONK, CF_EUCLID,
       CF_TRAPGEN, CF_HATGEN };

#define ui_scr (ui_pref_bytes[27])

#define ui_idle (ui_pref_bytes[10])
#define SCR_DEF 0u
static const uint8_t SCR_CODE[5] = {0, 2, 1, 4, 7};
static const uint8_t SCR_DEC[8] = {0, 2, 1, 0, 3, 0, 0, 4};
#define SCR_EAT_MS 300u
static const uint8_t SCR_MIN[5] = {0, 5, 15, 30, 60};
enum { SCR_ON, SCR_OFF, SCR_WAKE, SCR_SLPOUT, SCR_SHOW };
static struct {
    uint8_t st;
    uint8_t eat;
    uint32_t idle;
    uint32_t t;
    uint32_t eat_t;
} scrn;
static uint32_t scr_get(void)
{
    return ui_scr < NELEM(SCR_DEC) ? SCR_DEC[ui_scr] : SCR_DEF;
}
static void scr_wake(void)
{
    if (scrn.st == SCR_OFF)
        scrn.st = SCR_WAKE;
    scrn.eat = 1;
}
static void scr_wake_now(void)
{
    if (scrn.st != SCR_ON)
        lcd_wake_now();
    scrn.st = SCR_ON;
    scrn.idle = fm1_ms;
}
static uint8_t mf_seen;

static int scr_input(uint32_t pressed, uint32_t notes)
{
    uint32_t held = fm1_in.buttons | fm1_in.notes, k;
    int32_t moved = 0;
    if (pressed | notes | held | panel_moved)
        scrn.idle = fm1_ms;
    panel_moved = 0;
    if (mf_seen && !(ui_prefs2 & (PREF_WAKE_PANEL >> 16))) {

        scrn.idle = fm1_ms;
        if (scrn.st == SCR_OFF)
            scrn.st = SCR_WAKE;
    }
    if (scrn.st == SCR_ON && !scrn.eat)
        return 0;
    for (k = 0; k < NE; k++)
        moved |= panel_enc(k);
    panel_moved = 0;
    if (moved)
        scrn.eat_t = scrn.idle = fm1_ms;
    if (pressed | notes | held | (uint32_t)moved)
        scr_wake();
    if (scrn.st != SCR_ON || held || (int32_t)(fm1_ms - scrn.eat_t) < (int32_t)SCR_EAT_MS)
        return 1;
    scrn.eat = 0;
    kb_asleep = 0;
    return 0;
}

static int scr_frame(void)
{
    uint32_t lim = SCR_MIN[scr_get()] * 60000u;
    if (ui.confirm || ui.uboot || seq_counting()) {
        scrn.idle = fm1_ms;
        if (scrn.st == SCR_OFF)
            scr_wake();
    }
    switch (scrn.st) {
    case SCR_ON:
        if (!lim || (int32_t)(fm1_ms - scrn.idle) < (int32_t)lim)
            return 1;
        lcd_power(0);
        scrn.st = SCR_OFF;
        scrn.t = fm1_ms;
        scrn.eat = 1;
        kb_asleep = 1;
        break;
    case SCR_WAKE:
        if (fm1_ms - scrn.t >= 120u) {
            lcd_power(1);
            scrn.st = SCR_SLPOUT;
            scrn.t = fm1_ms;
        }
        break;
    case SCR_SLPOUT:
        if (fm1_ms - scrn.t >= 120u) {
            scrn.st = SCR_SHOW;
            ui.force = 1;
            return 1;
        }
        break;
    case SCR_SHOW:
        return 1;
    }
    ui.msg_t = 0;
    ui.msg2[0] = 0;
    ui.hot_t = ui.bpm_t = 0;
    return 0;
}
static void scr_shown(void)
{
    if (scrn.st != SCR_SHOW)
        return;
    lcd_power(2);
    scrn.st = SCR_ON;
    scrn.idle = fm1_ms;
}

static const page_t *page_over;
static const page_t *cur_page(void) { return page_over ? page_over : &PAGES[ui.page]; }

enum { LK_OFF, LK_LABEL, LK_TALL };
static int large_face_has(const char *s)
{
    for (; *s; s++)
        if (glyph_at(&AF_L, fold(&AF_L, (uint8_t)*s)) < 0)
            return 0;
    return 1;
}

enum { LAYER_NONE, LAYER_FX, LAYER_GLO, LAYER_SCL, LAYER_EDIT, LAYER_SEQ, LAYER_REC, LAYER_ENV, LAYER_N };

static uint8_t fxl_sel;
static uint8_t cla_st;

static int fxt_x(const page_t *pg)
{
    return fxl_sel ? (pg->scope == SC_FM6 || pg->scope == SC_TRACK || pg->scope == SC_ENGINE || pg->scope == SC_CHGEN || pg->scope == SC_BANKA || pg->scope == SC_BANKS || pg->scope == SC_BANKALL || pg->scope == SC_GEN85 || pg->scope == SC_TRAPGEN || pg->scope == SC_HATGEN || (pg->scope == SC_PHONK && !pg->fxt)) && pg->graph != GR_GBSEQ : pg->fxt == 1u;
}
static int page_visible(uint32_t i)
{
    if (i >= NPAGES || fxt_x(&PAGES[i]))
        return 0;
    if (PAGES[i].scope == SC_TRK && PAGES[i].id[0] >= 2u)
        return i == ui.page;
    if (FELUCCA_SIMPLE && PAGES[i].fam == FAM_SEQ)
        return (PAGES[i].graph == GR_ACID) == (!fxl_sel && acid_on(&trk[song.sel % NTRK]));
    if (PAGES[i].scope == SC_SLICE)
        return eng_idx(trk[song.sel % NTRK].eng_req) == ENGI_SAMPLER;
    if(TRAP_FM6_TEST && TSEL->eng_req==ENGI_FM6 && PAGES[i].scope==SC_TRACK &&
       (PAGES[i].id[0]==P_ATK || PAGES[i].id[0]==P_M2DST || PAGES[i].fam==FAM_FLT)) return 0;
#if TRAP_UX_TEST
    if(TRAP_FM6_TEST && TSEL->eng_req==ENGI_FM6 && PAGES[i].scope==SC_TRACK &&
       (PAGES[i].fam==FAM_ENV || PAGES[i].fam==FAM_LFO || PAGES[i].fam==FAM_FX)) return 0;
#endif
    if(PAGES[i].scope==SC_FM6) return TRAP_FM6_TEST && TSEL->eng_req==ENGI_FM6;
    return PAGES[i].scope != SC_ENGINE || page_id_an(&PAGES[i], PAGES[i].id[0]) != 0xFFu;
}
static uint32_t acid_pg(const page_t *pg)
{
    return (uint32_t)(pg->title[0] == 'A');
}

static uint32_t page_first(uint32_t fam)
{
    uint32_t i, f = NPAGES;
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].fam == fam) {
            if (page_visible(i))
                return i;
            if (f == NPAGES)
                f = i;
        }
    return f < NPAGES ? f : 0;
}

static void ui_say(const char *a, const char *b)
{
    uint32_t n;
    str_cpy(ui.msg, a, sizeof ui.msg);
    n = str_len(ui.msg);
    str_cpy(ui.msg + n, b, sizeof ui.msg - n);
    ui.msg_t = FELUCCA_SIMPLE ? 80u : 40u;
    ui.msg2[0] = 0;
}

static void ui_message(const char *s) { ui_say(s, ""); }

#define MSG_NOFILE "\x01"

#define MSG_NO_SAMPLE MSG_NOFILE "NO SAMPLE"

static int transport_busy(void)
{
    int busy;
    fm1_irq_off();
    busy = song.playing || transport_req == 1u || seq_counting();
    fm1_irq_on();
    return busy;
}

static uint32_t lock_pid(const page_t *pg, uint32_t k)
{
    uint32_t id = page_id_an(pg, pg_kid(pg, k));
    if (FELUCCA_SIMPLE && alt_mode && alt_ids(pg))
        id = alt_ids(pg)[k & 3u];
    if (pg->scope == SC_FM6 || id == 0xFFu)
        return 0xFFu;
    if ((id & 0x80u) || pg->scope == SC_GLOBAL)
        id = motion_gid(id & 0x7Fu);
    return id < MOTION_N && motion_param(id) ? id : 0xFFu;
}
static int lock_page_ok(uint32_t i)
{
    uint32_t k;
    if (i >= NPAGES || PAGES[i].graph == GR_MOD || PAGES[i].graph == GR_PERF ||
        (PAGES[i].scope != SC_TRACK && PAGES[i].scope != SC_ENGINE && !(FELUCCA_SIMPLE && PAGES[i].scope == SC_GLOBAL)))
        return 0;
    for (k = 0; k < 4u; k++)
        if (lock_pid(&PAGES[i], k) != 0xFFu)
            return 1;
    return 0;
}
static uint32_t lock_id(uint32_t k)
{
    uint32_t id = ui.plk_src && lock_page_ok(ui.plk_src - 1u) ? lock_pid(&PAGES[ui.plk_src - 1u], k)
                                                               : ENGINES[TSEL->eng_req % NENGINES]->knob[k & 3u];
    return id < MOTION_N && motion_param(id) ? id : 0xFFu;
}

#define HELP_FRAMES 120u
static int help_on(void) { return (ui_prefs2 & (PREF_HELP >> 16)) != 0u; }

#define MLIT_MS 90u
static uint32_t midi_lit_ms[NTRK];
static int midi_lit(uint32_t k) { return midi_lit_ms[k % NTRK] && (fm1_ms | 1u) - midi_lit_ms[k % NTRK] < MLIT_MS; }
static void help_start(void) { ui.help_t = (uint8_t)(help_on() ? HELP_FRAMES : 0u); }
static void page_entered(void)
{
    const page_t *pg = cur_page();
    if (!ui.home && lock_page_ok(ui.page))
        ui.plk_src = (uint8_t)(ui.page + 1u);
    song.seq_mode = !ui.home && pg->fam == FAM_SEQ;
    if (FELUCCA_SIMPLE && !ui.home && pg->graph == GR_PERF && !fxt_x(pg))
        ui.sgrid = 0;
    ui.entry_open = 0;
    ui.hot_t = 0;
    ui.act = pg->graph == GR_USER ? 4u : 0u;
    ui.tl_clean = 0;
    smp_lay = 0;
    help_start();
    ui.force = 1;
}

static void step_clear(step_t *st)
{
    st->n = 0;
    st->time = ST_REST;
    st->flags = 0;
    st->vel = 0;
    st->hit = st->acc = 0;
    st->probability = 0;
    st->roll = 0;
}

static int grid_on(void) { return !ui.home && !ui.confirm && cur_page()->graph == GR_ROLL && drum_track(TSEL); }

static int detail_on(void)
{
    return !ui.home && !ui.confirm && cur_page()->scope == SC_STEP && str_eq(cur_page()->title, "DETAIL");
}

static int pm_page(void);
static int pm_down(void);
static uint8_t gbsv;
static char gbsv_msg[2][16];
static uint32_t gbsv_t;
static int gbsv_keys_on(void) { return gbsv == 1u || gbsv == 2u; }
static int sg_page(void);
static int sgrid_on(void) { return FELUCCA_SIMPLE && ui.sgrid == 1u && !ui.confirm && !pm_page() && !pm_down() && !gbsv_keys_on() && !sg_page(); }

#define PM_N 8u
#define PM_NM 8u
#define PM_B 32u
typedef struct { uint8_t n; uint8_t trk[PM_N], id[PM_N]; int16_t val[PM_N]; } perf_mac_t;
static perf_mac_t pm_mac[PM_NM];
static struct {
    uint8_t n;
    uint8_t mac[PM_NM], key[PM_NM];
    uint32_t t0;
    uint8_t nb, btrk[PM_B], bid[PM_B], bedit[PM_B];
    int16_t bval[PM_B];
} pm_h;
static uint8_t pm_fade, pm_func = 2, pm_arp = 2;
static uint8_t pm_rel;
static struct { uint8_t n, trk[32], id[32]; int16_t from[32], to[32]; uint32_t t0, dur; } pm_rl;
static uint8_t pm_ui_a = 255, pm_ui_c = 255;
static int16_t pm_ui_u;
static int pm_page(void) { return !ui.home && !ui.confirm && cur_page()->graph == GR_PERF && !fxt_x(cur_page()); }
static int pm_down(void) { return pm_h.n != 0u; }
static int fxl_on(void) { return fxl_sel && sgrid_on(); }
static int16_t *sq_lenp(void) { return fxl_on() ? &fxl.len : &TSEL->p[P_SLEN]; }
static uint32_t sq_len(void) { return fxl_on() ? fxl_len() : (uint32_t)TSEL->p[P_SLEN]; }

static void fxl_refresh(void)
{
    uint32_t k;
    fm1_irq_off();
    mg_ov_n = 0;
    for (k = 0; song.playing && k < fxl_len(); k++)
        if ((fxl.trig >> ((fxl_pos + fxl_len() - k) % fxl_len())) & 1u) {
            fxl_step((fxl_pos + fxl_len() - k) % fxl_len());
            break;
        }
    fm1_irq_on();
}

static uint8_t par_k, par_pg;
static int par_on(void)
{
    return FELUCCA_SIMPLE && ui.sgrid == 2u && !ui.confirm && !pm_page() && !pm_down() && !gbsv_keys_on() &&
           lock_page_ok(par_pg);
}

static int kit_on(void)
{
    return FELUCCA_SIMPLE && ui.sgrid == 3u && !ui.confirm && !pm_page() && !pm_down() && !gbsv_keys_on();
}

static int acid_kon(void)
{
    return FELUCCA_SIMPLE && ui.sgrid == 4u && !fxl_sel && !ui.confirm && !pm_page() && !pm_down() && !gbsv_keys_on();
}
static void acid_page(uint32_t n)
{
    uint32_t i;
    for (i = 0; i < NPAGES; i++)
        if (PAGES[i].graph == GR_ACID && acid_pg(&PAGES[i]) == n) {
            ui.page = (uint8_t)i;
            ui.fam_last[FAM_SEQ] = (uint8_t)i;
            ui.home = 0;
            page_entered();
            ui.force = 1;
            return;
        }
}

#define TM_MS 700u
#define TM_SHOW_MS 1000u
static uint32_t tm_t;
static uint32_t gb_nd_t;

#define HINT_MS 500u
static uint8_t gb_hint;
static uint8_t tm_dn, tm_up, tm_k = 0xFFu;

static const char *const TM_NAME[6] = {"LFO", "AMP ENV", "FILT ENV", "LFO 1", "LFO 2", "LFO 3"};
static const uint8_t TM_SRC[3] = {MS_LFO, MS_ENV, MS_FENV};
static int16_t *tm_slot(track_t *t) { return &t->p[P_M1SRC + 3u * ((tm_dn - 1u) % 3u)]; }

static uint32_t tm_fxk(void) { return 2u - (tm_dn - 1u) % 3u; }
static int16_t *tm_fxd(void) { return &song.g[FXL_ID[tm_fxk()][2]]; }
static int16_t *tm_fxa(void) { return &song.g[FXL_ID[tm_fxk()][3]]; }
static uint32_t tm_fxmd(const page_t *pg, uint32_t k)
{
    uint32_t id = page_id_an(pg, pg_kid(pg, k)), j;
    if (id == 0xFFu || pg->graph == GR_PERF || pg->fam == FAM_TRK || !(pg->scope == SC_GLOBAL || (id & 0x80u)))
        return 0;
    for (j = 1; j < NELEM(FXD_G); j++)
        if (FXD_G[j] == (id & 0x7Fu))
            return j;
    return 0;
}
static uint32_t tm_dest(void)
{
    const int16_t *sp = tm_slot(TSEL);
    if (fxl_sel)
        return (uint32_t)*tm_fxd();
    return sp[0] == TM_SRC[(tm_dn - 1u) % 3u] ? (uint32_t)sp[1] : 0u;
}
static int16_t *tm_amt(void) { return fxl_sel ? tm_fxa() : &tm_slot(TSEL)[2]; }

static uint32_t tm_md(const page_t *pg, uint32_t k)
{
    uint32_t id = page_id_an(pg, pg_kid(pg, k)), j;
    if (pg->scope == SC_FM6 || id == 0xFFu || pg->graph == GR_PERF || pg->scope == SC_TRK || pg->scope == SC_STEP || pg->fam == FAM_TRK)
        return 0;
    if ((id & 0x80u) || pg->scope == SC_GLOBAL)
        id = 0x80u | (id & 0x7Fu);
    else if (id >= P_E0 && id < P_E7)
        return MD_E1 + id - P_E0;
    else if (id == P_DIST)
        return MD_DIST;
    else if (id == P_TRANS || id == P_LRATE || id == P_LD_PIT || id == P_DLY || id == P_REV || id == P_PAN ||
             id == P_FLT_CUT)
        return id == P_TRANS ? MD_PITCH : id == P_LRATE ? MD_RATE : id == P_LD_PIT ? MD_VIB : id == P_DLY ? MD_DLY :
               id == P_REV ? MD_REV : id == P_FLT_CUT ? MD_CUT : MD_PAN;
    for (j = 0; j < MD_N - MD_X0; j++)
        if (MDX[j] == id)
            return MD_X0 + j;
    return 0;
}

static const char *tm_name(const track_t *t, uint32_t d)
{
    if (d >= MD_X0 && d < MD_N && MDX[d - MD_X0] >= P_LN0 && MDX[d - MD_X0] <= P_LN2)
        return track_desc(t, MDX[d - MD_X0])->label;
    return MD_ENGINE(d) ? track_desc(t, P_E0 + d - MD_E1)->label : N_MDST[d % MD_N];
}

#define PM_G 0xFFu
static int16_t *pm_ptr(uint32_t tr, uint32_t id)
{
    return tr >= NTRK ? &song.g[id % G_COUNT] : &trk[tr].p[id];
}
static int pm_find(const perf_mac_t *m, uint32_t tr, uint32_t id)
{
    uint32_t i;
    for (i = 0; i < m->n; i++)
        if (m->trk[i] == tr && m->id[i] == id)
            return (int)i;
    return -1;
}
static int pm_bfind(uint32_t tr, uint32_t id)
{
    uint32_t i;
    for (i = 0; i < pm_h.nb; i++)
        if (pm_h.btrk[i] == tr && pm_h.bid[i] == id)
            return (int)i;
    return -1;
}

static int xf_vp(const int16_t *vp);
static int mom_down(void);
static uint64_t lock_held(void);
static int pm_altered(const track_t *t, const int16_t *vp)
{
    if (vp && xf_vp(vp) >= 0)
        return 1;
    if (!pm_h.n || !vp)
        return 0;
    if (vp >= song.g && vp < song.g + G_COUNT)
        return pm_bfind(PM_G, (uint32_t)(vp - song.g)) >= 0;
    if (vp < t->p || vp >= t->p + P_COUNT)
        return 0;
    return pm_bfind((uint32_t)(t - trk), (uint32_t)(vp - t->p)) >= 0;
}

static int32_t pm_mv(int m, uint32_t b)
{
    if (m >= 0) {
        int i = pm_find(&pm_mac[m], pm_h.btrk[b], pm_h.bid[b]);
        if (i >= 0)
            return pm_mac[m].val[i];
    }
    return pm_h.bval[b];
}
static void pm_put(uint32_t b, int32_t v)
{
    int16_t *vp = pm_ptr(pm_h.btrk[b], pm_h.bid[b]);
    if (*vp != v)
        *vp = (int16_t)v;
}
static int pm_badd(uint32_t tr, uint32_t id, int16_t was)
{
    int b = pm_bfind(tr, id);
    if (b >= 0)
        return b;
    if (pm_h.nb >= PM_B)
        return -1;
    b = (int)pm_h.nb++;
    pm_h.btrk[b] = (uint8_t)tr;
    pm_h.bid[b] = (uint8_t)id;
    pm_h.bedit[b] = 0;
    pm_h.bval[b] = was;
    return b;
}
static uint32_t pm_step_ms(void) { return beat_samples() * 250u / (uint32_t)FS; }
static uint32_t pm_time(void)
{
    static const uint8_t ST[4] = {1, 2, 16, 64};
    return pm_fade ? pm_step_ms() * ST[(pm_fade - 1u) & 3u] : 0u;
}
static uint32_t pm_rel_ms(void)
{
    return pm_step_ms() * pm_rel;
}
static void pm_rl_rebase(void)
{
    uint32_t i;
    for (i = 0; i < pm_rl.n; i++)
        pm_rl.from[i] = *pm_ptr(pm_rl.trk[i], pm_rl.id[i]);
    pm_rl.t0 = fm1_ms;
}
static void pm_rl_snap(uint32_t tr, uint32_t id)
{
    uint32_t i;
    for (i = 0; i < pm_rl.n; i++)
        if (pm_rl.trk[i] == tr && pm_rl.id[i] == id) {
            *pm_ptr(tr, id) = pm_rl.to[i];
            pm_rl.n--;
            pm_rl.trk[i] = pm_rl.trk[pm_rl.n]; pm_rl.id[i] = pm_rl.id[pm_rl.n];
            pm_rl.from[i] = pm_rl.from[pm_rl.n]; pm_rl.to[i] = pm_rl.to[pm_rl.n];
            return;
        }
}
static void pm_rl_step(void)
{
    uint32_t i, el = fm1_ms - pm_rl.t0;
    if (!pm_rl.n)
        return;
    for (i = 0; i < pm_rl.n; i++)
        *pm_ptr(pm_rl.trk[i], pm_rl.id[i]) = (int16_t)(el >= pm_rl.dur ? pm_rl.to[i]
            : pm_rl.from[i] + (int32_t)(((int32_t)pm_rl.to[i] - pm_rl.from[i]) * (int32_t)(el * 1024u / pm_rl.dur) >> 10));
    if (el >= pm_rl.dur)
        pm_rl.n = 0;
}

static uint32_t pm_order(uint8_t *seq)
{
    uint32_t i, j, n = pm_h.n, c = 0;
    uint8_t tmp[PM_NM];
    for (i = 0; i < n; i++)
        tmp[i] = pm_h.mac[i];
    if (pm_arp != 4u)
        for (i = 1; i < n; i++)
            for (j = i; j > 0 && tmp[j - 1u] > tmp[j]; j--) {
                uint8_t x = tmp[j];
                tmp[j] = tmp[j - 1u];
                tmp[j - 1u] = x;
            }
    if (pm_arp == 1u)
        for (i = 0; i < n; i++)
            seq[c++] = tmp[n - 1u - i];
    else {
        for (i = 0; i < n; i++)
            seq[c++] = tmp[i];
        if (pm_arp == 2u)
            for (i = n >= 2u ? n - 2u : 0u; i > 0; i--)
                seq[c++] = tmp[i];
    }
    return c;
}

static void pm_update(void)
{
    uint32_t el, T, i, n = pm_h.n;
    pm_rl_step();
    perf_rel_sl = pm_rel ? (int32_t)(32768u / (pm_rel_ms() * (uint32_t)FS / 1000u + 1u) + 1u) : 0;
    if (!n) {
        pm_ui_c = pm_ui_a = 255;
        return;
    }
    el = fm1_ms - pm_h.t0;
    T = pm_time();
    if (n <= 2u) {
        int a = n == 2u ? (int)pm_h.mac[0] : -1, c = (int)pm_h.mac[n - 1u];
        int32_t u;
        if (!T)
            u = n == 2u ? 0 : 1024;
        else if (pm_func == 0u)
            u = el >= T ? 1024 : (int32_t)(el * 1024u / T);
        else if (pm_func == 1u)
            u = (int32_t)((el % T) * 1024u / T);
        else {
            uint32_t ph = el % (2u * T);
            u = (int32_t)((ph < T ? ph : 2u * T - ph) * 1024u / T);
        }
        pm_ui_a = (uint8_t)a;
        pm_ui_c = (uint8_t)c;
        pm_ui_u = (int16_t)u;
        for (i = 0; i < pm_h.nb; i++) {
            int32_t va, vb;
            if (pm_h.bedit[i])
                continue;
            va = pm_mv(a, i);
            vb = pm_mv(c, i);
            pm_put(i, va + (((vb - va) * u) >> 10));
        }
    } else {
        uint8_t seq[2u * PM_NM];
        uint32_t cnt = pm_order(seq), step = el / (T ? T : pm_step_ms() * 2u), idx;
        if (pm_arp == 3u) {
            uint32_t h = step * 2654435761u + 0x9E3779B9u;
            h ^= h >> 15;
            h *= 2246822519u;
            h ^= h >> 13;
            idx = (h >> 8) % cnt;
        } else if (pm_func == 0u)
            idx = step < cnt ? step : cnt - 1u;
        else if (pm_func == 1u)
            idx = step % cnt;
        else {
            uint32_t ph = cnt > 1u ? step % (2u * cnt - 2u) : 0u;
            idx = ph < cnt ? ph : 2u * cnt - 2u - ph;
        }
        pm_ui_a = 255;
        pm_ui_c = seq[idx];
        pm_ui_u = (int16_t)(((el % (T ? T : pm_step_ms() * 2u)) * 1024u) / (T ? T : pm_step_ms() * 2u));
        for (i = 0; i < pm_h.nb; i++)
            if (!pm_h.bedit[i])
                pm_put(i, pm_mv((int)seq[idx], i));
    }
}

static void pm_begin(uint32_t mac, uint32_t key)
{
    perf_mac_t *m;
    uint32_t i;
    if (pm_h.n >= PM_NM || mac >= PM_NM)
        return;
    for (i = 0; i < pm_h.n; i++)
        if (pm_h.mac[i] == mac)
            return;
    m = &pm_mac[mac];
    pm_h.mac[pm_h.n] = (uint8_t)mac;
    pm_h.key[pm_h.n] = (uint8_t)key;
    pm_h.n++;
    for (i = 0; i < m->n; i++) {
        pm_rl_snap(m->trk[i], m->id[i]);
        pm_badd(m->trk[i], m->id[i], *pm_ptr(m->trk[i], m->id[i]));
    }
    pm_h.t0 = fm1_ms;
    pm_update();
}

static void pm_restore(uint32_t only_unused)
{
    uint32_t i, j, w = 0, rel = pm_rel_ms();
    if (rel)
        pm_rl_rebase();
    for (i = 0; i < pm_h.nb; i++) {
        int used = 0;
        if (only_unused)
            for (j = 0; j < pm_h.n && !used; j++)
                used = pm_find(&pm_mac[pm_h.mac[j]], pm_h.btrk[i], pm_h.bid[i]) >= 0;
        if (used) {
            pm_h.btrk[w] = pm_h.btrk[i]; pm_h.bid[w] = pm_h.bid[i]; pm_h.bedit[w] = pm_h.bedit[i]; pm_h.bval[w] = pm_h.bval[i];
            w++;
        } else if (rel && pm_rl.n < 32u && *pm_ptr(pm_h.btrk[i], pm_h.bid[i]) != pm_h.bval[i]) {
            uint32_t r = pm_rl.n++;
            pm_rl.trk[r] = pm_h.btrk[i]; pm_rl.id[r] = pm_h.bid[i];
            pm_rl.from[r] = *pm_ptr(pm_h.btrk[i], pm_h.bid[i]); pm_rl.to[r] = pm_h.bval[i];
        } else
            *pm_ptr(pm_h.btrk[i], pm_h.bid[i]) = pm_h.bval[i];
    }
    if (rel) {
        pm_rl.t0 = fm1_ms;
        pm_rl.dur = rel;
    }
    pm_h.nb = (uint8_t)w;
}

static void pm_drop(uint32_t i)
{
    uint32_t j;
    for (j = i; j + 1u < pm_h.n; j++) {
        pm_h.mac[j] = pm_h.mac[j + 1u];
        pm_h.key[j] = pm_h.key[j + 1u];
    }
    pm_h.n--;
    pm_restore(pm_h.n ? 1u : 0u);
    if (!pm_h.n)
        pm_h.nb = 0;
    pm_h.t0 = fm1_ms;
}
static void pm_clear(void)
{
    if (pm_h.n != 1u)
        return;
    pm_restore(0u);
    pm_mac[pm_h.mac[0]].n = 0;
    pm_h.nb = 0;
}

#define XF_CC_A 102u
#define XF_CC_B 103u
#define XF_CC_POS 104u
#define XF_SLEW_MS 20u
#define XF_N (2u * PM_N)
#define XF_ALL 0xFEu
static struct {
    volatile uint8_t a, b, pos;
    uint8_t n, trk[XF_N], id[XF_N];
    int16_t base[XF_N], keep[XF_N];
    int32_t u;
    uint32_t t;
} xf;

static int xf_cc(uint32_t cc, uint32_t v)
{
    if (cc == XF_CC_POS)
        xf.pos = (uint8_t)v;
    else if (cc != XF_CC_A && cc != XF_CC_B)
        return 0;
    else if (v <= PM_NM) {
        if (cc == XF_CC_A)
            xf.a = (uint8_t)v;
        else
            xf.b = (uint8_t)v;
    }
    return 1;
}
static int xf_find(uint32_t tr, uint32_t id)
{
    uint32_t i;
    for (i = 0; i < xf.n; i++)
        if (xf.trk[i] == tr && xf.id[i] == id)
            return (int)i;
    return -1;
}
static int xf_vp(const int16_t *vp)
{
    uint32_t i;
    for (i = 0; i < xf.n; i++)
        if (pm_ptr(xf.trk[i], xf.id[i]) == vp)
            return (int)i;
    return -1;
}
static void xf_mbase(const int16_t *vp, int16_t v)
{
    uint32_t k, id, f = motion_guard();
    for (k = 0; k < NTRK; k++)
        for (id = 0; id < MOTION_N; id++)
            if (motion_vp(&trk[k], id) == vp)
                motion_base[k][id] = v;
    motion_unguard(f);
}
static void xf_drop(uint32_t i)
{
    int16_t *vp = pm_ptr(xf.trk[i], xf.id[i]);
    int b;
    uint32_t j;
    pm_rl_snap(xf.trk[i], xf.id[i]);
    b = pm_bfind(xf.trk[i], xf.id[i]);
    if (b >= 0)
        pm_h.bval[b] = xf.base[i];
    else
        *vp = xf.base[i];
    xf_mbase(vp, xf.base[i]);
    for (j = i; j + 1u < xf.n; j++) {
        xf.trk[j] = xf.trk[j + 1u];
        xf.id[j] = xf.id[j + 1u];
        xf.base[j] = xf.base[j + 1u];
    }
    xf.n--;
}

static void xf_leave(uint32_t tr)
{
    uint32_t i;
    for (i = xf.n; i > 0; i--)
        if (tr == XF_ALL || xf.trk[i - 1u] == tr)
            xf_drop(i - 1u);
}

static void xf_save(int begin)
{
    uint32_t i;
    if (begin)
        smp_freeze = 1;
    for (i = 0; i < xf.n; i++) {
        int16_t *vp = pm_ptr(xf.trk[i], xf.id[i]);
        if (begin) {
            xf.keep[i] = *vp;
            *vp = xf.base[i];
            xf_mbase(vp, xf.base[i]);
        } else {
            *vp = xf.keep[i];
        }
    }
    if (!begin)
        smp_freeze = 0;
}
static int32_t xf_end(const perf_mac_t *m, uint32_t tr, uint32_t id, int32_t base)
{
    int i = m ? pm_find(m, tr, id) : -1;
    return i >= 0 ? m->val[i] : base;
}

static void xf_knob(track_t *t, const int16_t *vp, int16_t old)
{
    int i = xf_vp(vp);
    int32_t v;
    (void)t;
    if (i < 0 || *vp == old)
        return;
    v = xf.base[i] + (*vp - old);
    if (xf.trk[i] == PM_G)
        v = clamp(v, GP[xf.id[i]].min, GP[xf.id[i]].max);
    else {
        const param_desc_t *d = param_desc_of(eng_idx(trk[xf.trk[i]].eng_req), xf.id[i]);
        if (d->max > d->min)
            v = clamp(v, d->min, d->max);
    }
    xf.base[i] = (int16_t)v;
}

static void xf_poll(void)
{
    uint32_t a = xf.a, b = xf.b, i, k, dt;
    const perf_mac_t *ma = a ? &pm_mac[a - 1u] : 0, *mb = b ? &pm_mac[b - 1u] : 0;
    int32_t tgt = (int32_t)xf.pos * (1024 << 8) / 127, st, u;
    dt = fm1_ms - xf.t;
    xf.t = fm1_ms;
    if (dt > 100u)
        dt = 100u;
    if (dt) {
        st = (tgt - xf.u) * (int32_t)dt / (int32_t)(dt + XF_SLEW_MS);
        xf.u = st ? xf.u + st : tgt;
    }
    for (i = xf.n; i > 0; i--) {
        uint32_t tr = xf.trk[i - 1u], id = xf.id[i - 1u];
        if (!(ma && pm_find(ma, tr, id) >= 0) && !(mb && pm_find(mb, tr, id) >= 0))
            xf_drop(i - 1u);
    }
    for (k = 0; k < 2u; k++) {
        const perf_mac_t *m = k ? mb : ma;
        for (i = 0; m && i < m->n; i++) {
            int bb;
            if (xf_find(m->trk[i], m->id[i]) >= 0 || xf.n >= XF_N)
                continue;
            pm_rl_snap(m->trk[i], m->id[i]);
            bb = pm_bfind(m->trk[i], m->id[i]);
            xf.trk[xf.n] = m->trk[i];
            xf.id[xf.n] = m->id[i];
            xf.base[xf.n] = bb >= 0 ? pm_h.bval[bb] : *pm_ptr(m->trk[i], m->id[i]);
            xf.n++;
        }
    }
    u = xf.u >> 8;
    for (i = 0; i < xf.n; i++) {
        uint32_t tr = xf.trk[i], id = xf.id[i], g, gl = 0;
        int32_t va, vb, v;
        int16_t *vp = pm_ptr(tr, id);
        if (pm_bfind(tr, id) >= 0)
            continue;
        for (g = 0; g < pm_rl.n && !gl; g++)
            gl = pm_rl.trk[g] == tr && pm_rl.id[g] == id;
        if (gl)
            continue;
        va = xf_end(ma, tr, id, xf.base[i]);
        vb = xf_end(mb, tr, id, xf.base[i]);
        v = va + (((vb - va) * u + 512) >> 10);
        if (*vp != v)
            *vp = (int16_t)v;
    }
}

static int pm_global_ok(uint32_t id)
{
    return id == G_BPM || id == G_SWING || (id >= G_DTIME && id <= G_CDEPTH) || id == G_MIDI || id == G_SYNC || id == G_INFO ||
           (id >= G_F1RATE && id < G_COUNT);
}

#if TRAP_UX_TEST
static struct { int16_t *vp, before, after; uint32_t at; uint8_t tr; } ux_edit;
static void ux_edit_take(track_t *t, int16_t *vp, int16_t old) {
    if(*vp==old) return;
    if(ux_edit.vp!=vp || ux_edit.tr!=(uint8_t)(t-trk) || ux_edit.after!=old || fm1_ms-ux_edit.at>400u)
        ux_edit.before=old;
    ux_edit.vp=vp;ux_edit.after=*vp;ux_edit.at=fm1_ms;ux_edit.tr=(uint8_t)(t-trk);
}
#endif

static void pm_record(track_t *t, int16_t *vp, int16_t old)
{
    perf_mac_t *m;
#if TRAP_UX_TEST && TRAP_FM6_TEST
    tf6_ui_commit(vp);
#endif
#if TRAP_UX_TEST
    if(!pm_h.n && !lock_held() && !mom_down()) ux_edit_take(t,vp,old);
#endif
    uint32_t id, tr = (uint32_t)(t - trk);
    int i, b;
    if (!pm_h.n && !lock_held() && !mom_down())
        xf_knob(t, vp, old);
    if (pm_h.n != 1u || *vp == old)
        return;
    if (vp >= song.g && vp < song.g + G_COUNT) {
        id = (uint32_t)(vp - song.g);
        tr = PM_G;
        if (!pm_global_ok(id))
            return;
    } else {
        if (vp < t->p || vp >= t->p + P_COUNT)
            return;
        id = (uint32_t)(vp - t->p);
        if (!motion_param(id) && !(id >= P_M1SRC && id <= P_M4AMT) && id != P_FTYPE && id != P_ESYNC)
            return;
    }
    m = &pm_mac[pm_h.mac[0]];
    i = pm_find(m, tr, id);
    if (i < 0) {
        if (m->n >= PM_N) {
            ui_message("MACRO FULL");
            return;
        }
        i = (int)m->n++;
        m->trk[i] = (uint8_t)tr;
        m->id[i] = (uint8_t)id;
    }
    b = pm_badd(tr, id, old);
    if (b >= 0)
        pm_h.bedit[b] = 1;
    m->val[i] = *vp;
}

static void cursor_set(int32_t c)
{
    int32_t len = TSEL->p[P_SLEN] > 0 ? TSEL->p[P_SLEN] : 1;
    ui.cursor = (uint8_t)((c % len + len) % len);
    ui.bank = (uint8_t)(ui.cursor / 16u);
    ui.entry_open = 0;
}

static void cursor_fix(void)
{
    if (ui.cursor >= (uint32_t)TSEL->p[P_SLEN])
        cursor_set(TSEL->p[P_SLEN] - 1);
}

#define FOLLOW_MS 3000u
static struct {
    uint32_t t;
    uint16_t prev, steps;
    uint8_t hold;
} follow;

static void follow_hold(void)
{
    follow.hold = 1;
    follow.t = fm1_ms;
    follow.prev = TSEL->seq_idx;
    follow.steps = 0;
    ui.bank = (uint8_t)(ui.cursor / 16u);
}

static void follow_poll(void)
{
    uint32_t len = (uint32_t)TSEL->p[P_SLEN], want;
    if (!song.playing || ui.home || ui.confirm || cur_page()->graph != GR_ROLL || drum_track(TSEL)) {
        follow.hold = 0;
        return;
    }
    if (len <= 16u)
        return;
    if (ui.entry_open)
        follow_hold();
    if (follow.hold) {
        uint32_t idx = TSEL->seq_idx;
        follow.steps = (uint16_t)(follow.steps + (idx + len - follow.prev) % len);
        follow.prev = (uint16_t)idx;
        if (fm1_ms - follow.t < FOLLOW_MS && follow.steps < len)
            return;
        follow.hold = 0;
    }
    want = TSEL->seq_idx / 16u;
    if (want * 16u < len && want != ui.bank)
        ui.bank = (uint8_t)want;
}

static void open_family(uint32_t fam)
{
    if (!ui.home && cur_page()->fam == fam) {
        uint32_t n, i = ui.page;
        for (n = 0; n < NPAGES; n++) {
            i = (i + 1u) % NPAGES;
            if (PAGES[i].fam == fam && page_visible(i)) break;
        }
        ui.page = (uint8_t)i;
    } else if (fam == FAM_SAVE) {
        uint32_t i;
        for (i = 0; i < NPAGES; i++)
            if (PAGES[i].graph == GR_USER) break;
        if (i < NPAGES)
            ui.page = (uint8_t)i;
    } else {
        uint32_t i = ui.fam_last[fam];
        ui.page = (uint8_t)(i && i < NPAGES && PAGES[i].fam == fam && page_visible(i) ? i : page_first(fam));
    }
    ui.fam_last[fam] = ui.page;
    ui.home = 0;
    page_entered();
}

static void go_home(void)
{
    ui.home = 1;
    ui.plk_src = 0;
    ui.entry_open = 0;
    ui.hot_t = 0;
    song.seq_mode = 0;
    ui.force = 1;
    help_start();
}

static void nav_poll(void);
static void nav_swap(void);
static void home_tap(void)
{
    uint32_t i = ui.page + 1u;
    (void)i;
    nav_poll();
    if (!ui.home && cur_page()->fam == FAM_TRK) {
        open_family(FAM_TRK);
    } else {
        ui.home = 0;
        open_family(FAM_TRK);
    }
    return;
    if (ui.home) {
        ui.fam_last[FAM_TRK] = 0;
        open_family(FAM_TRK);
    } else if (cur_page()->fam == FAM_TRK && i < NPAGES && PAGES[i].fam == FAM_TRK) {
        open_family(FAM_TRK);
    } else {
        go_home();
    }
}

enum { UNDO_SOUND = 1, UNDO_PAT = 2, UNDO_MOT = 4 };
static struct {
    uint8_t trk;
    uint8_t keep;
    uint8_t what;
    uint8_t eng, preset, user, patn;
#if TRAP_UX_TEST && TRAP_FM6_TEST
    int16_t fm6[158];
#endif
    int16_t p[P_COUNT];
    step_t step[NSTEP];
    gen_recipe_t gen;
    uint8_t last_n,last_note[4];
    motion_store_t motion_backup;
    uint32_t after;
    uint32_t pat;
    uint32_t from_h;
    uint8_t from;
    uint32_t t_ms;
} undo __attribute__((section(".pool")));
static uint8_t undo_depth;

static uint32_t pat_sig[NTRK];

static uint8_t pat_last[NTRK];

static uint8_t pat_from[NTRK];
static uint32_t pat_from_h[NTRK];
static uint8_t load_from;
static uint8_t load_mo;

static uint32_t fnv(uint32_t h, const void *p, uint32_t n)
{
    const uint8_t *b = (const uint8_t *)p;
    while (n--)
        h = (h ^ *b++) * 16777619u;
    return h;
}
static uint32_t steps_sig(const track_t *t) { return fnv(2166136261u, t->step, sizeof t->step); }
static uint32_t track_sig(const track_t *t)
{
    uint8_t id[3] = {t->eng_req, t->preset, t->user};
    uint32_t h = fnv(fnv(steps_sig(t), t->p, sizeof t->p), id, 3), k = trk_index(t);
    for (uint32_t j = 0; j < motion.count; j++)
        if ((motion.event[j].place >> 6) == k) h = fnv(h, &motion.event[j], sizeof motion.event[j]);
    return h ^ ((motion.on >> k) & 1u);
}
static uint32_t motion_sig(const track_t *t)
{
    uint32_t h = 2166136261u, k = trk_index(t);
    for (uint32_t j = 0; j < motion.count; j++)
        if ((motion.event[j].place >> 6) == k) h = fnv(h, &motion.event[j], sizeof motion.event[j]);
    return h ^ ((motion.on >> k) & 1u);
}

static uint32_t pat_from_sig(const track_t *t)
{
    int16_t q = t->p[P_SQNT];
    uint32_t h = fnv(fnv(steps_sig(t), &t->p[P_SLEN], 4u * sizeof t->p[0]), &q, sizeof q);
    return h ^ motion_sig(t);
}

static void load_motion_hold(track_t *t, uint32_t what)
{
    if (what & UNDO_PAT) {
        gen_recipe[trk_index(t)].active=0;gen_undo_ready=0;
        motion_reset(t);
        load_mo = 0;
        return;
    }
    load_from = t->eng_req;
    load_mo = (uint8_t)(1u | (motion_enabled(t) ? 2u : 0u));
    motion_set_enabled(t, 0);
}

#define MOM_N 8u
static struct {
    uint8_t n;
    int16_t *vp[MOM_N];
    int16_t v[MOM_N];
} mom __attribute__((section(".pool")));
static void mom_restore(void)
{
    while (mom.n) {
        mom.n--;
        *mom.vp[mom.n] = mom.v[mom.n];
    }
}
static void mom_back(const track_t *t)
{
    uint32_t i, k = 0;
    for (i = 0; i < mom.n; i++)
        if (!t || (mom.vp[i] >= t->p && mom.vp[i] < t->p + P_COUNT) || (mom.vp[i] >= song.g && mom.vp[i] < song.g + G_COUNT)) {
            *mom.vp[i] = mom.v[i];
        } else {
            mom.vp[k] = mom.vp[i];
            mom.v[k++] = mom.v[i];
        }
    mom.n = (uint8_t)k;
}
static void load_begin(track_t *t, uint32_t what)
{
    uint32_t i = trk_index(t);
#if TRAP_UX_TEST
    ux_edit.vp=0;
#endif
    if (what & UNDO_SOUND) {
        mom_back(t);
    }
    if (undo_depth++)
        return;
    motion_restore(t);
    if (!TRAP_UX_TEST && undo.keep && undo.trk == i + 1u && !(undo.what & UNDO_MOT) && track_sig(t) == undo.after) {
        undo.what |= (uint8_t)what;
        load_motion_hold(t, what);
        return;
    }
    undo.trk = (uint8_t)(i + 1u);
    undo.what = (uint8_t)what;
    undo.eng = t->eng_req;
    undo.preset = t->preset;
    undo.user = t->user;
    memcpy(undo.p, t->p, sizeof undo.p);
#if TRAP_UX_TEST && TRAP_FM6_TEST
    memcpy(undo.fm6,tf6_edit[i],sizeof undo.fm6);
#endif
    memcpy(undo.step, t->step, sizeof undo.step);
    undo.gen=gen_recipe[i];undo.last_n=lastc[i];memcpy(undo.last_note,lastn[i],4);
    undo.pat = pat_sig[i];
    undo.from = pat_from[i];
    undo.from_h = pat_from_h[i];
    undo.patn = pat_last[i];
    motion_snapshot_track(t, &undo.motion_backup);
    load_motion_hold(t, what);
}

static void load_end(track_t *t)
{
    if (--undo_depth)
        return;
    if (load_mo & 1u) {
        (void)motion_sound_loaded(t, load_from, t->eng_req);
        if ((load_mo & 2u) && motion_count(t))
            motion_set_enabled(t, 1);
        load_mo = 0;
    }
    motion_rebase(t);
    undo.after = track_sig(t);
    undo.keep = 1;
    undo.t_ms = fm1_ms;
}

static uint32_t undo_mot_tag;

static uint32_t edit_sig(const track_t *t) { return undo.what & UNDO_PAT ? motion_sig(t) ^ steps_sig(t) : motion_sig(t); }
static void edit_undo_take(track_t *t, uint32_t tag, uint32_t what)
{
    uint32_t i = trk_index(t);
    if (tag && tag == undo_mot_tag && undo.keep && undo.trk == i + 1u && undo.what == what &&
        edit_sig(t) == undo.after && fm1_ms - undo.t_ms < 1500u)
        return;
    undo_mot_tag = tag;
    undo.trk = (uint8_t)(i + 1u);
    undo.what = (uint8_t)what;
    undo.eng = t->eng_req;
    undo.preset = t->preset;
    undo.user = t->user;
    memcpy(undo.p, t->p, sizeof undo.p);
#if TRAP_UX_TEST && TRAP_FM6_TEST
    memcpy(undo.fm6,tf6_edit[i],sizeof undo.fm6);
#endif
    memcpy(undo.step, t->step, sizeof undo.step);
    undo.gen=gen_recipe[i];undo.last_n=lastc[i];memcpy(undo.last_note,lastn[i],4);
    undo.pat = pat_sig[i];
    undo.from = pat_from[i];
    undo.from_h = pat_from_h[i];
    undo.patn = pat_last[i];
    motion_snapshot_track(t, &undo.motion_backup);
    undo.keep = 0;
}
static void motion_undo_take(track_t *t, uint32_t tag) { edit_undo_take(t, tag, UNDO_MOT); }
static void motion_undo_done(track_t *t)
{
    undo.after = edit_sig(t);
    undo.keep = 1;
    undo.t_ms = fm1_ms;
}

static int param_kept(uint32_t i);

static void track_defaults_steps(track_t *t)
{
    uint32_t i;
    gen_recipe[trk_index(t)].active=0;
    motion_reset(t);
    for (i = 0; i < NSTEP; i++)
        step_clear(&t->step[i]);
}

static int param_kept(uint32_t i)
{
    return i == P_LEVEL || i == P_PAN || i == P_MUTE || (i >= P_AMODE && i <= P_SGATE) ||
           (i >= P_SLCR && i <= P_SLDEPTH) || i == P_CHRD || i == P_VOIC || i == P_SQNT;
}

static uint32_t preset_orig(const engine_t *e, uint32_t k)
{
    (void)e;
    return k;
}

static void set_engine_of(track_t *t, uint32_t ei);

static void machine_defaults_p(const track_t *t, int16_t *p)
{
    const machine_t *m = trk_machine(t);
    const engine_t *e = ENGINES[m->engine];
    const preset_t *pr = &e->presets[m->preset % e->npresets];
    uint32_t i;
    for (i = 0; i < 7u; i++)
        p[P_E0 + i] = (int16_t)pr->e[i];
    p[P_TRANS] = 0;
    p[P_ATK] = pr->env[0];
    p[P_DEC] = pr->env[1];
    p[P_SUS] = pr->env[2];
    p[P_REL] = pr->env[3];
    if (m->engine == ENGI_ANALOG) {
        p[P_LN0] = AN_DEF[m->sub % AN_N][7];
        p[P_LN1] = 0;
        p[P_DIST] = 0;
        p[P_ED_FX] = AN_ACC_DEF;
        p[P_GLIDE] = 0;
    } else if (m->engine == ENGI_SAMPLER) {
        p[P_LN0] = 100;
        p[P_LN2] = TP[P_LN2].def;
        p[P_DIST] = 0;
        p[P_E5] = 0;
    } else if (m->engine == ENGI_DIGITAL && m->sub == 0u) {
        for (i = P_FM3_ATK; i <= P_FM4_LEVEL; i++)
            p[i] = TP[i].def;
        p[P_LN1] = 0;
    } else if (m->engine == ENGI_MONO) {
        p[P_LN0] = 0;
        p[P_LN1] = 0;
        p[P_VOICE] = V_MONO;
    }
}
static void machine_defaults(track_t *t) {
    machine_defaults_p(t, t->p);
#if TRAP_FM6_TEST
    if(t->eng_req==ENGI_FM6) tf6_load(trk_index(t),0);
#endif
}

static int page_reset(void)
{
    const page_t *pg = cur_page();
    int16_t md[P_COUNT], *vp;
    uint32_t k, n = 0;
    xf_leave(XF_ALL);
    if (ui.home || fxt_x(pg) || pg->scope == SC_TRK || pg->fam == FAM_TRK || pg->graph == GR_PERF)
        return 0;
    for (k = 0; k < P_COUNT; k++)
        md[k] = track_desc(TSEL, k)->def;
    machine_defaults_p(TSEL, md);
    for (k = 0; k < 4u; k++) {
        const param_desc_t *d = page_desc(pg, k, &vp);
        if (!d || !vp || d->max == d->min || vp == &TSEL->p[P_E7])
            continue;
        if (pg->scope == SC_FM6 || pg->scope == SC_TAPE || pg->scope == SC_PHONK || pg->scope == SC_BANKS || pg->scope == SC_GEN85 || pg->scope == SC_TRAPGEN || pg->scope == SC_HATGEN || pg->scope == SC_SLICE)
            *vp = d->def;
        else if (vp >= TSEL->p && vp < TSEL->p + P_COUNT)
            *vp = (int16_t)param_fit(d, md[vp - TSEL->p]);
        else if (vp >= song.g && vp < song.g + G_COUNT)
            *vp = GP[vp - song.g].def;
        n++;
    }
    if(pg->scope==SC_GEN85)gen_fresh[song.sel]=0;
    ui.force = 1;
    return n != 0u;
}

static void machine_set(track_t *t, uint32_t mach)
{
    const machine_t *m = &MACHINES[mach % NMACH];
    xf_leave(trk_index(t));
    fm1_irq_off();
    t->eng_req = m->engine;
    t->p[P_E7] = m->sub;
    machine_defaults(t);
    fm1_irq_on();
    ui.force = 1;
}
static void apply_preset_to(track_t *t, uint32_t pi)
{
    const engine_t *e = ENGINES[t->eng_req % NENGINES];
    uint32_t i;
    xf_leave(trk_index(t));
    load_begin(t, UNDO_SOUND);
    panic_req |= (uint8_t)(1u << trk_index(t));
    t->user = 0;
    if (t == TSEL)
        sync_reload = 1;
#if TRAP_FM6_TEST
    if(t->eng_req==ENGI_FM6) tf6_load(trk_index(t),0);
#endif
    if (!e->npresets) {
        load_end(t);
        return;
    }
    pi = preset_orig(e, pi % e->npresets);
    t->preset = (uint8_t)pi;
    for (i = 0; i < P_E0; i++)
        if (!param_kept(i))
            t->p[i] = TP[i].def;
    for (i = 0; i < 8u; i++)
        t->p[P_E0 + i] = (int16_t)e->presets[pi].e[i];
    t->p[P_ATK] = e->presets[pi].env[0];
    t->p[P_DEC] = e->presets[pi].env[1];
    t->p[P_SUS] = e->presets[pi].env[2];
    t->p[P_REL] = e->presets[pi].env[3];
    t->p[P_ED_FLT] = e->presets[pi].fenv;
    t->p[P_VOICE] = e->presets[pi].mono ? V_LEGATO : V_POLY;
    {
        static const uint8_t FX_DEF[4] = {0, 24, 28, 36};
        const preset_t *pr = &e->presets[pi];
        for (i = 0; i < 4u; i++)
            t->p[P_DIST + i] = (int16_t)(pr->fx[i] ? pr->fx[i] - 1 : FX_DEF[i]);
    }
    load_end(t);
}

static void set_engine_of(track_t *t, uint32_t ei)
{
    const engine_t *e = ENGINES[ei % NENGINES];
    uint32_t i;
    load_begin(t, UNDO_SOUND);
    fm1_irq_off();
    t->eng_req = (uint8_t)(ei % NENGINES);
    for (i = 0; i < 8u; i++)
        t->p[P_E0 + i] = e->edit[i].def;
    apply_preset_to(t, 0);
    fm1_irq_on();
    load_end(t);
}

static void set_engine(uint32_t ei) { set_engine_of(TSEL, ei); }

#define CLEAN_REL 40
static void clean_engine_of(track_t *t)
{
    uint32_t ei = t->eng_req % NENGINES, i;
    const engine_t *e = ENGINES[ei];
    load_begin(t, UNDO_SOUND);
    set_engine_of(t, ei);
    fm1_irq_off();
    for (i = 0; i < P_E0; i++)
        if (!param_kept(i))
            t->p[i] = TP[i].def;
    for (i = 0; i < 8u; i++)
        t->p[P_E0 + i] = e->edit[i].def;
    t->p[P_ATK] = 0;
    t->p[P_SUS] = TP[P_SUS].max;
    t->p[P_REL] = CLEAN_REL;
    fm1_irq_on();
    load_end(t);
}
static void clean_engine(void) { clean_engine_of(TSEL); }

static volatile uint8_t *const mpc_io = ui.mpc;
static void midi_pc_load(track_t *t, uint32_t n)
{
    if (n < ENGINES[t->eng_req % NENGINES]->npresets)
        apply_preset_to(t, n);
}
static void midi_pc_poll(void)
{
    uint32_t k, v;
    for (k = 0; k < NTRK; k++) {
        if (!ui.mpc[k])
            continue;
        fm1_irq_off();
        v = ui.mpc[k];
        ui.mpc[k] = 0;
        fm1_irq_on();
        midi_pc_load(&trk[k], v & 127u);
        ui.force = 1;
    }
}

static void track_defaults(track_t *t)
{
    uint32_t i;
    for (i = 0; i < P_E0; i++)
        t->p[i] = TP[i].def;
    track_defaults_steps(t);
}

static track_t *home_trk(uint32_t k) { return home_levels() ? &trk[k & 3u] : TSEL; }
static const param_desc_t *home_param(uint32_t k, int16_t **vp)
{
    uint32_t id = home_levels() ? (uint32_t)P_LEVEL : ENGINES[TSEL->eng_req % NENGINES]->knob[k & 3u];
    *vp = &home_trk(k)->p[id];
    return track_desc(home_trk(k), id);
}

static uint8_t side_page[2];
static void fxt_set(uint32_t on)
{
    on = on != 0u;
    if (fxl_sel == (uint8_t)on)
        return;
    side_page[fxl_sel] = ui.page;
    nav_poll();
    nav_swap();
    if (on && ui.sgrid == 4u)
        ui.sgrid = 1;
    fxl_sel = (uint8_t)on;
    if (!ui.home && !page_visible(ui.page)) {
        uint32_t i = page_first(PAGES[ui.page].fam);
        if (!page_visible(i))
            i = side_page[on];
        if (!page_visible(i) || (PAGES[i].graph == GR_PERF && ui.sgrid == 1u))
            i = page_first(on ? FAM_FX : FAM_EDIT);
        ui.page = (uint8_t)i;
        page_entered();
    }
    ui.force = 1;
}
static void track_select(uint32_t i)
{
    if (i < NTRK)
        fxt_set(0);
    if (i >= NTRK || i == song.sel)
        return;
    song.sel = (uint8_t)i;
    if (FELUCCA_SIMPLE && song.rec)
        song.rec = (uint8_t)(1u << i);
    ui.entry_open = 0;
    ui.hot_t = 0;
    ui.ev_keep = 0;
    ui.cursor = 0;
    ui.bank = 0;
    follow.hold = 0;
    sync_reload = 1;
    ui.force = 1;
}

#include "trap_bank_a.c"

#include "trap_banks.c"
#include "trap_stage.c"
#include "trap_slice.c"

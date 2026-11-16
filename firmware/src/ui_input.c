/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

static uint8_t led_pos[41];

static void led_pos_init(void)
{
    uint32_t id, p, r;
    for (id = 0; id < 41u; id++) {
        led_pos[id] = 0xFF;
        for (p = 0; p < FM1_NCOL; p++)
            for (r = 1; r < 5u; r++)
                if (FM1_KEYMAP[r][p] == (int8_t)id)
                    led_pos[id] = (uint8_t)((p << 3) | r);
    }
}

static void led_put(uint8_t *nl, uint32_t id, int on)
{
    uint8_t q = led_pos[id];
    if (q != 0xFF && on)
        nl[q >> 3] |= (uint8_t)(1u << (q & 7u));
}

#define LED_PLAY_GREEN ((8u << 3) | 1u)
static void led_clear(uint8_t *nl, uint32_t id)
{
    uint8_t q = led_pos[id];
    if (q != 0xFF)
        nl[q >> 3] &= (uint8_t)~(1u << (q & 7u));
}

static const uint8_t FAM_BTN[FAM_COUNT] = {B_HOME, B_ENV, B_LFO, B_EDIT, B_HOME, B_FX, B_GLO, B_SAVE,

                                           B_ARP, B_SEQ, B_HOME,
                                           B_SCL};

static uint32_t cur_fam(void) { return ui.home ? FAM_HOME : cur_page()->fam; }

static int32_t accel(uint32_t role, int32_t s, int32_t range);

#define OCT_BREATH 4u
#define OCT_BREATH_DN 8u

static uint32_t tm_btn(uint32_t d)
{
    uint32_t i, k;
    for (i = 0; d && i < NPAGES; i++)
        if (page_visible(i))
            for (k = 0; k < 4u; k++)
                if ((fxl_sel ? tm_fxmd(&PAGES[i], k) : tm_md(&PAGES[i], k)) == d)
                    return FAM_BTN[PAGES[i].fam];
    return 0xFFu;
}

static void layer_kill(void);
static int clip_paste_ready(void);

static uint8_t seq_pick, seq_eat;

static int smp_osc1(void)
{
    const page_t *pg = cur_page();
    return FELUCCA_SIMPLE && !ui.home && !page_over && !ui.confirm && !fxl_sel && pg->scope == SC_ENGINE &&
           pg->id[0] == P_E0 && eng_idx(TSEL->eng_req) == ENGI_SAMPLER;
}
static int sl_keys_on(void);
static uint32_t sl_leds(void);
static void hint_poll(void)
{
    static uint32_t t0[2];
    static const uint8_t HB[2] = {B_SEQ, B_OCTUP};
    uint32_t i, h = 0;
    for (i = 0; i < 2u; i++) {
        uint32_t dn = (fm1_in.buttons >> panel.btn[HB[i]]) & 1u;
        if (!dn)
            t0[i] = 0;
        else if (!t0[i])
            t0[i] = fm1_ms | 1u;
        else if (i == 0u) {
            if (!seq_pick && !seq_eat && (int32_t)(fm1_ms - t0[i]) >= (int32_t)TM_MS && !ui.confirm && !gbsv_keys_on()) {
                seq_pick = 1;
                layer_kill();
            }
        } else if (!h && (int32_t)(fm1_ms - t0[i]) >= (int32_t)HINT_MS && !ui.confirm && !gbsv_keys_on() && !smp_osc1()) {
            h = i + 1u;
        }
    }
    if (ui.confirm || gbsv_keys_on() || pm_page())
        seq_pick = 0;
    if (seq_pick)
        h = 1;
    if (!h && clip_paste_ready())
        h = 3;
    if (h != gb_hint)
        ui.force = 1;
    gb_hint = (uint8_t)h;
}

#define FILL_MS 500u

#define CLA_MS 1000u
static int8_t cla_oct;
static void trap_kit_power(void);
static void cla_clear(int all)
{
    uint32_t a, i;
    if (all)
        xf_leave(XF_ALL);
    if (all || fxl_on()) {
        int16_t fl = fxl.len;
        memset(&fxl, 0, sizeof fxl);
        fxl.len = fl;
        fxl_refresh();
    }
    if (!all && fxl_on()) {
        ui_message("FX CLEARED");
        return;
    }
    if (all) {
        for (i = 0; i < G_COUNT; i++)
            if (i != G_BPM && i != G_SWING && i != G_CLOCK && i != G_TUNE && i != G_ROUTE)
                song.g[i] = GP[i].def;
        undo_depth++;
        for (a = 0; a < NTRK; a++) {
            track_t *t = &trk[a];
            kit_undo(t);
            for (i = 0; i < 16u; i++)
                kit[a][i].n = 0;
            fm1_irq_off();
            track_defaults(t);
            fm1_irq_on();
            set_engine_of(t, ENGI_DIGITAL);
            apply_preset_to(t, TRK_DEF[a]);
            acid_reset(&acid[a]);
            lastc[a] = 0;
        }
        trap_kit_power();
        undo_depth--;
        panic_req |= (uint8_t)((1u << NTRK) - 1u);
        ui_message("ALL CLEARED");
        return;
    }
    if (acid_kon()) {
        acid_t *ac = acid_of(song.sel);
        uint8_t pl = ac->play;
        acid_reset(ac);
        ac->play = pl;
        ui_message("ACID CLEARED");
        return;
    }
    for (i = 0; i < NSTEP; i++)
        step_clear(&TSEL->step[i]);
    motion_clear(TSEL);
    lastc[song.sel % NTRK] = 0;
    ui_message("TRACK CLEARED");
}
#define CLR2_MS 800u
static void cla_poll(uint32_t pressed)
{
    static uint32_t t0, t1;
    static uint8_t t1_on;
    uint32_t dn = panel.btn[B_OCTDN], on = fxl_on() ? NTRK : song.sel;
    if ((pressed >> dn) & 1u)
        t0 = fm1_ms | 1u;
    if (!((fm1_in.buttons >> dn) & 1u)) {
        if (cla_st == 1u && !TRAP_UX_TEST) {
            if (t1 && t1_on == on && (int32_t)(fm1_ms - t1) < (int32_t)CLR2_MS) {
                cla_clear(0);
                t1 = 0;
            } else {
                t1 = fm1_ms | 1u;
                t1_on = (uint8_t)on;
                ui_message("OCT- AGAIN TO CLEAR");
            }
        } else if (cla_st == 2u) {
            ui_message("CANCELLED");
            ui.force = 1;
        }
        cla_st = t0 = 0;
        return;
    }
    if (!t0 || cla_st >= 2u || (int32_t)(fm1_ms - t0) < (int32_t)CLA_MS)
        return;
    if ((fm1_in.buttons & ~(1u << dn)) || fm1_in.notes || tm_dn || ui.home || ui.confirm) {
        t0 = 0;
        return;
    }
    cla_st = 2;
    song.octave = cla_oct;
    ui.msg_t = 0;
    ui.force = 1;
}
static uint8_t fill_arm;
static uint32_t fill_t0;
#define PANIC_MS 400u
static uint32_t stop_ms;
static void fill_poll(void)
{
    uint32_t dn = (fm1_in.buttons >> panel.btn[B_PLAY]) & 1u;
    if (!fill_arm)
        return;
    if (dn && song.playing) {
        if (!seq_fill && fm1_ms - fill_t0 >= FILL_MS) {
            seq_fill = 1;
            ui.force = 1;
        }
        return;
    }
    if (!seq_fill && song.playing) {
        transport_req = 2;
        stop_ms = fm1_ms | 1u;
    }
    if (seq_fill)
        ui.force = 1;
    seq_fill = 0;
    fill_arm = 0;
}

static const uint8_t TM_BTN[3] = {B_LFO, B_ENV, B_SCL};
static uint8_t tm_eat;

static uint32_t tm_poll(uint32_t pressed)
{
    static uint32_t t0[3];
    static uint8_t was;
#define eat tm_eat
    uint32_t i, held = 0, ok = FELUCCA_SIMPLE && !ui.confirm && !ui.uboot && !gbsv_keys_on();
    for (i = 0; ok && i < 3u; i++)
        held |= ((fm1_in.buttons >> panel.btn[TM_BTN[i]]) & 1u) << i;
    for (i = 0; i < 3u; i++) {
        uint32_t m = 1u << i;
        if ((held & m) && !(was & m)) {
            t0[i] = tm_dn && tm_dn != i + 1u ? 0u : fm1_ms | 1u;
            if (tm_dn == i + 1u) {
                tm_dn = 0;
                eat |= (uint8_t)m;
                t0[i] = 0;
                ui.force = 1;
            }
        }
        if (!(held & m) || (tm_dn && tm_dn != i + 1u))
            t0[i] = 0;
        else if (t0[i] && !(eat & m)) {
            if ((int32_t)(fm1_ms - t0[i]) >= (int32_t)TM_MS) {
                tm_dn = (uint8_t)(i + 1u);
                tm_k = 0xFFu;
                eat |= (uint8_t)m;
                ui.force = 1;
            }
        }
        if (eat & m) {
            ui.pg_down &= (uint16_t)~(1u << panel.btn[TM_BTN[i]]);
            if (ui.ly)
                layer_kill();
            if (!(held & m))
                eat &= (uint8_t)~m;
        }
    }
    was = (uint8_t)held;
    if (!ok)
        tm_dn = 0;
    if (tm_dn && tm_k != 0xFFu && fm1_ms - tm_t >= TM_SHOW_MS) {
        tm_dn = 0;
        ui.force = 1;
    }
    i = tm_dn && tm_k != 0xFFu;
    if (i != tm_up)
        ui.force = 1;
    tm_up = (uint8_t)i;
    if (tm_dn && ((pressed >> panel.btn[B_OCTDN]) & 1u)) {
        char m[24];
        *tm_amt() = 0;
        if (fxl_sel)
            *tm_fxd() = 0;
        else
            tm_slot(TSEL)[1] = 0;
        str_cpy(m, TM_NAME[fxl_sel ? 3u + tm_fxk() : tm_dn - 1u], sizeof m);
        str_cpy(m + str_len(m), " DEST OFF", sizeof m - str_len(m));
        ui_message(m);
        tm_dn = 0;
        ui.force = 1;
        pressed &= ~(1u << panel.btn[B_OCTDN]);
    }
    return pressed;
#undef eat
}

static void tm_turn(uint32_t k, int32_t s)
{
    int16_t *p = tm_slot(TSEL), src = TM_SRC[(tm_dn - 1u) % 3u];
    uint32_t d = ui.home ? 0u : fxl_sel ? tm_fxmd(cur_page(), k) : tm_md(cur_page(), k);
    if (!d)
        return;
    if (fxl_sel) {
        if (*tm_fxd() != (int16_t)d) {
            *tm_fxa() = 0;
            *tm_fxd() = (int16_t)d;
        } else if (tm_k == k) {
            *tm_fxa() = (int16_t)clamp(*tm_fxa() + accel(EN_K1 + k, s, 127), -64, 63);
        }
    } else if (p[1] != (int16_t)d || p[0] != src) {
        p[2] = 0;
        p[1] = (int16_t)d;
        p[0] = src;
    } else if (tm_k == k) {
        p[2] = (int16_t)clamp(p[2] + accel(EN_K1 + k, s, 127), -64, 63);
    }
    tm_k = (uint8_t)k;
    tm_t = fm1_ms;
    ui.force = 1;
}

static int mom_down(void)
{
    return ((fm1_in.buttons >> panel.btn[FELUCCA_SIMPLE ? B_OCTUP : B_LFO]) & 1u) && !ui.confirm && !ui.uboot;
}
#define MOM_KEEP (FELUCCA_SIMPLE ? B_OCTDN : B_OCTUP)

static int mom_put(const track_t *t, const param_desc_t *d, int16_t *vp, uint32_t k, int32_t s);
static int mom_turn(uint32_t k, int32_t s)
{
    const page_t *pg = cur_page();
    const param_desc_t *d;
    int16_t *vp = 0;
    const track_t *t = ui.home ? home_trk(k) : TSEL;
    if (ui.home) {
        d = home_param(k, &vp);
    } else if (pg->scope == SC_TRK && !lock_held()) {
        uint32_t tk = pg->id[k & 3u], id = tk & MIX_PAN ? P_PAN : P_LEVEL;
        if ((tk & ~MIX_PAN) >= NTRK)
            return 0;
        t = &trk[tk & ~MIX_PAN];
        vp = &trk[tk & ~MIX_PAN].p[id];
        d = &TP[id];
    } else if (FELUCCA_SIMPLE && pg->graph == GR_PERF) {
        static const uint8_t GK[3] = {G_MIDI, G_SYNC, G_CRATE};
        if (k == 0u)
            return 0;
        vp = &song.g[GK[(k - 1u) % 3u]];
        d = &GP[GK[(k - 1u) % 3u]];
    } else {
        if (lock_held() || !(pg->scope == SC_TRACK || pg->scope == SC_ENGINE || pg->scope == SC_GLOBAL) ||
            pg->graph == GR_EVENTS || pg->graph == GR_SONG || pg->graph == GR_BROWSE || pg->graph == GR_USER ||
            pg->graph == GR_PATS || (pg->graph == GR_SLICES && k < 2u) || (pg->graph == GR_MOD && k == 0u))
            return 0;
        d = page_desc(pg, k, &vp);
    }
    return mom_put(t, d, vp, k, s);
}

static int mom_put(const track_t *t, const param_desc_t *d, int16_t *vp, uint32_t k, int32_t s)
{
    uint32_t i, g;
    g = vp && vp >= song.g && vp < song.g + G_COUNT ? (uint32_t)(vp - song.g) : G_COUNT;
    if (!d || !vp || d->max == d->min ||
        !((vp >= t->p && vp < t->p + P_COUNT) || (g < G_COUNT && g != G_CLOCK && g != G_TUNE && g != G_ROUTE)))
        return 0;
    for (i = 0; i < mom.n && mom.vp[i] != vp; i++)
        ;
    if (i == mom.n) {
        if (mom.n >= MOM_N) {
            ui_message("MOMENTARY FULL");
            return 1;
        }
        mom.vp[i] = vp;
        mom.v[i] = *vp;
        mom.n++;
        if (i == 0u && !FELUCCA_SIMPLE)
            ui_message("MOMENTARY  [OCT+] KEEP");
    }
    *vp = (int16_t)param_turn(d, *vp, accel(EN_K1 + k, s, d->fmt == F_ENUM ? 0 : d->max - d->min));
    return 1;
}

static uint32_t oct_leds(void)
{
    if (ui.confirm)
        return 1u | OCT_BREATH;
    if (mom.n && mom_down())
        return FELUCCA_SIMPLE ? 2u : (song.octave < 0 ? 1u : 0u) | OCT_BREATH;
    return (song.octave < 0 ? 1u : 0u) | (song.octave > 0 ? 2u : 0u);
}

static int gbs_trigless(const track_t *t, uint32_t p)
{
    return p < NSTEP && !(t->step[p].time == ST_NOTE && t->step[p].n) &&
           (t->step[p].time == ST_LOCK || ((motion_lock_steps(trk_index(t)) >> p) & 1u));
}
#define GBS_PG (NTRK + 1u)
static uint32_t gbs_base(void);
static uint32_t fxl_leds(uint32_t *br)
{
    uint32_t k, m = 0, len = fxl_len(), ph = song.playing ? fxl_pos : 0xFFu, b = gbs_base(), npg = (len + 15u) / 16u;
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k), on;
        if (!key_black(k))
            on = b + p < len && (((fxl.trig >> (b + p)) & 1u) ^ (uint32_t)(b + p == ph));
        else if (p >= GBS_PG && p < GBS_PG + npg) {
            on = p - GBS_PG == ui.bank;
            if (br && npg == 1u && ph != 0xFFu)
                on = 0;
            if (br && !on && ph != 0xFFu && p - GBS_PG == ph / 16u)
                *br |= 1u << k;
        } else
            on = p == NTRK;
        m |= on << k;
    }
    return m;
}

static uint32_t gbs_base(void)
{
    uint32_t len = sq_len() > 0u ? sq_len() : 1u;
    if (ui.bank * 16u >= len)
        ui.bank = (uint8_t)((len - 1u) / 16u);
    return ui.bank * 16u;
}
static uint32_t sgrid_leds(uint32_t *br)
{
    const track_t *t = TSEL;
    uint32_t k, m = 0, len = (uint32_t)t->p[P_SLEN], b, npg = (len + 15u) / 16u, ppg;
    if (fxl_on())
        return fxl_leds(br);
    b = gbs_base();
    uint32_t ph = song.playing && t->seq_idx < len ? t->seq_idx : 0xFFu;
    ppg = ph != 0xFFu ? ph / 16u : 0xFFu;
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k), i = b + p, on;
        if (!key_black(k)) {
            on = i < len && t->step[i].time == ST_NOTE && t->step[i].n != 0u;
            on ^= (uint32_t)(i == ph);
            if (br && i < len && (gbs_trigless(t, i) || (t->step[i].time == ST_NOTE && t->step[i].n &&
                                                         gb_tc(&t->step[i]) == GB_TC_FILL))) {
                *br |= 1u << k;
                on = i == ph;
            }
        } else if (p >= GBS_PG && p < GBS_PG + npg) {
            on = p - GBS_PG == ui.bank;
            if (br && npg == 1u && ppg != 0xFFu)
                on = 0;
            if (br && !on && p - GBS_PG == ppg)
                *br |= 1u << k;
        } else {
            on = p < NTRK ? p == song.sel : 0u;
        }
        m |= on << k;
    }
    return m;
}

static uint32_t grid_glow(void)
{
    uint32_t k, m = 0, len = (uint32_t)TSEL->p[P_SLEN];
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k);
        m |= (uint32_t)(!key_black(k) ? ui.bank * 16u + p < len : p < NLANE || p == GK_ACC || len > 16u) << k;
    }
    return m;
}

static uint32_t beat_flash(void)
{
    uint32_t bt = 60000u / (uint32_t)clamp(song.g[G_BPM], 20, 999);
    return song.playing ? beat_pos < beat_samples() / 3u : fm1_ms % bt < bt / 3u;
}
static uint32_t arp_led(void)
{
    uint32_t k, on = 0, b = beat_samples();
    if (FELUCCA_SIMPLE)
        return arp_am(TSEL) == AM_OFF ? 2u : beat_flash();
    for (k = 0; k < NPART; k++)
        on |= arp_am(&trk[k]) && trk[k].nheld;
    return !on ? 2u : beat_pos < (beat_n ? b / 6u : b / 2u);
}

static uint32_t midi_leds(const track_t *t)
{
    uint32_t k, note, m = 0, seen[4] = {0, 0, 0, 0};
    if (!midi_owners[trk_index(t)])
        return 0;
    for (k = 0; k < 27u; k++) {
        note = kb_map(t, k);
        if (note > 127u || ((seen[note >> 5] >> (note & 31u)) & 1u))
            continue;
        seen[note >> 5] |= 1u << (note & 31u);
        m |= (uint32_t)midi_note_held(t, note) << k;
    }
    return m;
}

static uint32_t play_leds(void)
{
    const track_t *t = TSEL;
    uint8_t s[4 + NLANE + 4];
    uint32_t n = t->seq_n < 4u + NLANE ? t->seq_n : 4u + NLANE, i, k, note, used = 0, m = 0, hit;
    for (i = 0; i < n; i++)
        s[i] = t->seq_notes[i];
    if (t->arp_note)
        s[n++] = t->arp_note;
    for (i = 0; i < 3u; i++)
        if (t->arp_ch[i])
            s[n++] = t->arp_ch[i];
    for (k = 0; n && k < 27u; k++) {
        note = kb_map(t, k);
        for (i = 0, hit = 0; i < n; i++)
            if (s[i] == note && !((used >> i) & 1u)) {
                used |= 1u << i;
                hit = 1;
            }
        m |= hit << k;
    }
    return m | midi_leds(t);
}

static uint32_t step_leds(void)
{
    const track_t *t = TSEL;
    const step_t *st;
    uint32_t i, k, note, used = 0, m = 0;
    if (ui.home || ui.confirm || !song.seq_mode || cur_page()->graph != GR_ROLL || song.playing ||
        ui.cursor >= NSTEP)
        return 0;
    st = &t->step[ui.cursor];
    if (!st->n || st->time == ST_REST)
        return 0;
    for (k = 0; k < 27u; k++) {
        note = kb_map(t, k);
        for (i = 0; i < st->n && i < 4u; i++)
            if (st->note[i] == note && !((used >> i) & 1u)) {
                used |= 1u << i;
                m |= 1u << k;
            }
    }
    return m;
}

static int keys_own(void)
{
    return grid_on();
}

static uint32_t scale_leds(uint32_t *root)
{
    const track_t *t = TSEL;
    const engine_t *e = ENGINES[eng_idx(t->eng_req)];
    uint32_t k, m = 0, r = 0, mask = scale_mask(t), note;
    int32_t pc;
    *root = 0;
    if (!(ui_rec_prefs & (PREF_SCALE_LEDS >> 8)))
        return 0;
    for (k = 0; k < 27u; k++) {
        if (e->keys && e->keys(t, k) >= 0)
            continue;
        if (t->p[P_QUANT] == QN_WHITE) {
            if ((note = kb_map(t, k)) > 127u)
                continue;
            pc = (int32_t)note - KB_TRN(t);
        } else {
            pc = 53 + (int32_t)k + KB_TRN(t);
        }
        pc = ((pc - t->p[P_ROOT]) % 12 + 12) % 12;
        m |= ((mask >> pc) & 1u) << k;
        r |= (uint32_t)(pc == 0) << k;
    }
    *root = r & m;
    return m;
}

static uint32_t pm_leds(void);
static uint32_t trk_keys(int playing);
static uint32_t par_leds(void);
static uint32_t kit_leds(void);
static uint32_t acid_leds(uint32_t *br);
static void acid_follow(void);

#define IDLE_LEDS_MS 60000u
static int idle_leds(void)
{
    int due = (ui_idle & 1u) && !(ui_prefs & PREF_ANIM_OFF) && !song.playing && !seq_counting() &&
              !song.rec && !transport_req && !ui.confirm && !ui.uboot &&
              !(fm1_in.notes | fm1_in.buttons) && !panel_moved && !play_leds() && fm1_ms - scrn.idle >= IDLE_LEDS_MS;
    if (!due) {
        if (fm1_led_idle_on())
            fm1_led_anim_stop();
        return 0;
    }
    if (!fm1_led_anim_on())
        fm1_led_idle_start(settings_leds == LEDS_DIM || settings_leds == LEDS_DIM_LO);
    return fm1_led_idle_on();
}

static uint32_t sc_leds(void)
{
    int32_t pend = sc_pend;
    uint32_t k, c = 0, m = pend >= 0 ? (uint32_t)pend : (uint32_t)ph_masks[(uint32_t)ph_scene % 4u];
    for (k = 0; k < 27u; k++)
        if (key_black(k) && key_place(k) <= NTRK) {
            uint32_t p = key_place(k), on = song.playing && pend < 0 ? (p < NTRK ? !trk[p].p[P_MUTE] : !fxl_mute)
                                                          : (p < NTRK ? (m >> p) & 1u : !((m >> 6) & 1u));
            if (on)
                c |= 1u << k;
        }
    return c;
}
static uint32_t dj_leds(uint32_t *br)
{
    uint32_t k, c = 0, last = 0, lk = 27u;
    for (k = 0; k < 27u; k++)
        if (key_black(k)) {
            uint32_t p = key_place(k);
            if (p < NTRK) {
                if (p == dj_sel)
                    c |= 1u << k;
                else if (dj_trk[p] && br)
                    *br |= 1u << k;
            }
            if (p >= last) {
                last = p;
                lk = k;
            }
        }
    if (lk < 27u) {
        if (dj_sel >= NTRK)
            c |= 1u << lk;
        else if (song.g[G_MIDI] && br)
            *br |= 1u << lk;
    }
    return c;
}
static uint32_t cb_leds(void)
{
    uint32_t k, c = 0, last = 0, lk = 27u;
    for (k = 0; k < 27u; k++)
        if (key_black(k)) {
            uint32_t p = key_place(k);
            if (p < NTRK && p == cb_sel)
                c |= 1u << k;
            if (p >= last) {
                last = p;
                lk = k;
            }
        }
    if (lk < 27u && cb_sel >= NTRK)
        c |= 1u << lk;
    return c;
}
static uint32_t key_leds(uint32_t *br)
{
    uint32_t c;
    if (br)
        *br = 0;
    c = gbsv_keys_on() ? gbsv_leds() : sg_page() ? sg_leds() : sl_keys_on() ? sl_leds() : dj_keys ? dj_leds(br) : cb_keys ? cb_leds() : sc_keys ? sc_leds() : sgrid_on() ? sgrid_leds(br) : par_on() ? par_leds() | (fm1_in.notes & ~kb_layer) :
        kit_on() ? kit_leds() | (fm1_in.notes & ~kb_layer) :
        acid_kon() ? acid_leds(br) :
                 (fm1_in.notes & ~kb_layer) | play_leds() | step_leds() | (pm_page() ? pm_leds() : 0u);
    if (FELUCCA_SIMPLE && kb_octup && !gbsv_keys_on())
        c = (c & ~trk_keys(0)) | trk_keys(1);
    return c;
}

static void ui_leds(void)
{
    uint8_t nl[FM1_NCOL] = {0}, nd[FM1_NCOL] = {0}, own[FM1_NCOL] = {0}, og[FM1_NCOL] = {0}, nb[FM1_NCOL] = {0};
    uint8_t nm[FM1_NCOL] = {0}, sg[FM1_NCOL] = {0}, sm[FM1_NCOL] = {0}, iv[FM1_NCOL] = {0};
    uint32_t k, c, g, br, beat, sc, sr = 0;
    uint32_t fam = cur_fam(), mode = settings_leds, inv = leds_inv();
    int keys_map = keys_own();
    static uint8_t ready;
    if (idle_leds() || fm1_led_anim_on())
        return;
    if (!ready) {
        led_pos_init();
        ready = 1;
    }
    if (tm_dn) {
        uint32_t b = TM_BTN[(tm_dn - 1u) % 3u];
        led_put(nl, panel.btn[b], (int)beat_flash());
        if ((k = tm_btn(tm_dest())) != 0xFFu && k != b)
            led_put(nl, panel.btn[k], 1);
    } else
        led_put(nl, panel.btn[FAM_BTN[fam]], 1);
    if (sgrid_on())
        led_put(nl, panel.btn[B_SEQ], 1);
    led_put(nb, panel.btn[B_SEQ], FELUCCA_SIMPLE && ui.sgrid == 2u);
    if ((k = arp_led()) != 2u)
        led_put(nl, panel.btn[B_ARP], FAM_BTN[fam] == B_ARP ? !k : (int)k);
    led_put(nl, panel.btn[B_REC], song.rec != 0u);
    k = oct_leds();
    led_put(nl, panel.btn[B_OCTDN], (int)(k & 1u));
    led_put(nl, panel.btn[B_OCTUP], (int)((k >> 1) & 1u));
    led_put(nb, panel.btn[B_OCTUP], (int)(k & OCT_BREATH));
    led_put(nb, panel.btn[B_OCTDN], (k & OCT_BREATH_DN) != 0u);
    c = key_leds(&br);
    g = !keys_map ? 0u : grid_on() ? grid_glow() : 0u;
    beat = 0;
    sc = keys_map ? 0u : scale_leds(&sr);
    for (k = 0; k < 27u; k++) {
        uint32_t in = (sc >> k) & 1u, rt = (sr >> k) & 1u, idle = !((c >> k) & 1u);
        led_put(nm, 14u + k, (int)((beat >> k) & 1u));
        led_put(keys_map ? own : nl, 14u + k, (int)((c >> k) & 1u));
        led_put(keys_map ? og : nd, 14u + k, keys_map ? (int)((g >> k) & 1u) : !sc || (mode == LEDS_INV && rt));
        led_put(nb, 14u + k, (int)((br >> k) & 1u));
        if (!sc)
            continue;
        led_put(sg, 14u + k, mode == LEDS_INV ? !in && idle : (int)in);
        led_put(sm, 14u + k, mode == LEDS_INV ? in && !rt && idle : (int)rt);
    }
    for (k = 0; k < NB; k++)
        led_put(nd, panel.btn[k], 1);
    if (song.playing || seq_counting())
        led_clear(nd, panel.btn[B_PLAY]);
    for (c = 0; c < FM1_NCOL; c++) {
        if (mode == LEDS_INV) {
            iv[c] = (uint8_t)(nd[c] & ~nl[c]);
            nl[c] = inv ? 0u : iv[c];
        }
        nd[c] = mode == LEDS_DIM || mode == LEDS_DIM_LO ? (uint8_t)(nd[c] | og[c]) : nm[c];
        if (mode != LEDS_DIM && mode != LEDS_DIM_LO)
            nm[c] = inv ? iv[c] : 0u;

        nd[c] |= sg[c];
        nm[c] |= sm[c];
        nm[c] &= (uint8_t)~(nb[c] | own[c]);
        nl[c] |= own[c];
        nl[c] &= (uint8_t)~nb[c];
        nd[c] &= (uint8_t)~nb[c];
    }
    if (song.playing || (seq_counting() && cin_pos < click_beat_len() / 4u))
        nl[LED_PLAY_GREEN >> 3] |= (uint8_t)(1u << (LED_PLAY_GREEN & 7u));
    fm1_led_dim_level(mode == LEDS_DIM_LO || inv == 1u);
    for (c = 0; c < FM1_NCOL; c++) {
        fm1_led_dim[c] = nd[c];
        fm1_led_breath[c] = nb[c];
        fm1_led_mid[c] = nm[c];
    }
    for (c = 0; c < FM1_NCOL; c++)
        fm1_led[c] = nl[c];
}

#define ACC_GAP 40u
#define ACC_RATE 50u
static int32_t accel(uint32_t role, int32_t s, int32_t range)
{
    uint32_t now = fm1_ms & 0xFFFFFFu, st = ui.enc_t[role], up = s > 0, pi = st >> 25, a, i, m = 1;
    if ((!FELUCCA_SIMPLE && !(ui_prefs & PREF_ACCEL)) || range <= 32 || !s)
        return s;
    a = (uint32_t)(s < 0 ? -s : s);
    i = ((now - st) & 0xFFFFFFu) / a;
    if (!st || ((st >> 24) & 1u) != up || i >= ACC_GAP)
        i = 127u;
    else if (pi < ACC_GAP) {

        uint32_t sl = i > pi ? i : pi ? pi : 1u;
        m = sl >= 28u ? 1u : sl >= 11u ? (range > 64 ? 3u : 2u) : (range > 64 ? 8u : 4u);
    }
    ui.enc_t[role] = now | up << 24 | (i ? i : 1u) << 25;
    return s * (int32_t)m;
}

static void tracks_edit(uint32_t slot, int32_t steps)
{
    uint32_t tk = cur_page()->id[slot & 3u], id = tk & MIX_PAN ? P_PAN : P_LEVEL;
    track_t *t;
    int16_t *vp;
    const param_desc_t *d;
    tk &= ~MIX_PAN;
    if (tk >= NTRK)
        return;
    t = &trk[tk];
    vp = &t->p[id];
    d = &TP[id];
    *vp = (int16_t)clamp(*vp + accel(EN_K1 + slot, steps, d->max - d->min), d->min, d->max);
    motion_capture(t, (uint32_t)(vp - t->p), *vp);
    return;
    switch (slot) {
    case 3:
        t->p[P_MUTE] = (int16_t)(steps > 0);
        return;
    case 0:
        vp = &t->p[P_LEVEL];
        d = &TP[P_LEVEL];
        break;
    case 2:
        vp = &t->p[P_REV];
        d = &TP[P_REV];
        break;
    default:
        vp = &t->p[P_PAN];
        d = &TP[P_PAN];
        break;
    }
    *vp = (int16_t)clamp(*vp + accel(EN_K1 + slot, steps, d->max - d->min), d->min, d->max);
    motion_capture(t, (uint32_t)(vp - t->p), *vp);
}

static int roll_on(void)
{
    return !ui.home && !ui.confirm && song.seq_mode && cur_page()->graph == GR_ROLL && !drum_track(TSEL);
}

static void gbsv_done(const char *what, uint32_t n, const char *how)
{
    char nb[4];
    str_cpy(gbsv_msg[0], what, sizeof gbsv_msg[0]);
    fmt_int(nb, (int32_t)n + 1);
    str_cpy(gbsv_msg[0] + str_len(gbsv_msg[0]), nb, sizeof gbsv_msg[0] - str_len(gbsv_msg[0]));
    str_cpy(gbsv_msg[1], how, sizeof gbsv_msg[1]);
    gbsv_t = fm1_ms | 1u;
    if (gbsv == 2u)
        gbsv = 0;
}
static uint32_t gbsv_t0;
static uint8_t gbsv_did;
static void gbsv_buttons(uint32_t pressed, int ok)
{
    static const uint8_t PAGE_B[] = {B_FX, B_SCL, B_ENV, B_LFO, B_EDIT, B_GLO, B_HOME, B_SEQ, B_ARP};
    uint32_t k, down = (fm1_in.buttons >> panel.btn[B_SAVE]) & 1u;
    if (!ok) {
        if (gbsv || gbsv_t) { gbsv = 0; gbsv_t = 0; ui.force = 1; }
        return;
    }
    if (gbsv_t && (int32_t)(fm1_ms - gbsv_t) > (str_eq(gbsv_msg[1],"LOADED") ? 350 : 1200)) {
        gbsv_t = 0;
        ui.force = 1;
    }
    for (k = 0; k < NELEM(PAGE_B); k++)
        if ((gbsv || gbsv_t) && ((pressed >> panel.btn[PAGE_B[k]]) & 1u)) {
            gbsv = 0;
            gbsv_t = 0;
            ui.force = 1;
        }
    if ((pressed >> panel.btn[B_SAVE]) & 1u) {
        if (gbsv == 2u) {
            gbsv = 0;
            gbsv_t0 = 0;
        } else {
            gbsv = 1;
            gbsv_t0 = fm1_ms | 1u;
            gbsv_did = 0;
            gbsv_t = 0;
        }
        ui.force = 1;
    }
    if (gbsv == 1u && !down) {
        gbsv = !gbsv_did && fm1_ms - gbsv_t0 < 700u ? 2u : 0u;
        ui.force = 1;
    }
}
static void gbsv_keys(uint32_t pressed)
{
    uint32_t k;
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k), ld = gbsv == 2u;
        if (!((pressed >> k) & 1u))
            continue;
        if (!key_black(k) && p < 16u) {
            if (gbsv == 1u ? gbs_save_sound(p) : gbs_load_sound(p)) {
                char nm[10];
                str_cpy(nm, GB_POOL_NAME[gb_pool(trk_mach(TSEL))], sizeof nm - 1u);
                str_cpy(nm + str_len(nm), " ", sizeof nm - str_len(nm));
                gbsv_done(nm, p, gbsv == 1u ? (transport_busy() ? "SAVED AT STOP" : "SAVED") : "LOADED");
            }
        } else if (key_black(k) && p < 11u) {
            if (gbsv == 1u ? gbk_save(p) : gbk_load(p)) {
                gbsv_done("KIT ", p, gbsv == 1u ? (transport_busy() ? "SAVED AT STOP" : "SAVED") : "LOADED");
            }
        } else {
            continue;
        }
        if (ld)
            kb_eat |= 1u << k;
        gbsv_did = 1;
        ui.force = 1;
        if (!gbsv)
            break;
    }
}
static uint32_t gbsv_leds(void)
{
    uint32_t k, m = 0, mach = gb_pool(trk_mach(TSEL));
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k);
        if (key_black(k) ? gbk_used(p) : p < 16u && gbs_used(mach, p))
            m |= 1u << k;
    }
    return m;
}

static void pm_keys(uint32_t pressed)
{
    uint32_t k;
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k);
        if (!((pressed >> k) & 1u))
            continue;
        if (key_black(k)) {
            if (p < NTRK && !kb_octup)
                track_select(p);
        } else if (p < PM_NM) {
            pm_begin(p, k);
            ui.force = 1;
        } else if (p < PM_NM + 4u) {
            pm_fade = pm_fade == p - PM_NM + 1u ? 0u : (uint8_t)(p - PM_NM + 1u);
            ui.force = 1;
        }
    }
}

static void pm_poll(void)
{
    uint32_t i, was = pm_h.n;
    for (i = pm_h.n; i > 0; i--)
        if (!((fm1_in.notes >> pm_h.key[i - 1u]) & 1u))
            pm_drop(i - 1u);
    pm_update();
    xf_poll();
    if (was && !pm_h.n && !pm_page()) {
        for (i = 0; i < NPAGES; i++)
            if (PAGES[i].graph == GR_PERF) {
                fxt_set(1);
                ui.home = 0;
                ui.page = (uint8_t)i;
                page_entered();
                break;
            }
    }
    if (was != pm_h.n)
        ui.force = 1;
}
static uint32_t trk_keys(int playing)
{
    uint32_t k, m = 0;
    for (k = 0; k < 27u; k++)
        if (key_black(k) && key_place(k) < NTRK && (!playing || !trk[key_place(k)].p[P_MUTE]))
            m |= 1u << k;
    return m;
}
static uint32_t pm_leds(void)
{
    uint32_t k, m = trk_keys(1);
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k);
        if (key_black(k)) {
            if((p==7u || p==8u) && ((fm1_in.notes>>k)&1u))m|=1u<<k;
            continue;
        }
        if (p < PM_NM ? pm_mac[p].n : p < PM_NM + 4u ? pm_fade == p - PM_NM + 1u : p < 16u && ((fm1_in.notes >> k) & 1u))
            m |= 1u << k;
    }
    return m;
}

static int rec_tap(void)
{
    uint8_t bit = (uint8_t)(1u << song.sel);
    if (!ui.home && cur_page()->graph == GR_SONG && !(song.rec & bit)) {
        ui_message("[SEQ] TO RECORD");
        return 0;
    }
    song.rec ^= bit;
    ui.force = 1;
    if ((song.rec & bit) && !song.playing && !seq_counting() && roll_on()) {
        ui_message(ui_prefs2 & (PREF_CHORD_ADD >> 16) ? "KEYS ADD TO STEP" : "KEYS WRITE STEPS");

        return 1;
    }
    if ((song.rec & bit) && !song.playing && !seq_counting())
        transport_req = 1;
    if ((song.rec & bit) && grid_on())
        ui_message("LANE KEYS RECORD");
    else if ((song.rec & bit) && !ui.home && cur_page()->graph == GR_ROLL)
        ui_message("KEYS RECORD LIVE");
    return 1;
}

static int live_rec_sel(void)
{
    return ((song.rec >> song.sel) & 1u) && (song.playing || transport_req == 1u || seq_counting());
}

static void g_capture(const int16_t *vp)
{
    uint32_t id, k;
    if (!vp || vp < song.g || vp >= song.g + G_COUNT || (id = motion_gid((uint32_t)(vp - song.g))) == 0xFFu)
        return;
    for (k = 0; k < NTRK; k++)
        (void)motion_capture(&trk[k], id, *vp);
}

static uint32_t gbs_off, gbs_kept;
static void gbs_step_off(step_t *st)
{
    step_clear(st);
    (void)motion_clear_locks(TSEL, (uint32_t)(st - TSEL->step));
}
static void gbs_keep(void)
{
    uint32_t k;
    for (k = 0; k < 27u; k++)
        if (((fm1_in.notes >> k) & 1u) && !key_black(k))
            gbs_kept |= 1u << k;
}

static uint64_t gbs_held(void)
{
    uint32_t k, len = sq_len(), b;
    uint64_t m = 0;
    if (!sgrid_on())
        return 0;
    b = gbs_base();
    for (k = 0; k < 27u; k++) {
        uint32_t p = b + key_place(k);
        if (((fm1_in.notes >> k) & 1u) && !key_black(k) && p < len)
            m |= (uint64_t)1 << p;
    }
    return m;
}

static uint8_t sfn;
static uint32_t sfn_sel_t;
static void sfn_sync(void)
{
    uint64_t h = gbs_held();
    uint32_t p = 0;
    const step_t *s;
    if(!h)sfn_sel_t=0;
    if (sfn && !h) {
        sfn = 0;
        page_over = 0;
        ui.force = 1;
    }
    if (!sfn)
        return;
    while (p < NSTEP - 1u && !((h >> p) & 1u))
        p++;
    s = &TSEL->step[p];
    if (sfn == 1u) {
        sfn_val[0] = (int16_t)((s->flags & SF_ACCENT) != 0u);
        sfn_val[1] = TSEL->p[P_ED_FX];
        sfn_val[2] = TSEL->p[P_SSWING];
        sfn_val[3] = 0;
    } else {
        sfn_val[0] = (int16_t)(step_ratchet(s) - 1u);
        sfn_val[1] = (int16_t)step_stair(s);
        sfn_val[2] = (int16_t)step_ramp(s);
        sfn_val[3] = (int16_t)((s->roll & SR_LAST) != 0u);
    }
}
static void sfn_algo(int32_t s)
{
    static const char *const NAME[3] = {"LOCKS", "ACCENT", "ROLL"};
    sfn = (uint8_t)clamp((int32_t)sfn + (s > 0 ? 1 : -1), 0, 2);
    page_over = sfn ? &SFN_PAGES[sfn - 1u] : 0;
    ui_message(NAME[sfn]);
    sfn_sync();
    ui.force = 1;
}
static void sfn_turn(uint32_t k, int32_t s)
{
    uint64_t h = gbs_held();
    int32_t d = clamp(s, -3, 3);
    uint32_t p;
    track_t *t = TSEL;
    if (sfn == 1u && k == 1u) {
        if (eng_idx(t->eng_req) != ENGI_ANALOG)
            return;
        t->p[P_ED_FX] = (int16_t)clamp(t->p[P_ED_FX] + accel(EN_K1 + k, s, 127), 0, 127);
    } else if (sfn == 1u && k == 2u) {
        t->p[P_SSWING] = (int16_t)clamp(t->p[P_SSWING] + accel(EN_K1 + k, s, 100), 0, 100);
    } else {
        for (p = 0; p < NSTEP; p++) {
            step_t *st = &t->step[p];
            if (!((h >> p) & 1u))
                continue;
            if (sfn == 1u && k == 0u) {
                st->flags = (uint8_t)(d > 0 ? st->flags | SF_ACCENT : st->flags & ~SF_ACCENT);
            } else if (sfn == 2u && k == 0u) {
                uint32_t r = (uint32_t)clamp((int32_t)step_ratchet(st) - 1 + d, 0, 3);
                st->flags = (uint8_t)((st->flags & ~SF_RATCH) | r << SF_RATCH_SH);
            } else if (sfn == 2u && k == 1u) {
                step_set_roll(st, clamp(step_stair(st) + d, -3, 3), step_ramp(st), st->roll & SR_LAST);
            } else if (sfn == 2u && k == 2u) {
                step_set_roll(st, step_stair(st), (uint32_t)clamp((int32_t)step_ramp(st) + d, 0, 2), st->roll & SR_LAST);
            } else if (sfn == 2u && k == 3u) {
                step_set_roll(st, step_stair(st), step_ramp(st), d > 0);
            }
        }
    }
    gbs_keep();
    sfn_sync();
    ui.hot_col = (uint8_t)k;
    ui.hot_t = 40;
    ui.force = 1;
}

#define CLIP_LK 16u
static struct { uint8_t full, nl, lid[CLIP_LK]; int8_t lv[CLIP_LK]; step_t st; } clip;
static int clip_key(uint32_t *kp)
{
    uint32_t k, n = 0, len = (uint32_t)TSEL->p[P_SLEN];
    if (!sgrid_on() || fxl_on())
        return 0;
    for (k = 0; k < 27u; k++)
        if (((fm1_in.notes >> k) & 1u) && !key_black(k) && gbs_base() + key_place(k) < len) {
            *kp = k;
            n++;
        }
    return n == 1u;
}
static int clip_paste_ready(void)
{
    uint32_t k;
    return clip.full && clip_key(&k) && !(((gbs_off | gbs_kept) >> k) & 1u);
}
static int clip_save(void)
{
    uint32_t k, p, i, place;
    track_t *t = TSEL;
    if (!clip_key(&k))
        return 0;
    p = gbs_base() + key_place(k);
    place = trk_index(t) << 6 | p;
    if ((gbs_off >> k) & 1u) {
        clip.st = t->step[p];
        clip.nl = 0;
        for (i = 0; i < motion.count && clip.nl < CLIP_LK; i++)
            if (motion.event[i].place == place && (motion.event[i].param & MOTION_LOCK)) {
                clip.lid[clip.nl] = (uint8_t)(motion.event[i].param & ~MOTION_LOCK);
                clip.lv[clip.nl++] = motion.event[i].value;
            }
        clip.full = 1;
        gbs_keep();
        ui_message("TRIG COPIED");
    } else if (clip.full) {
        (void)motion_clear_locks(t, p);
        t->step[p] = clip.st;
        for (i = 0; i < clip.nl; i++)
            (void)motion_set_lock(t, p, clip.lid[i], clip.lv[i]);
        gbs_keep();
        ui_message("TRIG PASTED");
    } else {
        ui_message("NOTHING COPIED");
    }
    ui.force = 1;
    return 1;
}
static void sgrid_release(void)
{
    uint32_t k, up = (gbs_off | gbs_kept) & ~fm1_in.notes;
    for (k = 0; k < 27u; k++)
        if ((up >> k) & 1u && ((gbs_off & ~gbs_kept) >> k) & 1u) {
            if (fxl_on()) {
                fxl.trig &= ~((uint64_t)1 << (gbs_base() + key_place(k)));
                fxl.lk[gbs_base() + key_place(k)] = 0;
                fxl_refresh();
            } else {
                gbs_step_off(&TSEL->step[gbs_base() + key_place(k)]);
            }
            ui.force = 1;
        }
    gbs_off &= ~up;
    gbs_kept &= ~up;
}

static void par_poll(void)
{
    uint32_t id = par_on() ? lock_pid(&PAGES[par_pg % NPAGES], par_k) : 0xFFu;
    kb_seqdn = (uint8_t)(par_on() && ((fm1_in.buttons >> panel.btn[B_SEQ]) & 1u));
    if (id == P_E7)
        id = 0xFFu;
    if (id != 0xFFu) {
        const param_desc_t *d = id < P_COUNT ? track_desc(TSEL, id) : motion_desc(TSEL, id);
        int16_t lo = (int16_t)clamp(d->min, -64, 127), hi = (int16_t)clamp(d->max, -64, 127);
        if (par_id != id || par_lo != lo || par_hi != hi) {
            par_id = 0xFFu;
            par_lo = lo;
            par_hi = hi;
        }
    }
    par_id = (uint8_t)id;
}

static int seq_dn(void)
{
    return FELUCCA_SIMPLE && !ui.confirm && !gbsv_keys_on() && !pm_page() && !pm_down() &&
           (seq_pick || ((fm1_in.buttons >> panel.btn[B_SEQ]) & 1u));
}
static void seq_white(uint32_t notes)
{
    uint32_t k;
    if (!seq_dn())
        return;
    for (k = 0; k < 27u; k++)
        if (((notes >> k) & 1u) && !key_black(k)) {
            layer_kill();
            seq_pick = 0;
            if (ui.sgrid != 3u) {
                ui.sgrid = 3;
                ui.force = 1;
            }
            return;
        }
}
static int seq_knob(uint32_t k)
{
    uint32_t id;
    if (!seq_dn())
        return 0;
    layer_kill();
    id = !ui.home && lock_page_ok(ui.page) ? lock_pid(cur_page(), k) : 0xFFu;
    if (id == 0xFFu || id == P_E7)
        return 1;
    par_k = (uint8_t)k;
    par_pg = ui.page;
    seq_pick = 0;
    ui.sgrid = 2;
    ui.force = 1;
    return 1;
}

static void par_keys(uint32_t pressed)
{
    uint32_t k;
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k);
        if (!((pressed >> k) & 1u) || !key_black(k))
            continue;
        if (kb_seqdn) {
            layer_kill();
            if (p < 4u)
                par_k = (uint8_t)p;
            ui.force = 1;
        } else if (p < NTRK && !kb_octup) {
            track_select(p);
        }
    }
}

static uint32_t par_leds(void)
{
    uint32_t k, m = 0, pos = 0xFFu;
    if (par_id != 0xFFu && par_hi > par_lo) {
        int32_t v = *motion_vp(TSEL, par_id);
        pos = (uint32_t)clamp(((v - par_lo) * 15 + (par_hi - par_lo) / 2) / (par_hi - par_lo), 0, 15);
    }
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k);
        if (key_black(k) ? p == (kb_seqdn ? par_k : song.sel) : p == pos)
            m |= 1u << k;
    }
    return m;
}

static struct { uint32_t t0; uint8_t k; } gpk;
static int gbs_page_held(void)
{
    uint32_t k;
    if (!sgrid_on())
        return -1;
    for (k = 0; k < 27u; k++)
        if (((fm1_in.notes >> k) & 1u) && key_black(k) && key_place(k) >= GBS_PG)
            return (int)(key_place(k) - GBS_PG);
    return -1;
}
static void gbs_page_poll(void)
{
    track_t *t = TSEL;
    int16_t *lp = sq_lenp();
    if (!gpk.t0)
        return;
    if (!((fm1_in.notes >> gpk.k) & 1u) || !sgrid_on()) {
        gpk.t0 = 0;
        return;
    }
    if ((int32_t)(fm1_ms - gpk.t0) >= (int32_t)HINT_MS) {
        int16_t was = *lp;
        char m[8] = "LEN ";
        gpk.t0 = 0;
        *lp = (int16_t)((key_place(gpk.k) - GBS_PG + 1u) * 16u);
        pm_record(t, lp, was);
        fmt_int(m + 4, *lp);
        ui_message(m);
        ui.force = 1;
    }
}
static void gbs_page_del(uint32_t pg)
{
    track_t *t = TSEL;
    uint32_t i;
    int16_t *lp = sq_lenp(), was = *lp;
    gpk.t0 = 0;
    if (!pg) {
        ui_message("PAGE 1 STAYS");
        return;
    }
    for (i = pg * 16u; i < NSTEP; i++) {
        if (fxl_on()) {
            fxl.trig &= ~((uint64_t)1 << i);
            fxl.lk[i] = 0;
        } else {
            gbs_step_off(&t->step[i]);
        }
    }
    if (fxl_on())
        fxl_refresh();
    gbs_off = gbs_kept = 0;
    *lp = (int16_t)(pg * 16u);
    pm_record(t, lp, was);
    ui.bank = (uint8_t)(pg - 1u);
    ui_message(pg == 1u ? "PAGE 2 DELETED" : pg == 2u ? "PAGE 3 DELETED" : "PAGE 4 DELETED");
    ui.force = 1;
}
static void sgrid_keys(uint32_t pressed)
{
    uint32_t k, len = sq_len();
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k);
        if (!((pressed >> k) & 1u))
            continue;
        if (key_black(k)) {
            if (p >= GBS_PG && !kb_octup) {
                uint32_t pg = p - GBS_PG;
                if (pg * 16u >= len) {
                    int16_t *lp = sq_lenp(), was = *lp;
                    *lp = (int16_t)((pg + 1u) * 16u);
                    pm_record(TSEL, lp, was);
                    len = sq_len();
                    ui_message(pg == 1u ? "PAGE 2 ADDED" : pg == 2u ? "PAGE 3 ADDED" : "PAGE 4 ADDED");
                }
                if (pg != ui.bank) {
                    gbs_off = gbs_kept = 0;
                    ui.bank = (uint8_t)pg;
                }
                gpk.k = (uint8_t)k;
                gpk.t0 = fm1_ms | 1u;
                ui.force = 1;
                continue;
            }
            if (p <= NTRK && !kb_octup) {
                gbs_off = gbs_kept = 0;
                fxt_set(p == NTRK);
                if (p < NTRK)
                    track_select(p);
                ui.force = 1;
            }
            continue;
        }
        if (fxl_on()) {
            p += gbs_base();
            if (p < len) {
                if ((fxl.trig >> p) & 1u) {
                    gbs_off |= 1u << k;
                } else {
                    fxl.lk[p] = 0;
                    fxl.trig |= (uint64_t)1 << p;
                }
                ui.force = 1;
            }
            continue;
        }
        p += gbs_base();
        if (p >= len)
            continue;
        {
            step_t *st = &TSEL->step[p];
            if ((st->time == ST_NOTE && st->n) || gbs_trigless(TSEL, p)) {
                gbs_off |= 1u << k;
            } else if (kb_octup) {
                gbs_step_off(st);
                st->time = ST_LOCK;
                mt_used = 1;
                ui.force = 1;
            } else {
                gbs_step_off(st);
                st->time = ST_NOTE;
                st->n = (uint8_t)grid_chord(TSEL, st->note);
                st->vel = 100;
                if (seq_fill)
                    st->acc = (uint8_t)(GB_TC0 + GB_TC_FILL);
            }
        }
        cursor_set((int32_t)p);
    }
}

static void gbseq_knob(uint32_t slot, int32_t steps)
{
    track_t *t = TSEL;
    uint64_t held = gbs_held();
    uint32_t p;
    int16_t was;
    if (fxl_sel) {
        if (slot == 0u)
            fxl.len = (int16_t)clamp((int32_t)fxl_len() + (steps > 0 ? 1 : -1), 1, (int32_t)FXN);
        ui.force = 1;
        return;
    }
    if (slot >= 2u && held) {
        gbs_keep();
        for (p = 0; p < NSTEP; p++) {
            step_t *s = &t->step[p];
            if (!((held >> p) & 1u))
                continue;
            if (slot == 2u) {
                int32_t c = clamp(gb_own(s) + accel(EN_K1 + slot, steps, 101), -1, 100);
                s->probability = (uint8_t)(c < 0 ? 0 : c == 0 ? 101 : c);
            } else {
                int32_t g = clamp((int32_t)gb_trig(s) + accel(EN_K1 + slot, steps, (int32_t)GB_NTC), 0, (int32_t)GB_NTC);
                s->acc = (uint8_t)(g ? GB_TC0 + (uint32_t)g : 0u);
            }
        }
        ui.force = 1;
        return;
    }
    if (slot == 3u) {
        ui_message("HOLD A STEP");
        return;
    }
    if (slot == 0u) {
        was = t->p[P_SLEN];
        t->p[P_SLEN] = (int16_t)clamp(was + (steps > 0 ? 1 : -1), 1, NSTEP);
        pm_record(t, &t->p[P_SLEN], was);
    } else if (slot == 1u) {
        was = t->p[P_SDIV];
        t->p[P_SDIV] = GB_SPEED[clamp((int32_t)gb_speed(was) + (steps > 0 ? 1 : -1), 0, 4)];
        pm_record(t, &t->p[P_SDIV], was);
    } else {
        was = t->p[P_APROB];
        t->p[P_APROB] = (int16_t)clamp((was > 100 ? 100 : was) + accel(EN_K1 + slot, steps, 100), 0, 100);
        pm_record(t, &t->p[P_APROB], was);
    }
    ui.force = 1;
}

static uint64_t lock_held(void)
{
    const track_t *t = TSEL;
    uint32_t k, len = (uint32_t)t->p[P_SLEN], keys = fm1_in.notes & ~kb_layer, b;
    uint64_t m = 0;
    if (sgrid_on()) {
        if (ui.home || !keys || !lock_page_ok(ui.page))
            return 0;
        ui.plk_src = (uint8_t)(ui.page + 1u);
        if (fxl_on())
            len = fxl_len();
        b = gbs_base();
        for (k = 0; k < 27u; k++)
            if (((keys >> k) & 1u) && !key_black(k) && b + key_place(k) < len)
                m |= (uint64_t)1 << (b + key_place(k));
        return m;
    }
    if (ui.home || ui.confirm || !song.seq_mode || cur_page()->graph != GR_ROLL ||
        !keys || detail_on())
        return 0;
    if (!grid_on())
        return ui.entry_open && !live_rec_sel() && ui.cursor < len ? (uint64_t)1 << ui.cursor : 0u;
    for (k = 0; k < 27u; k++)
        if (((keys >> k) & 1u) && !key_black(k) && ui.bank * 16u + key_place(k) < len)
            m |= (uint64_t)1 << (ui.bank * 16u + key_place(k));
    return m;
}

static void fxl_turn(uint32_t k, int32_t s, uint64_t held)
{
    uint32_t id = lock_id(k), j, i;
    const param_desc_t *d;
    if (id == 0xFFu || id < P_COUNT) {
        ui_message(id == 0xFFu && cur_page()->fxt ? "AMT LOCKS ONLY" : "FX: MASTER ONLY");
        return;
    }
    j = id - P_COUNT;
    d = &GP[MOTION_G[j]];
    s = accel(EN_K1 + k, s, d->max - d->min);
    for (i = 0; i < FXN; i++) {
        int32_t v;
        if (!((held >> i) & 1u))
            continue;
        v = (fxl.lk[i] >> j) & 1u ? fxl.v[i][j] : song.g[MOTION_G[j]];
        fxl.v[i][j] = (int8_t)param_turn(d, v, s);
        fxl.lk[i] |= (uint16_t)(1u << j);
        fxl.trig |= (uint64_t)1 << i;
    }
}
static void lock_turn(uint32_t k, int32_t s, uint64_t held)
{
    track_t *t = TSEL;
    uint32_t id = lock_id(k), i;
    const param_desc_t *d;
    if (fxl_on()) {
        fxl_turn(k, s, held);
        return;
    }
    if (id == 0xFFu)
        return;
    ui.plk_rm &= ~held;
    d = id < P_COUNT ? track_desc(t, id) : motion_desc(t, id);
    s = accel(EN_K1 + k, s, d->fmt == F_ENUM ? 0 : d->max - d->min);
    motion_undo_take(t, 0x100u | k);
    for (i = 0; i < NSTEP; i++) {
        int16_t v;
        if (!((held >> i) & 1u))
            continue;
        if (!motion_lock_get(t, i, id, &v))
            v = motion_base_value(t, id);
        if (motion_set_lock(t, i, id, (int16_t)param_turn(d, v, s)) == 2)
            break;
    }
    motion_undo_done(t);
}

static uint32_t kit_held(void)
{
    uint32_t k, m = 0, keys = fm1_in.notes & ~kb_layer;
    if (!kit_on())
        return 0;
    for (k = 0; k < 27u; k++)
        if (((keys >> k) & 1u) && !key_black(k) && key_place(k) < KIT_KEYS)
            m |= 1u << key_place(k);
    return m;
}

static int kit_turn(uint32_t k, int32_t s, uint32_t held)
{
    track_t *t = TSEL;
    uint32_t tk = song.sel, id = lock_page_ok(ui.page) ? lock_pid(cur_page(), k) : 0xFFu, p, i;
    const param_desc_t *d;
    if (id == 0xFFu)
        return 0;
    d = id < P_COUNT ? track_desc(t, id) : motion_desc(t, id);
    s = accel(EN_K1 + k, s, d->fmt == F_ENUM ? 0 : d->max - d->min);
    for (p = 0; p < 16u; p++) {
        kit_key_t *kk = &kit[tk][p];
        if (!((held >> p) & 1u))
            continue;
        for (i = 0; i < kk->n && kk->id[i] != id; i++)
            ;
        if (i == kk->n) {
            if (kk->n == KIT_N) {
                ui_message("KEY LOCKS FULL");
                continue;
            }
            kk->id[kk->n] = (uint8_t)id;
            kk->v[kk->n++] = kit_cur[tk] == p + 1u ? *motion_vp(t, id) : motion_base_value(t, id);
        }
        kk->v[i] = (int16_t)param_turn(d, kk->v[i], s);
        if (kit_cur[tk] == p + 1u)
            kit_apply(t, p);
    }
    return 1;
}

static int kit_shown(int16_t *vp, int16_t *cur)
{
    uint32_t id, held = kit_held(), p = 0, i;
    if (vp >= TSEL->p && vp < TSEL->p + P_COUNT)
        id = (uint32_t)(vp - TSEL->p);
    else if (vp >= song.g && vp < song.g + G_COUNT && motion_gid((uint32_t)(vp - song.g)) != 0xFFu)
        id = motion_gid((uint32_t)(vp - song.g));
    else
        return 0;
    if (held) {
        const kit_key_t *kk;
        while (!((held >> p) & 1u))
            p++;
        kk = &kit[song.sel][p];
        for (i = 0; i < kk->n; i++)
            if (kk->id[i] == id) {
                *cur = kk->v[i];
                return 1;
            }
    }
    (void)kit_base(TSEL, id, cur);
    return 0;
}

static void kit_keys(uint32_t pressed)
{
    uint32_t k;
    for (k = 0; k < 27u; k++)
        if (((pressed >> k) & 1u) && key_black(k) && key_place(k) < NTRK && !kb_octup)
            track_select(key_place(k));
}

static uint32_t kit_leds(void)
{
    uint32_t k, m = 0, p;
    for (k = 0; k < 27u; k++) {
        p = key_place(k);
        if (key_black(k) ? p == song.sel : p < KIT_KEYS && kit[song.sel][p].n)
            m |= 1u << k;
    }
    return m;
}

static uint32_t acid_t0;
static void acid_keys(uint32_t pressed)
{
    uint32_t k;
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k);
        if (!((pressed >> k) & 1u))
            continue;
        if (!key_black(k)) {
            if (p < ACID_N) {
                acid_sel[song.sel % NTRK] = (uint8_t)p;
                acid_t0 = fm1_ms | 1u;
                if (ui.home || cur_page()->graph != GR_ACID || acid_pg(cur_page()) != 0u)
                    acid_page(0);
                ui.force = 1;
            }
        } else if (p >= GBS_PG && p < GBS_PG + 2u) {
            acid_page(p - GBS_PG);
        } else if (p < NTRK && !kb_octup) {
            track_select(p);
        }
    }
}

static uint32_t acid_held(void)
{
    uint32_t k, m = 0;
    if (!acid_kon())
        return 0;
    for (k = 0; k < 27u; k++)
        if (((fm1_in.notes >> k) & 1u) && !key_black(k) && key_place(k) < ACID_N)
            m |= 1u << key_place(k);
    return m;
}

static uint32_t acid_leds(uint32_t *br)
{
    const acid_t *a = acid_of(song.sel);
    uint32_t k, m = 0, tk = song.sel % NTRK, ph = song.playing && a->play && acid_rt[tk].go == 2u ? acid_rt[tk].st : 0xFFu;
    uint32_t pg = !ui.home && cur_page()->graph == GR_ACID ? acid_pg(cur_page()) : 0xFFu;
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k), on;
        if (!key_black(k)) {
            on = p < a->stages && a->st[p].gate != AG_MUTE;
            on ^= (uint32_t)(p == ph);
            if (br && p == acid_sel[tk] && p != ph) {
                *br |= 1u << k;
                on = 0;
            }
        } else if (p >= GBS_PG && p < GBS_PG + 2u) {
            on = p - GBS_PG == pg;
        } else {
            on = p == song.sel;
        }
        m |= on << k;
    }
    return m;
}

static int acid_pop(void)
{
    if (!acid_held())
        acid_t0 = 0;
    return acid_t0 && (int32_t)(fm1_ms - acid_t0) >= (int32_t)HINT_MS;
}

static void acid_knob(uint32_t slot, int32_t steps)
{
    track_t *t = TSEL;
    acid_t *a = acid_of(song.sel);
    acid_st_t *s = &a->st[acid_sel[song.sel % NTRK] % ACID_N];
    int32_t d = steps > 0 ? 1 : -1;
    if (acid_pop()) {
        if (slot == 0u)
            s->flags = (uint8_t)(d > 0 ? s->flags | AF_SLIDE : s->flags & ~AF_SLIDE);
        else if (slot == 1u)
            s->stime = (uint8_t)clamp((int32_t)(s->stime ? s->stime : 40u) + accel(EN_K1 + slot, steps, 126), 1, 127);
        else
            return;
    } else switch (acid_pg(cur_page()) * 4u + slot) {
    case 0: s->pitch = (uint8_t)clamp((int32_t)s->pitch + d, 0, 12); break;
    case 1: s->pulses = (uint8_t)clamp((int32_t)s->pulses + d, 1, 8); break;
    case 2: s->gate = (uint8_t)clamp((int32_t)s->gate + d, 0, AG_N - 1); break;
    case 3: s->oct = (uint8_t)clamp((int32_t)s->oct + d, 0, 6); break;
    case 4: a->stages = (uint8_t)clamp((int32_t)a->stages + d, 1, ACID_N); break;
    case 5: a->dir = (uint8_t)clamp((int32_t)a->dir + d, 0, AD_N - 1); break;
    case 6: {
        int16_t was = t->p[P_SDIV];
        t->p[P_SDIV] = GB_SPEED[clamp((int32_t)gb_speed(was) + d, 0, 4)];
        pm_record(t, &t->p[P_SDIV], was);
        break;
    }
    case 7: a->gate = (uint8_t)clamp((int32_t)a->gate + accel(EN_K1 + slot, steps, 126), 1, 127); break;
    default: return;
    }
    ui.hot_col = (uint8_t)slot;
    ui.hot_t = 40;
    ui.force = 1;
}

static uint8_t trk_sg[NTRK];
static int8_t trk_oct[NTRK];
static void trk_mode_poll(void)
{
    static uint8_t last = 0xFFu;
    uint8_t m;
    if (fxl_sel || last == song.sel)
        return;
    if (last < NTRK) {
        trk_sg[last] = (ui.sgrid >= 2u && ui.sgrid <= 4u) ? (uint8_t)(ui.sgrid + 1u) : 1u;
        trk_oct[last] = (int8_t)song.octave;
        song.octave = trk_oct[song.sel % NTRK];
    }
    m = trk_sg[song.sel % NTRK];
    if (m >= 3u && m <= 5u) {
        if (ui.sgrid != m - 1u) { ui.sgrid = (uint8_t)(m - 1u); ui.force = 1; }
    } else if (m == 1u && (ui.sgrid == 2u || ui.sgrid == 3u)) {
        ui.sgrid = 1u;
        ui.force = 1;
    }
    last = song.sel;
}

static void acid_follow(void)
{
    uint32_t want;
    if (fxl_sel || (ui.sgrid != 1u && ui.sgrid != 4u))
        return;
    want = acid_on(TSEL) ? 4u : 1u;
    if (ui.sgrid != want) {
        ui.sgrid = (uint8_t)want;
        ui.force = 1;
    }
    if (!ui.home && cur_page()->fam == FAM_SEQ && !page_visible(ui.page)) {
        if (want == 4u)
            acid_page(1);
        else
            open_family(FAM_SEQ);
    }
}

static void lock_clear(uint64_t held)
{
    uint32_t i, n = 0;
    if (fxl_on()) {
        for (i = 0; i < FXN; i++)
            if ((held >> i) & 1u) {
                n += fxl.lk[i] != 0u;
                fxl.lk[i] = 0;
            }
        fxl_refresh();
        ui_message(n ? "LOCKS CLEARED" : "NO LOCKS");
        return;
    }
    ui.plk_rm &= ~held;
    if (!(motion_lock_steps(song.sel) & held)) {
        ui_message("NO LOCKS");
        return;
    }
    motion_undo_take(TSEL, 0);
    for (i = 0; i < NSTEP; i++)
        if ((held >> i) & 1u)
            n += motion_clear_locks(TSEL, i);
    motion_undo_done(TSEL);
    ui_message(n ? "LOCKS CLEARED" : "NO LOCKS");
}

static void msrc_fix(void)
{
    uint32_t i;
    for (i = 0; FELUCCA_SIMPLE && i < NTRK; i++) {
        if (trk[i].p[P_M1SRC] != MS_LFO)
            trk[i].p[P_M1SRC] = MS_LFO;
        if (trk[i].p[P_M2SRC] != MS_ENV)
            trk[i].p[P_M2SRC] = MS_ENV;
    }
}

static int page_btn_reset(void)
{
    static const uint8_t RB[7] = {B_FX, B_SCL, B_ENV, B_LFO, B_EDIT, B_GLO, B_ARP};
    uint32_t i, f, b = 0xFFu;
    char m[24];
    for (i = 0; i < 7u && b == 0xFFu; i++)
        if ((fm1_in.buttons >> panel.btn[RB[i]]) & 1u)
            b = RB[i];
    if (b == 0xFFu || tm_dn)
        return 0;
    for (f = 0; f < FAM_COUNT && !(FAM_BTN[f] == b && page_first(f) < NPAGES && PAGES[page_first(f)].fam == f); f++)
        ;
    if (f == FAM_COUNT)
        return 0;
    if (ui.home || cur_page()->fam != f)
        open_family(f);
    ui.pg_down &= (uint16_t)~(1u << panel.btn[b]);
    layer_kill();
    for (i = 0; i < 3u; i++)
        if (TM_BTN[i] == b)
            tm_eat |= (uint8_t)(1u << i);
    str_cpy(m, cur_page()->title, sizeof m);
    str_cpy(m + str_len(m), page_reset() ? " RESET" : ": NOTHING", sizeof m - str_len(m));
    ui_message(m);
    return 1;
}

static int32_t smp_len_fit(const track_t *t)
{
    uint32_t sl = smp_slot(t->p[P_E5]), n, s;
    if (eng_idx(t->eng_req) != ENGI_SAMPLER || sl >= smp_n || smp_tab[sl].len < 2u)
        return -1;
    n = smp_tab[sl].len;
    s = (uint32_t)(t->p[P_E2] & 127) * (n - 1u) / 127u;
    return (int32_t)((n - s - 1u) * 127u / (n - 1u));
}
#include "trap_rand.c"
#include "trap_chord_gen.c"
#include "trap_phonk.c"
#include "trap_drum_gen.c"
#include "trap_hat_gen.c"
#include "trap_v0910.c"

#if FELUCCA_CPU2

static void cpu2_knob(int32_t steps)
{
    uint8_t on = steps > 0, set = (ui_prefs2 & (PREF_CPU2_ON >> 16)) != 0u;
    if (!steps)
        return;
    if (on != set) {
        ui_prefs2 = (uint8_t)(on ? ui_prefs2 | (PREF_CPU2_ON >> 16) : ui_prefs2 & ~(PREF_CPU2_ON >> 16));
        settings_save();
    }
    if (on)
        ui_message(fm1_c1_on ? "2ND CORE ON" : "2ND CORE ON: RESTART");
    else
        ui_message(fm1_c1_on ? "2ND CORE OFF: RESTART" : "2ND CORE OFF");
    ui.force = 1;
}
#endif

#define GEN_N 4u
static int gen_select(int32_t s)
{
    static const char *const GN[GEN_N] = {"LUCKY DRUMS", "ACE HATS", "LOW ROLLER", "GOLDEN CHORDS"};
    uint32_t i, k, at = GEN_N, n;
    if (ui.home || page_over || ui.confirm)
        return 0;
    for (k = 0; k < GEN_N; k++)
        if (str_eq(cur_page()->title, GN[k]))
            at = k;
    if (at >= GEN_N)
        return 0;
    for (n = 0; n < GEN_N; n++) {
        at = (at + (s > 0 ? 1u : GEN_N - 1u)) % GEN_N;
        for (i = 0; i < NPAGES; i++)
            if (PAGES[i].fam == FAM_ARP && str_eq(PAGES[i].title, GN[at]) && page_visible(i)) {
                ui.page = (uint8_t)i;
                ui.fam_last[FAM_ARP] = (uint8_t)i;
                page_entered();
                ui.force = 1;
                return 1;
            }
    }
    return 1;
}

static uint32_t pr_s0(void)
{
    const track_t *t = TSEL;
    uint32_t len = t->p[P_SLEN] ? (uint32_t)t->p[P_SLEN] : 1u, idx = t->seq_idx % len;
    if (!ui.home && str_eq(cur_page()->title, "QUICK FILL"))
        return (song.playing ? idx : len - 1u) / 16u * 16u;
    return (ov_bar ? (uint32_t)ov_bar - 1u : song.playing ? idx / 16u : 0u) * 16u;
}
static uint32_t pr_end(const track_t *t, uint32_t s, uint32_t len)
{
    uint32_t e = s + 1u;
    while (e < len && t->step[e].time == ST_TIE)
        e++;
    return e;
}
static int gl_btn(uint32_t b);
static void pr_reverse(track_t*t,uint32_t a,uint32_t b) {
    while(a<b){step_t tmp=t->step[a];t->step[a++]=t->step[b];t->step[b--]=tmp;}
}
static void pr_select_at(uint32_t step,uint8_t mask) {
    ov_bar=(uint8_t)(step/16u+1u);pr_sel=(uint8_t)(step%16u+1u);pr_selm=mask;
}

static void pr_mode(int32_t d)
{
    pr_single = (uint8_t)(d > 0);
    ou_eat = 1;
    if (pr_sel)
        pr_selm = pr_single ? 1u : 15u;
    ui.force = 1;
}
static void pr_pick(int32_t d)
{
    const track_t *t = TSEL;
    uint32_t s0 = pr_s0(), len = t->p[P_SLEN] ? (uint32_t)t->p[P_SLEN] : 1u, i, q, n = 0, at = 0, ls[80], lm[80], mode = pr_single;
    for (i = 0; i < 16u && s0 + i < len && s0 + i < NSTEP; i++) {
        const step_t *s = &t->step[s0 + i];
        if (s->time != ST_NOTE || !s->n)
            continue;
        if (mode != 1u) {
            ls[n] = i + 1u;
            lm[n++] = 0x0Fu;
        }
        for (q = 0; mode != 0u && q < s->n && q < 4u; q++) {
            ls[n] = i + 1u;
            lm[n++] = (uint8_t)(1u << q);
        }
    }
    if (!n) {
        pr_sel = pr_selm = 0;
        ui_message("NO NOTES");
        return;
    }
    for (i = 0; i < n; i++)
        if (ls[i] == pr_sel && lm[i] == pr_selm)
            at = i + 1u;
    at = at ? (at - 1u + (d > 0 ? 1u : n - 1u)) % n : d > 0 ? 0u : n - 1u;
    pr_sel = (uint8_t)ls[at];
    pr_selm = (uint8_t)lm[at];
}
static int pr_valid(void)
{
    const track_t *t = TSEL;
    uint32_t len = t->p[P_SLEN] ? (uint32_t)t->p[P_SLEN] : 1u, s = pr_s0() + pr_sel - 1u;
    if (!pr_sel || pr_sel > 16u || s >= len || s >= NSTEP || t->step[s].time != ST_NOTE || !t->step[s].n)
        return 0;
    if (pr_selm != 0x0Fu && !(pr_selm & ((1u << t->step[s].n) - 1u)))
        return 0;
    return 1;
}
static void pr_edit(uint32_t slot, int32_t steps)
{
    track_t *t = TSEL;
    uint32_t len = t->p[P_SLEN] ? (uint32_t)t->p[P_SLEN] : 1u, s0 = pr_s0(), s, e, q, all;
    int32_t d = steps > 0 ? 1 : -1;
    if (fxl_sel || acid_on(t)) {
        ui_message("NOT ON THIS TRACK");
        return;
    }
    if (slot == 0u || !pr_valid()) {
        pr_pick(slot == 0u ? d : 1);
        if (slot == 0u)
            return;
        if (!pr_valid())
            return;
    }
    s = s0 + pr_sel - 1u;
    all = pr_selm == 0x0Fu || t->step[s].n == 1u;
    fm1_irq_off();
    if (slot == 2u) {
        for (q = 0; q < t->step[s].n && q < 4u; q++)
            if (all || ((pr_selm >> q) & 1u))
                t->step[s].note[q] = (uint8_t)clamp((int32_t)t->step[s].note[q] + d, 0, 127);
    } else if (slot == 3u) {
        e = pr_end(t, s, len);
        if (d > 0 && e < len && e < NSTEP && e < s0 + 16u && t->step[e].time != ST_NOTE) {
            step_clear(&t->step[e]);
            t->step[e].time = ST_TIE;
        } else if (d < 0 && e > s + 1u)
            step_clear(&t->step[e - 1u]);
    } else if (slot == 1u) {
        int32_t to=(int32_t)s+d;
        e=pr_end(t,s,len);
        uint32_t first = str_eq(cur_page()->title,"QUICK FILL") ? s0 : 0u;
        uint32_t limit = str_eq(cur_page()->title,"QUICK FILL") && s0+16u<len ? s0+16u : len;
        if(to>=(int32_t)first && to<(int32_t)limit && to<(int32_t)NSTEP) {
            if(all) {
                if(d>0 && e<limit) {
                    uint32_t after=t->step[e].time==ST_NOTE?pr_end(t,e,limit):e+1u;
                    uint32_t offset=after-e;
                    pr_reverse(t,s,e-1u);pr_reverse(t,e,after-1u);pr_reverse(t,s,after-1u);
                    pr_select_at(s+offset,15u);
                } else if(d<0) {
                    uint32_t before=s-1u;
                    while(before>first && t->step[before].time==ST_TIE)before--;
                    pr_reverse(t,before,s-1u);pr_reverse(t,s,e-1u);pr_reverse(t,before,e-1u);
                    pr_select_at(before,15u);
                }
            } else {
                step_t*dst=&t->step[to];uint32_t qq=0;
                while(qq<4u && !((pr_selm>>qq)&1u))qq++;
                if(qq<t->step[s].n) {
                    if(dst->time==ST_NOTE && dst->n>=4u) {

                        uint8_t keep=dst->note[0];dst->note[0]=t->step[s].note[qq];t->step[s].note[qq]=keep;
                        pr_select_at((uint32_t)to,1u);
                    } else {
                        uint8_t note=t->step[s].note[qq],vel=t->step[s].vel;
                        if(dst->time!=ST_NOTE||!dst->n){step_clear(dst);dst->time=ST_NOTE;dst->vel=vel;}
                        dst->note[dst->n++]=note;
                        for(q=qq;q+1u<t->step[s].n;q++)t->step[s].note[q]=t->step[s].note[q+1u];
                        if(!--t->step[s].n)step_clear(&t->step[s]);
                        pr_select_at((uint32_t)to,(uint8_t)(1u<<(dst->n-1u)));
                    }
                }
            }
        }
    }
    fm1_irq_on();
    gen_recipe[trk_index(t)].active = 0;
}

static uint32_t bd_t;
static void bd_poll(void)
{
    const track_t *ts = TSEL;
    uint32_t src, dst, k, n = 0, sl = ts->p[P_SLEN] > 0 ? (uint32_t)ts->p[P_SLEN] : 1u;
    char m[12];
    if (!bd_t || fm1_ms - bd_t < 600u)
        return;
    bd_t = 0;
    if (!bar_dup)
        return;
    src = seq_bar ? (uint32_t)seq_bar - 1u : ov_bar ? (uint32_t)ov_bar - 1u : song.playing ? ts->seq_idx % sl / 16u : 0u;
    dst = (uint32_t)bar_dup - 1u;
    if (src == dst) {
        ui_message("SAME BAR");
        return;
    }
    fm1_irq_off();
    for (k = 0; k < NTRK; k++) {
        track_t *t = &trk[k];
        uint32_t len = t->p[P_SLEN] > 0 ? (uint32_t)t->p[P_SLEN] : 1u;
        if (src * 16u >= len || (dst + 1u) * 16u > NSTEP)
            continue;
        memcpy(&t->step[dst * 16u], &t->step[src * 16u], 16u * sizeof(step_t));
        if (len < (dst + 1u) * 16u)
            t->p[P_SLEN] = (int16_t)((dst + 1u) * 16u);
        gen_recipe[k].active = 0;
        n++;
    }
    fm1_irq_on();
    if (!n) {
        ui_message("NOTHING TO COPY");
        return;
    }
    str_cpy(m, "BAR 1 > 2", sizeof m);
    m[4] = (char)('1' + src);
    m[8] = (char)('1' + dst);
    ui_message(m);
    ui.force = 1;
}

static int qf_select(int32_t s)
{
    if (ui.home || page_over || !str_eq(cur_page()->title, "QUICK FILL"))
        return 0;
    ph_fill = (int16_t)param_turn(&PH_D[PH_FILL], ph_fill, s);
    ui.force = 1;
    return 1;
}

static int mix_select(int32_t s)
{
    uint32_t i, at = NPAGES, n = 0, idx[3];
    if (ui.home || page_over || cur_page()->scope != SC_TRK || cur_page()->graph != GR_TRK)
        return 0;
    for (i = 0; i < NPAGES && n < 3u; i++)
        if (PAGES[i].scope == SC_TRK && PAGES[i].graph == GR_TRK) {
            if (i == ui.page)
                at = n;
            idx[n++] = i;
        }
    if (at >= n)
        return 0;
    at = (uint32_t)clamp((int32_t)at + (s > 0 ? 1 : -1), 0, (int32_t)n - 1);
    if (idx[at] != ui.page) {
        ui.page = (uint8_t)idx[at];
        ui.fam_last[FAM_TRK] = (uint8_t)idx[at];
        page_entered();
        ui.force = 1;
    }
    return 1;
}

static int pr_select(int32_t s)
{
    if (ui.home || page_over || ui.confirm || !str_eq(cur_page()->title, "PIANO ROLL"))
        return 0;
    ov_bar = (int16_t)param_turn(&OV_D, ov_bar, s);
    pr_sel = pr_selm = 0;
    ui.force = 1;
    return 1;
}
static int pm_menu(int32_t s)
{
    static const uint8_t MB[9] = {B_FX, B_SCL, B_ENV, B_LFO, B_EDIT, B_GLO, B_HOME, B_SEQ, B_ARP};
    uint32_t i, b = 0xFFu, f = FAM_COUNT, j, n, cur;
    if (!FELUCCA_SIMPLE || gbs_held() || ui.confirm || ui.uboot || gbsv_keys_on())
        return 0;
    for (i = 0; i < 9u; i++)
        if ((fm1_in.buttons >> panel.btn[MB[i]]) & 1u) {
            b = MB[i];
            break;
        }
    if (b == 0xFFu)
        return 0;
    if (b == B_HOME)
        f = FAM_TRK;
    else if (b == B_SCL)
        f = FAM_FLT;
    else
        for (j = FAM_HOME + 1u; j < FAM_COUNT; j++)
            if (FAM_BTN[j] == b && j != FAM_TRK && j != FAM_SCL) {
                f = j;
                break;
            }
    if (f >= FAM_COUNT)
        return 0;
    tm_dn = 0;
    for (i = 0; i < 3u; i++)
        if (TM_BTN[i] == b)
            tm_eat |= (uint8_t)(1u << i);
    ui.pg_down &= (uint16_t)~(1u << panel.btn[b]);
    if (ui.ly)
        layer_kill();
    if (b == B_HOME)
        ui.home_t0 |= 2u;
    if (ui.home || page_over || cur_page()->fam != f) {
        if (!page_visible(page_first(f)))
            fxt_set(!fxl_sel);
        j = ui.fam_last[f];
        ui.page = (uint8_t)(j && j < NPAGES && PAGES[j].fam == f && page_visible(j) ? j : page_first(f));
    } else {
        cur = ui.page;
        for (n = 0; n < (uint32_t)(s < 0 ? -s : s); n++)
            for (j = 0; j < NPAGES; j++) {
                cur = (cur + (s > 0 ? 1u : NPAGES - 1u)) % NPAGES;
                if (PAGES[cur].fam == f && page_visible(cur))
                    break;
            }
        ui.page = (uint8_t)cur;
    }
    ui.fam_last[f] = ui.page;
    ui.home = 0;
    page_entered();
    tl_menu_f = (uint8_t)(f + 1u);
    tl_menu_b = (uint8_t)b;
    ui.force = 1;
    return 1;
}
static void edit_param(uint32_t slot, int32_t steps)
{
    int16_t *vp;
    const page_t *pg = cur_page();
    const param_desc_t *d;
    int32_t v;
    { uint32_t gi=gen_index(pg);
      if (gi<4u) {
        if(tl_an) return;
        if(slot==0u){gen_amount[gi]=(uint8_t)clamp((int32_t)gen_amount[gi]+steps,0,100);ui.force=1;return;}
        if(slot==1u)return;
        if(slot==2u){slot=3u;steps=-1;}
      }
    }
    if (pg->scope == SC_PHONK) {
        uint32_t id=pg->id[slot&3u];
        if(id>=PH_COUNT)return;
        if(id==PH_QPICK || id==PH_QMOVE || id==PH_QPITCH) {
            pr_edit(id==PH_QPICK?0u:id==PH_QMOVE?1u:2u,steps);
            ui.force=1;
        } else if(id==PH_CAPTURE && steps>0) {
            uint32_t mask=fxl_mute?64u:0u;for(uint32_t k=0;k<NTRK;k++)if(!trk[k].p[P_MUTE])mask|=1u<<k;
            ph_masks[(uint32_t)ph_scene%4u]=(int16_t)mask;ui_message("SCENE STORED");
        } else if(id==PH_APPLY && steps>0) {
            uint32_t mask=(uint32_t)ph_masks[(uint32_t)ph_scene%4u];
            if(song.playing) {
                sc_pend=(int16_t)mask;
            } else {
                sc_pend=-1;
                fm1_irq_off();sc_mask_set(mask);fm1_irq_on();
            }
            ui.force=1;
        } else if(id==PH_BMAKE || id==PH_JMAKE || id==PH_FMAKE) {
            if(steps<=0)return;
            if(song.playing || song.rec || transport_req)ui_message("STOP BEFORE MAKE");
            else if(!ph_track_ok(TSEL,id))ui_message(id==PH_JMAKE?"USE POLY SYNTH":"USE SYNTH OR 808");
            else {ph_request=(uint8_t)id;ui.confirm=CF_PHONK;ui.confirm_trk=song.sel;ui.force=1;}
        } else {
            int16_t *v=ph_value(id);*v=(int16_t)param_turn(&PH_D[id],*v,steps);ui.force=1;
        }
        return;
    }
    if (pg->scope == SC_SLICE) {
        uint32_t id = pg->id[slot & 3u];
        if (id == SL_MAKE) {
            if (steps > 0)
                sl_make();
            else if (steps < 0)
                sl_clear();
        } else if (id < SL_COUNT) {
            int16_t was = sl_cfg[id];
            sl_cfg[id] = (int16_t)param_turn(&SL_D[id], sl_cfg[id], steps);
            if (id == SL_MAN && sl_cfg[id] && !was)
                sl_mm = 0;
        }
        ui.force = 1;
        return;
    }
    if (pg->scope == SC_TAPE) {
        uint32_t id=pg->id[slot&3u];
        if(id<TF_COUNT) {
            tape_cfg[id]=(int16_t)param_turn(&TF_D[id],tape_cfg[id],accel(EN_K1+slot,steps,TF_D[id].fmt==F_ENUM?0:127));
            ui.force=1;
        }
        return;
    }
    if (str_eq(pg->title, "DJ") && dj_sel < NTRK) {
        if ((slot & 3u) == 0u)
            dj_trk[dj_sel] = (int8_t)param_turn(&GP[G_MIDI], dj_trk[dj_sel], accel(EN_K1, steps, 127));
        ui.force = 1;
        return;
    }
    if (pg->scope == SC_OVIEW && str_eq(pg->title, "PIANO ROLL")) {
        pr_edit(slot & 3u, steps);
        ui.force = 1;
        return;
    }
    if (pg->scope == SC_OVIEW) {
        if ((slot & 3u) == 0u)
            ov_bar = (int16_t)param_turn(&OV_D, ov_bar, steps);
        else if ((slot & 3u) == 1u)
            seq_bar = (int16_t)param_turn(&SB_D, seq_bar, steps);
        else if ((slot & 3u) == 2u) {
            bar_dup = (int16_t)param_turn(&BD_D, bar_dup, steps);
            bd_t = fm1_ms | 1u;
        }
        ui.force = 1;
        return;
    }
    if ((pg->scope == SC_HATGEN || pg->scope == SC_TRAPGEN || (pg->scope == SC_PHONK && (pg->id[3] == PH_BMAKE ||
         pg->id[3] == PH_JMAKE))) && tl_an) {
        ui.force = 1;
        return;
    }
    if (pg->scope == SC_HATGEN) {
        uint32_t id = pg->id[slot & 3u];
        if (id == 3u) {
            if (song.playing || song.rec || transport_req)
                ui_message("STOP BEFORE MAKE");
            else if (steps < 0)
                ui_message(hg_undo() ? "HATS UNDONE" : "NO HATS UNDO");
            else if (hg_target() >= NTRK)
                ui_message("NO HIHAT TRACK");
            else {
                ui.confirm = CF_HATGEN;
                ui.confirm_trk = (uint8_t)hg_target();
            }
        }
        ui.force = 1;
        return;
    }
    if (pg->scope == SC_TRAPGEN) {
        uint32_t id = pg->id[slot & 3u];
        if (id == 3u) {
            if (song.playing || song.rec || transport_req)
                ui_message("STOP BEFORE MAKE");
            else if (steps < 0)
                ui_message(tg_undo() ? "TRAP GEN UNDONE" : "NO TRAP GEN UNDO");
            else {
                ui.confirm = CF_TRAPGEN;
                ui.confirm_trk = 0;
            }
        }
        ui.force = 1;
        return;
    }
    if(pg->scope==SC_GEN85) {
        uint32_t id=pg->id[slot&3u],k=song.sel;
        if(id==2u || id==3u || id==7u) {
            if(steps<=0)return;
            if(song.playing || song.rec || transport_req)ui_message("STOP BEFORE MAKE");
            else if(id==2u)ui_message(trap_gen_undo(TSEL)?"PATTERN RESTORED":"NO GENERATOR UNDO");
            else if(acid_on(TSEL) || (id==3u && chord_kit(TSEL)))ui_message("UNSUITABLE TRACK");
            else {ui.confirm=id==7u?CF_EUCLID:CF_CHORD_GEN;ui.confirm_trk=(uint8_t)k;ui.force=1;}
        } else {
            int16_t *v=id==0u?&gen_var[k]:id==1u?&gen_live[k]:&eu_cfg[k][id-4u];
            *v=(int16_t)param_turn(&G85_D[id],*v,steps);
            if(id==0u)gen_fresh[k]=0;
            if(id==4u && eu_cfg[k][1]>*v)eu_cfg[k][1]=*v;
            if(id==5u && *v>eu_cfg[k][0])*v=eu_cfg[k][0];
            ui.force=1;
        }
        return;
    }
    if (pg->scope == SC_BANKALL) {
        uint32_t k = song.sel % NTRK, b = (uint32_t)bk_bank[k] % 5u;
        if (slot == 0u)
            bk_bank[k] = (int16_t)param_turn(&BK_D[0], bk_bank[k], steps);
        else if (slot == 1u && b)
            bank_extra_pick[b - 1u][k] = (int16_t)param_turn(&EXTRA_D[b - 1u][0], bank_extra_pick[b - 1u][k], steps);
        else if (slot == 1u)
            bank_a_pick[k] = (int16_t)param_turn(&BANKA_D[0], bank_a_pick[k], steps);
        else if (slot == 2u && steps > 0) {
            if (b)
                trap_bank_load(TSEL, b - 1u, (uint32_t)bank_extra_pick[b - 1u][k]);
            else
                trap_bank_a_load(TSEL, (uint32_t)bank_a_pick[k]);
        }
        ui.force = 1;
        return;
    }
    if(pg->scope==SC_BANKS) {
        uint32_t b=pg->id[3]%4u;
        if(slot==0u) {
            bank_extra_pick[b][song.sel]=(int16_t)param_turn(&EXTRA_D[b][0],bank_extra_pick[b][song.sel],steps);
            ui.force=1;
        } else if(slot==1u && steps>0)trap_bank_load(TSEL,b,(uint32_t)bank_extra_pick[b][song.sel]);
        return;
    }
    if (pg->scope == SC_BANKA) {
        if (slot == 0u) {
            bank_a_pick[song.sel] = (int16_t)param_turn(&BANKA_D[0], bank_a_pick[song.sel], steps);
            ui.force = 1;
        } else if (slot == 1u && steps > 0) {
            trap_bank_a_load(TSEL, (uint32_t)bank_a_pick[song.sel]);
        }
        return;
    }
    if (pg->scope == SC_CHGEN) {
        uint32_t id = pg->id[slot & 3u];
        if (id == 4u && steps > 0) {
            if (song.playing || song.rec || transport_req) ui_message("STOP BEFORE MAKE");
            else if (chord_kit(TSEL) || acid_on(TSEL)) ui_message("USE A SYNTH TRACK");
            else { ui.confirm = CF_CHORD_GEN; ui.confirm_trk = song.sel; ui.force = 1; }
        } else if (id < 4u) {
            int32_t v = param_turn(&CG_DESC[id], chord_cfg[song.sel][id], steps);
            if (id == CG_FOLLOW && v == (int32_t)song.sel + 1) {
                ui_message("CANNOT FOLLOW SELF"); v = 0;
            }
            if (id == CG_FOLLOW && v && !(an_is_kick(TSEL) || !chord_kit(TSEL))) {
                ui_message("SYNTH OR 808 KICK"); v = 0;
            }
            chord_cfg[song.sel][id] = (int16_t)v;
            if(id!=CG_FOLLOW)gen_fresh[song.sel]=0;
            ui.force = 1;
        }
        return;
    }
    if (pg->scope == SC_TRK) {
        tracks_edit(slot, steps);
        return;
    }
    if (pg->graph == GR_GBSEQ) {
        gbseq_knob(slot, steps);
        return;
    }
    if (pg->graph == GR_ACID) {
        acid_knob(slot, steps);
        return;
    }
    if (pg->graph == GR_STAGE) {
        sg_knob(slot, steps);
        return;
    }
    if (pg->graph == GR_PERF) {
        if (slot == 0u)
            pm_rel = (uint8_t)clamp((int32_t)pm_rel + (steps > 0 ? 1 : -1), 0, 16);
        else {
            static const uint8_t GK[3] = {G_MIDI, G_SYNC, G_CRATE};
            int16_t *gp = &song.g[GK[slot - 1u]], was = *gp;
            const param_desc_t *gd = &GP[GK[slot - 1u]];
            *gp = (int16_t)param_turn(gd, *gp, accel(EN_K1 + slot, steps, gd->max - gd->min));
            pm_record(TSEL, gp, was);
            g_capture(gp);
        }
        ui.force = 1;
        return;
    }
#if FELUCCA_CPU2
    if (slot == 3u && pg->id[3] == 0xFFu && str_eq(pg->title, "CLOCK")) {
        cpu2_knob(steps);
        return;
    }
#endif
    d = page_desc(pg, slot, &vp);
    if (!d || !vp || d->max == d->min)
        return;
    if (FELUCCA_SIMPLE && kit_cur[song.sel]) {
        if (vp >= TSEL->p && vp < TSEL->p + P_COUNT)
            kit_drop(TSEL, (uint32_t)(vp - TSEL->p));
        else if (vp >= song.g && vp < song.g + G_COUNT && motion_gid((uint32_t)(vp - song.g)) != 0xFFu)
            kit_drop(TSEL, motion_gid((uint32_t)(vp - song.g)));
    }
    if (FELUCCA_SIMPLE && vp == &TSEL->p[P_E3] && smp_len_fit(TSEL) >= 0 && *vp > smp_len_fit(TSEL))
        *vp = (int16_t)smp_len_fit(TSEL);
    if (FELUCCA_SIMPLE && vp == &TSEL->p[P_E5] && eng_idx(TSEL->eng_req) == ENGI_SAMPLER)
        *vp = (int16_t)smp_slot(*vp);
    v = param_turn(d, *vp, accel(EN_K1 + slot, steps, d->fmt == F_ENUM ? 0 : d->max - d->min));
    if (FELUCCA_SIMPLE && vp == &TSEL->p[P_E5] && eng_idx(TSEL->eng_req) == ENGI_SAMPLER && v > (int32_t)smp_n - 1)
        v = smp_n ? (int32_t)smp_n - 1 : 0;
    if (FELUCCA_SIMPLE && vp == &TSEL->p[P_E3] && smp_len_fit(TSEL) >= 0 && v > smp_len_fit(TSEL))
        v = smp_len_fit(TSEL);
    {
        int16_t was = *vp;
        *vp = (int16_t)v;
        pm_record(TSEL, vp, was);
    }
    if (FELUCCA_SIMPLE && vp == &TSEL->p[P_E2] && smp_len_fit(TSEL) >= 0 && TSEL->p[P_E3] > smp_len_fit(TSEL)) {
        TSEL->p[P_E3] = (int16_t)smp_len_fit(TSEL);
        motion_capture(TSEL, P_E3, TSEL->p[P_E3]);
    }
    if (vp == &TSEL->p[P_E7])
        machine_defaults(TSEL);
    if (vp >= TSEL->p && vp < TSEL->p + P_COUNT)
        motion_capture(TSEL, (uint32_t)(vp - TSEL->p), *vp);
    else
        g_capture(vp);
}

#if TRAP_UX_TEST
#include "trap_ux_undo.c"
#endif

static uint32_t oct_taps(uint32_t pressed, int here)
{
    static uint8_t down, chord;
    uint32_t dn = panel.btn[B_OCTDN], up = panel.btn[B_OCTUP];
    uint32_t now = ((fm1_in.buttons >> dn) & 1u) | ((fm1_in.buttons >> up) & 1u) << 1, tap;
    if (here)
        down |= (uint8_t)(((pressed >> dn) & 1u) | ((pressed >> up) & 1u) << 1);
    if (now == 3u)
        chord = 1;
    tap = down & ~now;
    down &= (uint8_t)now;
    if (chord) {
        tap = 0;
        chord = now != 0u;
    }
    return tap;
}

enum { BT_NONE, BT_TAP, BT_HOLD };
static uint32_t btn_hold(uint32_t *t0, uint32_t label, uint32_t now, int hold_ok)
{
    uint32_t tap;
    if ((fm1_in.buttons >> panel.btn[label]) & 1u) {
        if (!*t0)
            *t0 = (now | 1u) & ~2u;
        else if (hold_ok && !(*t0 & 2u) && now - (*t0 & ~3u) > 700u * 1000u * FM1_TICKS_PER_US) {
            *t0 |= 2u;
            return BT_HOLD;
        }
        return BT_NONE;
    }
    tap = *t0 && !(*t0 & 2u);
    *t0 = 0;
    return tap ? BT_TAP : BT_NONE;
}

static void enc_drop(void)
{
    uint32_t k;
    for (k = 0; k < NE; k++)
        panel_enc(k);
}

static uint32_t keys_mode(void)
{
    return gbsv_keys_on() ? 5u : sg_page() ? 9u : sl_keys_on() ? 10u : sgrid_on() ? 1u : pm_page() || pm_down() ? 4u : par_on() ? 6u : kit_on() ? 7u :
           acid_kon() ? 8u : 0u;
}

static void sc_poll(void)
{
    static int16_t was = -1;
    int32_t m = sc_pend;
    if (m >= 0 && !song.playing) {
        sc_pend = -1;
        fm1_irq_off();
        sc_mask_set((uint32_t)m);
        fm1_irq_on();
        m = -1;
    }
    if (was != (int16_t)m)
        ui.force = 1;
    was = (int16_t)m;
}

#include "trap_nav.c"

static void layer_arm(uint32_t pressed, uint32_t now);
static uint32_t layer_gesture(void);
static void layer_tap(uint32_t l);

#define SEQ_DBL_MS 350u
static int page_tap(uint32_t b)
{
    uint32_t f;
    nav_poll();
    if (FELUCCA_SIMPLE && b == B_SEQ && sg_page())
        return 1;
    if(FELUCCA_SIMPLE && b==B_SCL && gbs_held()) {
        if(sfn || (sfn_sel_t && (uint32_t)(fm1_ms-sfn_sel_t)<SEQ_DBL_MS)) {
            sfn=sfn==1u?2u:1u;sfn_sel_t=0;page_over=&SFN_PAGES[sfn-1u];
            gbs_keep();sfn_sync();ui.force=1;
        } else sfn_sel_t=fm1_ms|1u;
        return 1;
    }
    if (FELUCCA_SIMPLE && b == B_SEQ) {
        static uint32_t t_last;
        static uint8_t was;
        if (seq_eat) {
            seq_eat = 0;
            return 1;
        }
        if (pm_page()) {
            t_last = 0;
            ui.sgrid = 1;
            fxt_set(0);
            ui.force = 1;
            return 1;
        }
        if (t_last && (int32_t)(fm1_ms - t_last) < (int32_t)SEQ_DBL_MS) {
            t_last = 0;
            ui.sgrid = was;
            if (was == 4u)
                acid_of(song.sel)->play = 1;
            ui.fam_last[FAM_SEQ] = 0;
            ui.home = 0;
            open_family(FAM_SEQ);
            acid_follow();
        } else {
            t_last = fm1_ms | 1u;
            was = ui.sgrid;
            if (ui.sgrid == 4u)
                acid_of(song.sel)->play = 0;
            ui.sgrid = (uint8_t)(ui.sgrid == 1u || ui.sgrid == 4u ? 0u : 1u);
            acid_follow();
        }
        ui.force = 1;
        return 1;
    }
    if (FELUCCA_SIMPLE && b == FAM_BTN[FAM_FX] && !fxl_sel) {
        static uint32_t t_fx;
        if (t_fx && (int32_t)(fm1_ms - t_fx) < (int32_t)SEQ_DBL_MS && !ui.home && cur_page()->fam == FAM_FX) {
            t_fx = 0;
            fxt_set(1);
            ui.fam_last[FAM_FX] = ui.page;
            ui.force = 1;
            return 0;
        }
        t_fx = fm1_ms | 1u;
    }
    for (f = FAM_HOME + 1u; f < FAM_COUNT; f++)
        if (FAM_BTN[f] == b && b != B_HOME) {
            if (FELUCCA_SIMPLE && f != FAM_SAVE && !page_visible(page_first(f))) {
                fxt_set(!fxl_sel);
                ui.home = 1;
            }
            open_family(f);
            return 0;
        }
    return 0;
}

#define MF_HOLD_MS 250u

static void midi_take(void)
{
    uint32_t k, now = fm1_ms | 1u;
    fm1_irq_off();
    mf_seen = midi_seen;
    midi_seen = 0;
    fm1_irq_on();
    for (k = 0; k < NTRK; k++)
        if ((mf_seen >> k) & 1u)
            midi_lit_ms[k] = now;
}

static int mf_busy(void)
{
    return lock_held() || (ui.entry_open && (fm1_in.notes & ~kb_layer)) || ui.plk_rm || ui.confirm;
}
static int midi_follow(void)
{
    static uint32_t sw_ms;
    uint32_t seen = mf_seen, k, now = fm1_ms | 1u, cand, lit;
    mf_seen = 0;
    if (!seen)
        return 0;
    lit = midi_lit_ms[song.sel];

    cand = seen & ~(1u << song.sel);
    k = 0;
    if (cand && !((seen >> song.sel) & 1u) && (ui_rec_prefs & (PREF_MIDI_FOLLOW >> 8)) && song.g[G_ROUTE] != ROUTE_SEL &&
        !mf_busy() && (!lit || now - lit >= MF_HOLD_MS) &&
        (!sw_ms || now - sw_ms >= MF_HOLD_MS)) {
        while (!((cand >> k) & 1u))
            k++;
        track_select(k);
        sw_ms = now;
        k = 1;
    } else
        k = 0;
    return (int)k;
}

static void ui_notices(void)
{
    static uint32_t midi_t, midi_last;
    if (motion_full) { motion_full = 0; ui_message("AUTOMATION FULL"); }
    if (midi_follow())
        midi_hint = 0;
    if (midi_hint) {
        uint32_t h = midi_hint;
        midi_hint = 0;
        if (!ui.msg_t && (h != midi_last || fm1_ms - midi_t > 4000u)) {
            char b[4] = {'T', (char)('0' + h), 0, 0};
            ui_say("MIDI IN -> ", b);
            midi_last = h;
            midi_t = fm1_ms;
        }
    }
}

static void bm_close(void)
{
    bm_on = 0;
    kb_boot_hold = 0;
    enc_drop();
    ui.pg_down = 0;
    ui.force = 1;
}
static void bm_input(uint32_t pressed)
{
    int32_t s = panel_enc(EN_SELECT);
    uint32_t k;
    for (k = 0; k < NE; k++)
        if (k != EN_SELECT)
            (void)panel_enc(k);
    if (s || pressed)
        bm_touched = 1;
    if (bm_on == 3u) {
        if (fm1_ms - bm_t0 >= 1050u) { bm_close(); ui_message("NEW PROJECT"); }
        return;
    }
    if (bm_on == 2u) {
        if (s)
            bm_kit = (uint8_t)clamp((int32_t)bm_kit + (s > 0 ? 1 : -1), 0, 10);
        if ((pressed >> panel.btn[B_OCTDN]) & 1u)
            bm_on = 1;
        else if (((pressed >> panel.btn[B_OCTUP]) & 1u) && gbk_used(bm_kit)) {
            bm_close();
            if (gbk_load(bm_kit))
                ui_message("KIT LOADED");
        }
        ui.force = 1;
        return;
    }
    if (s)
        bm_sel = (uint8_t)clamp((int32_t)bm_sel + (s > 0 ? 1 : -1), 0, 2);
    if ((pressed >> panel.btn[B_OCTUP]) & 1u) {
        if (bm_sel == 1u) {
            bm_on = 2;
            for (bm_kit = 0; bm_kit < 10u && !gbk_used(bm_kit); bm_kit++)
                ;
        } else {
            bm_close();
            if (bm_sel == 0u) {
                cla_clear(1);
                bm_on = 3; bm_t0 = fm1_ms; bm_touched = 1; kb_boot_hold = 1;
            } else
                ui_message("LAST JAM");
        }
    } else if (!bm_touched && fm1_ms - bm_t0 >= BM_AUTO_MS) {
        bm_close();
        ui_message("LAST JAM");
    }
    ui.force = 1;
}
static void ui_input(void)
{
    uint32_t pressed = fm1_input_edges(0), notes = fm1_input_note_edges(), now = fm1_ticks(), id, b, k;
    uint32_t home, rec, save;
    uint32_t oct;
    uint32_t lytap;
    int32_t s;
    midi_take();
    midi_pc_poll();
    fx_usb_fixed = (ui_prefs & PREF_USB_FIXED) != 0u;
    scope_mix = (ui_prefs2 & (PREF_SCOPE_MIX >> 16)) != 0u;
    follow_poll();
    if (scr_input(pressed, notes))
        return;
    if (bm_on) {
        bm_input(pressed);
        return;
    }
#if TRAP_UX_TEST
    pressed = ux_undo_button(pressed);
#endif
    pressed = tm_poll(pressed);
    fill_poll();
    if (FELUCCA_SIMPLE)
        cla_poll(pressed);
    hint_poll();
    if (mom.n && !mom_down()) {
        mom_restore();
        ui_message(FELUCCA_SIMPLE ? "REVERTED" : "BACK");
    }
    if (!FELUCCA_SIMPLE && mom.n && ((pressed >> panel.btn[MOM_KEEP]) & 1u)) {
        mom.n = 0;
        pressed &= ~(1u << panel.btn[MOM_KEEP]);
        ui_message("KEPT");
    }
    home = btn_hold(&ui.home_t0, B_HOME, now, 1);
    rec = btn_hold(&ui.rec_t0, B_REC, now, 0);
    if (FELUCCA_SIMPLE) {
        static uint32_t rq_t0;
        if (!((fm1_in.buttons >> panel.btn[B_REC]) & 1u))
            rq_t0 = 0;
        else if (((pressed >> panel.btn[B_REC]) & 1u))
            rq_t0 = fm1_ms | 1u;
        else if (rq_t0 && (int32_t)(fm1_ms - rq_t0) >= (int32_t)HINT_MS && !ui.confirm) {
            rq_t0 = 0;
            layer_kill();
            TSEL->p[P_SQNT] = (int16_t)!TSEL->p[P_SQNT];
            ui_message(TSEL->p[P_SQNT] ? "QUANTIZE ON" : "QUANTIZE OFF");
        }
    }
    save = btn_hold(&ui.save_t0, B_SAVE, now, !ui.confirm);
    layer_arm(pressed, now);
    if (((pressed >> panel.btn[B_REC]) & 1u) && ui.ly == LAYER_REC)
        ui.rec_t0 |= 2u;
    oct = oct_taps(pressed, ui.confirm != 0);
    lytap = layer_gesture();
    seq_white(notes);
    par_poll();
    song.grid = (uint8_t)keys_mode();
    sc_keys = (uint8_t)(FELUCCA_SIMPLE && !ui.home && !page_over && !ui.confirm && ui.sgrid == 0u &&
                        str_eq(cur_page()->title, "SCENES") && !fxt_x(cur_page()));
    dj_keys = (uint8_t)(FELUCCA_SIMPLE && !ui.home && !page_over && !ui.confirm && ui.sgrid == 0u &&
                        str_eq(cur_page()->title, "DJ") && !fxt_x(cur_page()));
    cb_keys = (uint8_t)(FELUCCA_SIMPLE && !ui.home && !page_over && !ui.confirm && ui.sgrid == 0u &&
                        str_eq(cur_page()->title, "COMB") && !fxt_x(cur_page()));
    if (rec == BT_TAP && !ui.confirm)
        rec_tap();
    (void)save;
    if (FELUCCA_SIMPLE && ((pressed >> panel.btn[B_SAVE]) & 1u) && !ui.confirm && clip_save())
        pressed &= ~(1u << panel.btn[B_SAVE]);
    gbsv_buttons(pressed, !ui.confirm);
    if (ui.confirm) {
        if (oct & 2u) {
            uint32_t kind = ui.confirm;
            ui.confirm = 0;
            ui.force = 1;
            if (kind == CF_TRAPGEN) {
                if (song.playing || song.rec || transport_req)
                    ui_message("STOP BEFORE MAKE");
                else {
                    trap_drum_make();
                    tl_anim(TL_AN_TRAP, "TRAP GEN: EDIT FREELY");
                    tl_an_goto = "SEQUENCER";
                }
            } else if (kind == CF_HATGEN) {
                track_t *t = &trk[ui.confirm_trk % NTRK];
                if (song.playing || song.rec || transport_req)
                    ui_message("STOP BEFORE MAKE");
                else if (!hg_is_hat(t) || acid_on(t))
                    ui_message("NO HIHAT TRACK");
                else {
                    trap_hat_make(t);
                    track_select(trk_index(t));
                    tl_anim(TL_AN_HAT, "HATS: EDIT FREELY");
                    tl_an_goto = "SEQUENCER";
                }
            } else if(kind==CF_EUCLID) {
                track_t *t=&trk[ui.confirm_trk%NTRK];
                if(song.playing || song.rec || transport_req)ui_message("STOP BEFORE MAKE");
                else if(acid_on(t))ui_message("NO ACID TRACK");
                else {undo.keep=0;load_begin(t,UNDO_PAT);trap_euclid_generate(t);load_end(t);ui_message("EUCLID: EDIT FREELY");
                      tl_goto(tl_result_page(t));}
            } else if(kind==CF_PHONK) {
                track_t *t=&trk[ui.confirm_trk%NTRK];
                if(song.playing || song.rec || transport_req)ui_message("STOP BEFORE MAKE");
                else if(!ph_track_ok(t,ph_request))ui_message("UNSUITABLE TRACK");
                else {
                    load_begin(t,UNDO_PAT);
                    if(ph_request==PH_FMAKE)qf_before(t);
                    if(ph_request!=PH_FMAKE && !ph_fixed){tr_seed();if(ph_request==PH_JMAKE)jz_draw();}
                    trap_phonk_generate(t,ph_request);tr_mix=100;
                    if(ph_request==PH_FMAKE)qf_after(t);
                    load_end(t);
                    if(ph_request==PH_BMAKE || ph_request==PH_JMAKE) {
                        tl_anim(ph_request==PH_BMAKE?TL_AN_BASS:TL_AN_JAZZ,"PHONK: EDIT FREELY");
                        tl_an_goto="PIANO ROLL";
                    } else {ui_message("PHONK: EDIT FREELY");tl_goto(tl_result_page(t));}
                }
            } else if (kind == CF_CHORD_GEN) {
                track_t *t = &trk[ui.confirm_trk % NTRK];
                if (song.playing || song.rec || transport_req) ui_message("STOP BEFORE MAKE");
                else if (chord_kit(t) || acid_on(t)) ui_message("USE A SYNTH TRACK");
                else {
                    undo.keep=0;
                    load_begin(t, UNDO_PAT);
                    trap_chord_generate(t);
                    load_end(t);
                    if (str_eq(cur_page()->title, "CHORD VAR")) {
                        tl_anim(TL_AN_DICE, "CHORDS: EDIT FREELY");
                        tl_an_goto = "PIANO ROLL";
                    } else {
                        ui_message("CHORDS: EDIT FREELY");
                        tl_goto("PIANO ROLL");
                    }
                }
            } else if (kind == CF_CLEAR_MOTION) {
                track_t *t = &trk[ui.confirm_trk % NTRK];
                { load_begin(t, UNDO_PAT); motion_clear(t); load_end(t); ui_message("AUTOMATION CLEARED"); }
            } else if (kind == CF_INIT_SOUND) {
                { set_engine(TSEL->eng_req); ui_message("SOUND INIT"); }
            } else if (kind == CF_CLEAN_SOUND) {
                { clean_engine(); ui_message("CLEAN INIT"); }
            } else {
                track_t *t = &trk[ui.confirm_trk % NTRK];
                load_begin(t, UNDO_PAT);
                track_defaults_steps(t);
                load_end(t);
                t->nheld = 0;
                t->arp_phys = 0;
                if (kind == CF_CLEAR_TRK) {
                    char b[12] = "1 CLEARED";
                    b[0] = (char)('1' + ui.confirm_trk);
                    ui_say("TRACK ", b);
                } else {
                    ui_message("PATTERN CLEARED");
                }
            }
        } else if (oct & 1u) {
            ui.confirm = 0;
            ui.force = 1;
        }
        ui.pg_down = 0;
        enc_drop();
        return;
    }
    if (lytap)
        layer_tap(lytap);
    if (home == BT_TAP)
        home_tap();
    cursor_fix();
    for (id = 0; id < 14u; id++) {
        if (!((pressed >> id) & 1u))
            continue;
        b = panel_btn_of(id);
        switch (b) {
        case B_PLAY:
            if (FELUCCA_SIMPLE && song.playing && !seq_counting()) {
                fill_arm = 1;
                fill_t0 = fm1_ms;
                break;
            }
            if (FELUCCA_SIMPLE && stop_ms && (int32_t)(fm1_ms - stop_ms) < (int32_t)PANIC_MS &&
                !song.playing && !seq_counting()) {
                stop_ms = 0;
                panic_req = PANIC_ALL;
                ui_message("ALL SOUND OFF");
                break;
            }
            stop_ms = 0;
            if (song.playing || seq_counting())
                transport_req = 2;
            else
                transport_req = 1;
            break;
        case B_SEQ:
            if (FELUCCA_SIMPLE && seq_pick) {
                seq_pick = 0;
                seq_eat = 1;
                ui.force = 1;
            }
            break;
        case B_REC:
        case B_SAVE:
        case B_FX:
        case B_GLO:
        case B_SCL:
        case B_EDIT:
        case B_ENV:
        case B_HOME:
            break;
        case B_OCTDN:
        case B_OCTUP: {
            if (FELUCCA_SIMPLE && b == B_OCTUP && seq_dn() && !fxl_sel) {
                layer_kill();
                seq_pick = 0;
                ui.sgrid = 4;
                acid_of(song.sel)->play = 1;
                acid_page(0);
                mt_used = 1;
                break;
            }
            if (FELUCCA_SIMPLE && b == B_OCTUP && cla_st == 2u) {
                cla_clear(1);
                cla_st = 3;
                break;
            }
            if (b == B_OCTDN)
                cla_oct = (int8_t)song.octave;
            if (FELUCCA_SIMPLE && b == B_OCTDN && page_btn_reset())
                break;
            if (FELUCCA_SIMPLE && b == B_OCTDN && pm_h.n == 1u) {
                pm_clear();
                ui_message("MACRO CLEARED");
                break;
            }
            if (FELUCCA_SIMPLE && b == B_OCTDN && kit_held()) {
                uint32_t h = kit_held(), p;
                for (p = 0; p < 16u; p++)
                    if ((h >> p) & 1u) {
                        if (kit_cur[song.sel] == p + 1u)
                            kit_undo(TSEL);
                        kit[song.sel][p].n = 0;
                    }
                ui_message("KEY LOCKS CLEARED");
                break;
            }
            if (FELUCCA_SIMPLE && b == B_OCTDN && acid_kon()) {
                uint32_t h = acid_held(), p;
                acid_t *a = acid_of(song.sel);
                if (h) {
                    acid_t d;
                    acid_reset(&d);
                    for (p = 0; p < ACID_N; p++)
                        if ((h >> p) & 1u)
                            a->st[p] = d.st[p];
                    ui_message("STAGE RESET");
                    ui.force = 1;
                } else {
                    cla_st = 1;
                }
                break;
            }
            if (FELUCCA_SIMPLE && b == B_OCTUP && !sgrid_on())
                break;
            if (FELUCCA_SIMPLE && sgrid_on()) {
                uint32_t other = b == B_OCTDN ? B_OCTUP : B_OCTDN, a, i2, all = (fm1_in.buttons >> panel.btn[other]) & 1u;
                if (b == B_OCTDN && !fxl_on()) {
                    uint32_t k2, hk = NTRK;
                    for (k2 = 0; k2 < 27u; k2++)
                        if (((fm1_in.notes >> k2) & 1u) && key_black(k2) && key_place(k2) < NTRK)
                            hk = key_place(k2);
                    if (hk < NTRK) {
                        track_t *t2 = &trk[hk];
                        apply_preset_to(t2, trk_machine(t2)->preset);
                        machine_defaults(t2);
                        lastc[hk] = 0;
                        ui_message("SOUND RESET");
                        ui.force = 1;
                        break;
                    }
                }
                if (b == B_OCTDN && gbs_page_held() >= 0) {
                    gbs_page_del((uint32_t)gbs_page_held());
                    break;
                }
                if (b == B_OCTDN && gbs_held()) {
                    gbs_keep();
                    lock_clear(gbs_held());
                    ui_message("STEP LOCKS CLEARED");
                    break;
                }
                (void)a;
                (void)i2;
                if (b == B_OCTDN && !all)
                    cla_st = 1;
                break;
            }
            uint32_t both = (1u << panel.btn[B_OCTDN]) | (1u << panel.btn[B_OCTUP]);
            if ((fm1_in.buttons & both) == both)
                song.octave = 0;
            else
                song.octave += b == B_OCTDN ? (song.octave > -3 ? -1 : 0) : (song.octave < 3 ? 1 : 0);
            break;
        }
        default:
            ui.pg_down |= (uint16_t)(1u << id);
            break;
        }
    }
    b = ui.pg_down & ~fm1_in.buttons;
    ui.pg_down &= (uint16_t)~b;
    for (id = 0; b; id++, b >>= 1)
        if (b & 1u)
            page_tap(panel_btn_of(id));
    {
        static uint32_t ou_t0;
        static uint8_t ou_dn, ou_used;
        uint32_t dn = (fm1_in.buttons >> panel.btn[B_OCTUP]) & 1u;
        if (dn && !ou_dn) {
            ou_t0 = fm1_ms;
            ou_used = (uint8_t)((fm1_in.buttons >> panel.btn[B_OCTDN]) & 1u);
        } else if (dn && ((fm1_in.buttons >> panel.btn[B_OCTDN]) & 1u)) {
            ou_used = 1;
        }
        if (!dn && ou_dn && !ou_used && !ou_eat && fm1_ms - ou_t0 < HINT_MS &&
            !ui.confirm && !sgrid_on() && !par_on() && !acid_kon())
            song.octave += song.octave < 3 ? 1 : 0;
        else if (FELUCCA_SIMPLE && !dn && ou_dn && !ou_used && !ou_eat && fm1_ms - ou_t0 < HINT_MS && !ui.confirm &&
                 ((sgrid_on() && !fxl_sel) || acid_kon())) {
            acid_of(song.sel)->play = (uint8_t)!acid_kon();
            acid_follow();
        }
        kb_octup = (uint8_t)dn;
        if (mt_used || mom.n || ou_eat) {
            ou_used = 1;
            mt_used = 0;
            ou_eat = 0;
        }
        ou_dn = (uint8_t)dn;
        alt_mode = 0;
    }
    par_poll();
    trk_mode_poll();
    acid_follow();
    song.grid = (uint8_t)keys_mode();
    if (song.grid != 7u)
        for (k = 0; k < NTRK; k++)
            if (kit_cur[k])
                kit_undo(&trk[k]);
    pm_poll();
    sg_poll();
    sc_poll();
    sl_poll();
    bd_poll();
    nav_poll();
    sgrid_release();
    gbs_page_poll();
    gb_store_poll();
    jam_poll();
    gl_poll();
    msrc_fix();
    if (song.grid == 5u) {
        gbsv_keys(notes);
    } else if (song.grid == 9u) {
        sg_press(notes);
    } else if (song.grid == 10u) {
        sl_press(notes);
    } else if (song.grid == 4u) {
        pm_keys(notes);
    } else if (song.grid == 6u) {
        par_keys(notes);
    } else if (song.grid == 7u) {
        kit_keys(notes);
    } else if (song.grid == 8u) {
        acid_keys(notes);
    } else if (song.grid && sgrid_on()) {
        sgrid_keys(notes);
    }
    if ((s = panel_enc(EN_PRESET)) != 0 && gl_enc(EN_PRESET, s)) {
        ;
    } else if (s && !fxl_sel) {
        int32_t m = clamp((int32_t)trk_mach(TSEL) + (s > 0 ? 1 : -1), 0, (int32_t)NMACH - 1);
        if ((uint32_t)m != trk_mach(TSEL))
            machine_set(TSEL, (uint32_t)m);
    }
    sfn_sync();
    if ((s = panel_enc(EN_ALGO)) != 0 && gl_enc(EN_ALGO, s)) {
        ;
    } else if (s && FELUCCA_SIMPLE && !fxl_on() && gbs_held()) {
        sfn_algo(s);
    } else if (s && FELUCCA_SIMPLE) {
        gbs_off = gbs_kept = 0;

        int32_t to = fxl_sel ? (s > 0 ? (int32_t)NTRK : (int32_t)song.sel) : clamp((int32_t)song.sel + (s > 0 ? 1 : -1), 0, NTRK);
        fxt_set(to == NTRK);
        if (to < NTRK)
            track_select((uint32_t)to);
        ui.force = 1;
    } else if (s) {
        int16_t was = TSEL->p[P_LEVEL];
        TSEL->p[P_LEVEL] = (int16_t)param_turn(&TP[P_LEVEL], was, accel(EN_ALGO, s, TP[P_LEVEL].max - TP[P_LEVEL].min));
        pm_record(TSEL, &TSEL->p[P_LEVEL], was);
        motion_capture(TSEL, P_LEVEL, TSEL->p[P_LEVEL]);
        ui.force = 1;
    }
    if ((s = panel_enc(EN_SELECT)) != 0 && gl_enc(EN_SELECT, s)) {
        ;
    } else if (s && !(tm_dn && tm_dest()) && pm_menu(s)) {
        ;
    } else if (s && tm_dn) {
        if (tm_dest()) {
            *tm_amt() = (int16_t)clamp(*tm_amt() + accel(EN_SELECT, s, 127), -64, 63);
            tm_k = 4;
            tm_t = fm1_ms;
            ui.force = 1;
        }
    } else if (s && FELUCCA_SIMPLE && !gbs_held() && (gen_select(s) || pr_select(s) || mix_select(s) || qf_select(s))) {
        ;
    } else if (s && FELUCCA_SIMPLE && !fxl_on() && gbs_held()) {
        uint64_t h = gbs_held();
        for (k = 0; k < NSTEP; k++) {
            step_t *st = &TSEL->step[k];
            if ((h >> k) & 1u && st->time == ST_NOTE && st->n)
                step_set_nudge(st, step_nudge(st) + (s > 0 ? 1 : -1));
        }
        gbs_keep();
        gb_nd_t = fm1_ms | 1u;
        ui.force = 1;
    } else if (s) {
        int16_t was = song.g[G_BPM];
        song.g[G_BPM] = (int16_t)clamp(song.g[G_BPM] + accel(EN_SELECT, s, 200), GP[G_BPM].min, GP[G_BPM].max);
        pm_record(TSEL, &song.g[G_BPM], was);
        ui.bpm_t = 40;
    }
    for (k = 0; k < 4u; k++) {
        const page_t *pg = cur_page();
        int16_t *hv;
        uint64_t held;
        if ((s = panel_enc(EN_K1 + k)) == 0)
            continue;
#if TRAP_UX_TEST && TRAP_FM6_TEST
        if(!ui.home && pg->scope==SC_FM6 && gl_btn(B_OCTUP)) {
            if(k==0u) tf6_ui_op=(uint8_t)clamp((int32_t)tf6_ui_op+s,0,5);
            else if(k==1u && pg->id[0]==126u) tf6_ui_pitch=(uint8_t)(s>0);
            ou_eat=1;ui.force=1;continue;
        }
#endif
        if (k == 0u && !ui.home && (str_eq(pg->title, "PIANO ROLL") || str_eq(pg->title, "QUICK FILL")) &&
            gl_btn(B_OCTUP)) {
            pr_mode(s);
            continue;
        }
        if (gl_knob(k, s))
            continue;
        if (!ui.home && fxt_x(pg))
            continue;
        if (tm_dn) {
            tm_turn(k, s);
            continue;
        }
        if (sfn && gbs_held()) {
            sfn_turn(k, s);
            continue;
        }
        if (seq_knob(k))
            continue;
        if (!FELUCCA_SIMPLE && mom_down())
            ui.pg_down &= (uint16_t)~(1u << panel.btn[B_LFO]);
        if (kit_held() && kit_turn(k, s, kit_held())) {
            ui.hot_col = (uint8_t)k;
            ui.hot_t = 40;
            continue;
        }
        if ((held = lock_held()) != 0u) {
            gbs_keep();
            lock_turn(k, s, held);
            ui.hot_col = (uint8_t)k;
            ui.hot_t = 40;
            continue;
        }
        if (FELUCCA_SIMPLE && mom_down() && k == 0u && smp_osc1()) {
            smp_lay = (uint8_t)((smp_lay + (s > 0 ? 1u : 2u)) % 3u);
            ou_eat = 1;
            ui.force = 1;
            continue;
        }
        if (FELUCCA_SIMPLE && mom_down() && k == 1u && smp_osc1()) {
            if (mom_put(TSEL, track_desc(TSEL, P_E4), &TSEL->p[P_E4], k, s)) {
                ui.hot_col = (uint8_t)k;
                ui.hot_t = 40;
            }
            continue;
        }
        if (mom_down() && mom_turn(k, s)) {
            ui.hot_col = (uint8_t)k;
            ui.hot_t = 40;
            continue;
        }
        if (pg->scope == SC_TRK || page_desc(pg, k, &hv)) {
            ui.hot_col = (uint8_t)k;
            ui.hot_t = 40;
        }
        edit_param(k, s);
    }
    ui_notices();
}

#define SETUP_IDLE_MS 30000u
#define SETUP_GIVE_UP(t0) (fm1_ms - (t0) > SETUP_IDLE_MS || fm1_in.notes != 0u)
static void setup_title(void)
{
    lcd_fill(0, 0, 240, 240, T_BG);
    {
        const char *t = "HARDWARE CALIBRATION";
        int32_t x = (240 - (16 + 6 + text_w(&AF_M, t))) / 2;
        cv_begin(240, 24, T_BG);
        GFX_HOOK_ALIGN(0, 0, 240, 0, AL_H | AL_N(2), "calibration title centred");
        GFX_HOOK_ALIGN(0, HEAD_MY + AF_M_CAP_Y, 0, HEAD_MY + AF_M_CAP_Y + AF_M_CAP_H, AL_V | AL_PASS,
                       "header icon on its title's line");
        cv_icon_mid(x, 12, 16, ICON_X_DOCTOR, T_THEME, T_BG);
        cv_text(x + 22, HEAD_MY, &AF_M, t, T_TEXT);
        cv_blit(0, 5);
    }
    draw_text_box(0, 32, 240, &AF_S, "TEACH EACH BUTTON AND KNOB", T_MID, 1);
    lcd_fill(16, 56, 208, 1, T_LINE);
    draw_text_box(0, 206, 240, &AF_S, "ANY KEY: CANCEL", T_DIM, 1);
}
static void setup_show(const char *what, const char *name)
{
    draw_text_box(0, 80, 240, &AF_S, what, T_MID, 1);
    draw_text_box(0, 100, 240, &AF_L, name, T_THEME, 1);
}
static void panel_setup(void)
{
    uint32_t i, used = 0, t0 = fm1_ms;
    const panel_t old = panel;
    setup_title();
    while (fm1_in.buttons) {
        fm1_wdt_feed();
        if (SETUP_GIVE_UP(t0))
            goto timeout;
    }
    fm1_input_edges(0);
    for (i = 0; i < NB; i++) {
        uint32_t p = 0, id;
        setup_show("PRESS", B_NAME[i]);
        t0 = fm1_ms;
        while (!(p & ~used)) {
            fm1_wdt_feed();
            p |= fm1_input_edges(0);
            if (SETUP_GIVE_UP(t0))
                goto timeout;
        }
        for (id = 0; id < 14u; id++)
            if (((p & ~used) >> id) & 1u)
                break;
        panel.btn[i] = (uint8_t)id;
        used |= 1u << id;
    }
    used = 0;
    for (i = 0; i < NE; i++) {
        uint32_t e;
        int32_t st = 0;
        setup_show("TURN RIGHT", E_NAME[i]);
        for (e = 0; e < 7u; e++)
            fm1_enc_take(e);
        t0 = fm1_ms;
        for (;;) {
            fm1_wdt_feed();
            if (SETUP_GIVE_UP(t0))
                goto timeout;
            for (e = 0; e < 7u; e++)
                if (!((used >> e) & 1u) && (st = fm1_enc_take(e)) != 0)
                    break;
            if (e < 7u)
                break;
        }
        panel.enc[i] = (uint8_t)e;
        panel.dir[i] = (int8_t)(st > 0 ? 1 : -1);
        used |= 1u << e;
        fm1_delay_ms(300);
        fm1_enc_take(e);
    }
    panel.magic = PANEL_MAGIC;
    lcd_fill(0, 0, 240, 240, T_BG);
    ui.force = 1;
    return;
timeout:
    panel = old;
    lcd_fill(0, 0, 240, 240, T_BG);
    ui.force = 1;
    ui_message("SETUP CANCELLED");
}

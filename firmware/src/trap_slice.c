/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION contributors */

static uint16_t sl_mk[SMP_SLN], sl_mm;
static int sl_page(void)
{
    return !ui.home && !page_over && !ui.confirm && !fxl_sel && cur_page()->scope == SC_SLICE;
}
static int sl_keys_on(void) { return sl_page() && sl_cfg[SL_MAN]; }
static int sl_window(uint32_t *a, uint32_t *b)
{
    const track_t *t = TSEL;
    uint32_t sl = smp_slot(t->p[P_E5]), n, wa, wl;
    if (sl >= smp_n || !(n = smp_tab[sl].len))
        return 0;
    smp_window(n, t->p[P_E2], t->p[P_E3], &wa, &wl, 65u);
    *a = (uint32_t)(((uint64_t)wa << 16) / n);
    *b = (uint32_t)(((uint64_t)(wa + wl) << 16) / n);
    *b = *b > 65535u ? 65535u : *b;
    return *b > *a;
}
static uint32_t sl_auto(uint16_t *pos)
{
    uint32_t a, b, i, c = 0, g = 4u << (sl_cfg[SL_STYLE] % 3), n = (uint32_t)sl_cfg[SL_SET];
    if (!sl_window(&a, &b))
        return 0;
    for (i = 0; i <= n; i++) {
        uint32_t f = (2u * i * g + n) / (2u * n);
        uint32_t q = a + (uint32_t)(((uint64_t)(b - a) * f) / g);
        if (!c || q > pos[c - 1u])
            pos[c++] = (uint16_t)q;
    }
    return c ? c - 1u : 0u;
}
static uint32_t sl_manual(uint16_t *pos)
{
    uint32_t a, b, i, j = 0, c = 0;
    if (!sl_window(&a, &b))
        return 0;
    for (i = 0; i < SMP_SLN; i++)
        if (((sl_mm >> i) & 1u) && sl_mk[i] < b) {
            uint16_t q = sl_mk[i];
            for (j = c; j > 0 && pos[j - 1u] > q; j--)
                pos[j] = pos[j - 1u];
            pos[j] = q;
            c++;
        }
    for (i = j = 0; i < c; i++)
        if (!j || pos[i] > pos[j - 1u] + 128u)
            pos[j++] = pos[i];
    if (!j)
        return 0;
    pos[j] = (uint16_t)b;
    return j;
}
static void sl_stop(void)
{
    smp_pv_req[song.sel % NPART] = 2;
}
static void sl_make(void)
{
    smp_sl_t s;
    char b[4];
    memset(&s, 0, sizeof s);
    if (eng_idx(TSEL->eng_req) != ENGI_SAMPLER || fxl_sel) {
        ui_message("NO SAMPLER");
        return;
    }
    s.n = (uint8_t)(sl_cfg[SL_MAN] ? sl_manual(s.pos) : sl_auto(s.pos));
    if (!s.n) {
        ui_message(smp_slot(TSEL->p[P_E5]) < smp_n ? "TAP KEYS FIRST" : "NO SAMPLE");
        return;
    }
    fm1_irq_off();
    smp_sl[song.sel % NPART] = s;
    smp_sl_last[song.sel % NPART] = 0;
    fm1_irq_on();
    fmt_int(b, s.n);
    ui_say("SLICES ", b);
}
static void sl_clear(void)
{
    fm1_irq_off();
    smp_sl[song.sel % NPART].n = 0;
    smp_sl_last[song.sel % NPART] = 0;
    fm1_irq_on();
    ui_message("SLICES OFF");
}
static void sl_press(uint32_t pressed)
{
    uint32_t k, a, b;
    for (k = 0; k < 27u; k++) {
        uint32_t p = key_place(k);
        int32_t h;
        if (!((pressed >> k) & 1u) || key_black(k) || p >= SMP_SLN)
            continue;
        h = smp_pv_req[song.sel % NPART] == 1u ? -2 : smp_head(song.sel);
        if (h == -1) {
            if (!sl_window(&a, &b)) {
                ui_message("NO SAMPLE");
                return;
            }
            smp_pv_req[song.sel % NPART] = 1;
            h = (int32_t)a;
        } else if (h < 0) {
            h = sl_window(&a, &b) ? (int32_t)a : 0;
        }
        sl_mk[p] = (uint16_t)h;
        sl_mm |= (uint16_t)(1u << p);
        ui.force = 1;
    }
}
static uint32_t sl_leds(void)
{
    uint32_t k, m = 0;
    for (k = 0; k < 27u; k++)
        if (!key_black(k) && key_place(k) < SMP_SLN && ((sl_mm >> key_place(k)) & 1u))
            m |= 1u << k;
    return m;
}
static void sl_poll(void)
{
    static uint8_t was;
    uint32_t on = (uint32_t)sl_keys_on();
    if (was && !on)
        sl_stop();
    was = (uint8_t)on;
    if (on && smp_head(song.sel) >= 0)
        ui.force = 1;
}

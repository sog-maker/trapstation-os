/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION contributors */

static uint8_t nav_side[FAM_COUNT];
static void nav_poll(void)
{
    uint32_t i = ui.page;
    if (ui.home || page_over || ui.confirm || bm_on || i >= NPAGES || !page_visible(i))
        return;
    ui.fam_last[PAGES[i].fam] = (uint8_t)i;
}
static void nav_swap(void)
{
    uint32_t f;
    for (f = 0; f < FAM_COUNT; f++) {
        uint8_t t = ui.fam_last[f];
        ui.fam_last[f] = nav_side[f];
        nav_side[f] = t;
    }
}

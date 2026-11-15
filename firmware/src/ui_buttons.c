/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Blair (blairysbeatlab), BBL JAM BOX; based on Felucca by Leo Kuroshita (@kurogedelic), Hügelton Instruments */

static const uint8_t LY_BTN[LAYER_N] = {0, B_EDIT, B_GLO, B_SCL, B_FX, B_SEQ, B_REC, B_ENV};
#define LY_DEAD 8u

static int layer_allowed(void) { return !ui.confirm && !ui.uboot; }
static uint32_t ly_bit(uint32_t l) { return 1u << panel.btn[LY_BTN[l % LAYER_N]]; }

static void layer_arm(uint32_t pressed, uint32_t now)
{
    uint32_t l;
    if (ui.ly)
        return;
    for (l = LAYER_FX; l < LAYER_N; l++)
        if (pressed & ly_bit(l)) {
            ui.ly = (uint8_t)l;
            ui.ly_t0 = (now & ~15u) | 1u | (layer_allowed() ? 0u : LY_DEAD);
            return;
        }
}

static uint32_t layer_gesture(void)
{
    uint32_t l = ui.ly;
    if (!l)
        return 0;
    if (!layer_allowed())
        ui.ly_t0 |= LY_DEAD;
    if ((fm1_in.buttons & ly_bit(l)) != 0u)
        return 0;
    l = ui.ly_t0 & LY_DEAD ? 0u : l;
    ui.ly_t0 = 0;
    ui.ly = 0;
    return l;
}

static void layer_kill(void)
{
    if (ui.ly)
        ui.ly_t0 |= LY_DEAD;
}

static void layer_tap(uint32_t l)
{
    if (l == LAYER_REC)
        (void)rec_tap();
    else
        (void)page_tap(LY_BTN[l % LAYER_N]);
}

static void usb_serial_apply(void)
{
#if FELUCCA_CDC
    usb_cdc_switch(FELUCCA_CDC_DEFAULT);
#endif
}

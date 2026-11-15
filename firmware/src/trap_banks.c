/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION OS contributors */

#include "trap_bank_recipes.h"
static void trap_bank_load(track_t *t,uint32_t bank,uint32_t slot)
{
    if(bank>=BANK_EXTRA_COUNT || slot>=BANK_EXTRA_N)return;
    const ba_sound_t *s=&BANK_EXTRA[bank][slot];
    trap_sound_load(t,s,str_eq(s->machine,"8BAS"));
    bank_extra_pick[bank][trk_index(t)]=(int16_t)slot;
}

/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION OS contributors */

#define BANK_A_N 8u
#define BA_NP 32u
typedef struct { uint8_t id; int16_t v; } ba_param_t;
typedef struct {
    const char *name, *machine;
    ba_param_t p[BA_NP];
} ba_sound_t;
#define BA_END {P_COUNT, 0}
static const ba_sound_t BANK_A[BANK_A_N] = {
    {"MEMPHIS BELL", "FMSYN", {
        {P_E0,5},{P_E1,9},{P_E2,14},{P_E3,17},{P_E4,70},{P_E5,50},{P_E6,40},{P_ATK,0},{P_DEC,90},{P_SUS,0},{P_REL,80},{P_FM3_ATK,0},{P_FM3_DEC,30},{P_FM4_ATK,0},{P_FM4_DEC,25},{P_FM3_LEVEL,40},{P_FM4_LEVEL,0},{P_FLT_CUT,105},{P_FLT_RES,10},{P_FLT_DRV,5},{P_SPRD,30},BA_END}},
    {"RUST BELL", "FMSYN", {
        {P_E0,2},{P_E1,15},{P_E2,7},{P_E3,9},{P_E4,90},{P_E5,60},{P_E6,70},{P_ATK,0},{P_DEC,60},{P_SUS,0},{P_REL,50},{P_FM3_ATK,0},{P_FM3_DEC,20},{P_FM4_ATK,0},{P_FM4_DEC,12},{P_FM3_LEVEL,80},{P_FM4_LEVEL,30},{P_FLT_CUT,80},{P_FLT_RES,25},{P_FLT_DRV,40},{P_SPRD,15},BA_END}},
    {"TAPE ORGAN", "FMSYN", {
        {P_E0,7},{P_E1,10},{P_E2,5},{P_E3,3},{P_E4,20},{P_E5,10},{P_E6,50},{P_ATK,2},{P_DEC,20},{P_SUS,120},{P_REL,30},{P_FM3_ATK,0},{P_FM3_DEC,127},{P_FM4_ATK,0},{P_FM4_DEC,127},{P_FM3_LEVEL,0},{P_FM4_LEVEL,12},{P_FLT_CUT,100},{P_FLT_RES,6},{P_FLT_DRV,15},{P_SPRD,40},{P_CHOR,30},BA_END}},
    {"DARK ORGAN", "FMSYN", {
        {P_E0,4},{P_E1,17},{P_E2,3},{P_E3,6},{P_E4,60},{P_E5,40},{P_E6,60},{P_ATK,6},{P_DEC,40},{P_SUS,110},{P_REL,60},{P_FM3_ATK,3},{P_FM3_DEC,70},{P_FM4_ATK,3},{P_FM4_DEC,70},{P_FM3_LEVEL,20},{P_FM4_LEVEL,18},{P_FLT_CUT,70},{P_FLT_RES,25},{P_FLT_DRV,30},{P_SPRD,60},BA_END}},
    {"HORROR PAD", "FMSYN", {
        {P_E0,6},{P_E1,9},{P_E2,4},{P_E3,2},{P_E4,58},{P_E5,18},{P_E6,70},
        {P_ATK,80},{P_DEC,95},{P_SUS,95},{P_REL,90},
        {P_FM3_ATK,45},{P_FM3_DEC,127},{P_FM4_ATK,60},{P_FM4_DEC,127},
        {P_FM3_LEVEL,35},{P_FM4_LEVEL,25},{P_FLT_CUT,80},{P_FLT_RES,20},
        {P_FLT_DRV,8},{P_SPRD,80}, BA_END}},
    {"GHOST PAD", "FMSYN", {
        {P_E0,4},{P_E1,10},{P_E2,2},{P_E3,8},{P_E4,35},{P_E5,25},{P_E6,60},
        {P_ATK,100},{P_DEC,100},{P_SUS,105},{P_REL,105},
        {P_FM3_ATK,70},{P_FM3_DEC,127},{P_FM4_ATK,90},{P_FM4_DEC,127},
        {P_FM3_LEVEL,15},{P_FM4_LEVEL,45},{P_FLT_CUT,72},{P_FLT_RES,12},
        {P_FLT_DRV,5},{P_SPRD,100}, BA_END}},
    {"DARK LEAD", "FMSYN", {
        {P_E0,1},{P_E1,10},{P_E2,3},{P_E3,5},{P_E4,35},{P_E5,20},{P_E6,0},{P_ATK,4},{P_DEC,60},{P_SUS,100},{P_REL,45},{P_FM3_ATK,0},{P_FM3_DEC,40},{P_FM4_ATK,0},{P_FM4_DEC,30},{P_FM3_LEVEL,45},{P_FM4_LEVEL,8},{P_FLT_CUT,75},{P_FLT_RES,40},{P_FLT_DRV,15},{P_SPRD,20},BA_END}},
    {"MELODIC 808", "8BAS", {

        {P_E0,64},{P_E1,100},{P_E2,3},{P_E3,45},{P_E4,10},{P_E5,15},{P_E6,70},
        {P_LN0,100},{P_LN1,0},{P_DIST,65},{P_GLIDE,35},{P_TRANS,0}, BA_END}},
};
_Static_assert(NELEM(BANK_A)==NELEM(N_BANKA), "bank picker matches the recipes");

static void trap_sound_load(track_t *t, const ba_sound_t *s, int melodic808)
{
    uint32_t m, i;
    for (m=0; m<NMACH && !str_eq(MACHINES[m].pill,s->machine); m++) ;
    if (m==NMACH) return;
    xf_leave(trk_index(t));
    load_begin(t,UNDO_SOUND);
    fm1_irq_off();
    panic_req |= (uint8_t)(1u<<trk_index(t));
    for (i=0;i<P_E0;i++) if (!param_kept(i)) t->p[i]=TP[i].def;
    t->eng_req=MACHINES[m].engine;
    t->p[P_E7]=MACHINES[m].sub;
    machine_defaults(t);
    t->p[P_TRANS]=0;
    for (i=0;i<BA_NP && s->p[i].id<P_COUNT;i++)
        t->p[s->p[i].id]=s->p[i].v;
#if TRAP_FM6_TEST
    for(i=0;i<TF6_COUNT;i++) if(str_eq(s->name,TF6_ORIGINAL[i])) {
        t->eng_req=ENGI_FM6;
        t->p[P_E7]=0;
        t->p[P_VOICE]=V_POLY;
        t->p[P_SPRD]=0;
        t->p[P_DIST]=t->p[P_CHOR]=t->p[P_DLY]=t->p[P_REV]=0;
        tf6_load(trk_index(t),i);
        break;
    }
#endif
    if (melodic808) {
        if (!t->p[P_SCALE]) t->p[P_SCALE]=2;
        if (!t->p[P_QUANT]) t->p[P_QUANT]=3;
    }
    fm1_irq_on();
    load_end(t);
    ui.force=1;
    ui_message(s->name);
}

static void trap_bank_a_load(track_t *t,uint32_t slot)
{
    if(slot>=BANK_A_N)return;
    trap_sound_load(t,&BANK_A[slot],slot==7u);
    bank_a_pick[trk_index(t)]=(int16_t)slot;
}

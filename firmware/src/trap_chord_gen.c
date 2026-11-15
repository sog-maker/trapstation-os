/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION OS contributors */

static void trap_chord_generate(track_t *t)
{
    uint32_t k=trk_index(t),root,mode;
    if(gen_fresh[k])gen_var[k]=(int16_t)(gen_var[k]%20+1);
    gen_fresh[k]=1;
    t->p[P_SCALE]=(int16_t)GEN_SCL[chord_cfg[k][CG_STYLE]%8];
    gen_recipe_t r={1,(uint8_t)chord_cfg[k][CG_STYLE],(uint8_t)chord_cfg[k][CG_RHYTHM],(uint8_t)(gen_var[k]-1),
      (int8_t)chord_cfg[k][CG_OCT],(uint8_t)t->p[P_ROOT],(uint8_t)t->p[P_SCALE],(uint8_t)t->p[P_CHRD],
      (uint8_t)t->p[P_VOIC],(uint8_t)trk_vmode(t),(uint8_t)clamp(t->p[P_SLEN],1,NSTEP),GEN_V2};
    for(uint32_t i=0;i<NSTEP;i++){
        step_t *s=&t->step[i];step_clear(s);if(i>=r.len)continue;
        s->n=(uint8_t)gen_original(&r,i,s->note,&root,&mode);
        s->time=s->n?ST_NOTE:r.rhythm==3?ST_REST:ST_TIE;
        if(s->n)s->vel=(uint8_t)(92u+(i%4u==0?8u:0u)-r.variation%4u);
    }
    gen_recipe[k]=r;
    lastc[k]=t->step[0].n;memcpy(lastn[k],t->step[0].note,4);
    gen_undo_ready=(uint8_t)(k+1u);
}
static void trap_euclid_generate(track_t *t)
{
    uint32_t k=trk_index(t),len=(uint32_t)clamp(eu_cfg[k][0],1,NSTEP);
    uint32_t hits=(uint32_t)clamp(eu_cfg[k][1],0,len),rot=(uint32_t)eu_cfg[k][2]%len;
    for(uint32_t i=0;i<NSTEP;i++){
        step_t *s=&t->step[i];step_clear(s);
        if(i>=len || (((i+len-rot)%len)*hits)%len>=hits)continue;
        s->time=ST_NOTE;s->n=chord_kit(t)||trk_vmode(t)!=V_POLY?1:lastc[k]?lastc[k]:1;
        if(s->n>4)s->n=4;
        for(uint32_t j=0;j<s->n;j++)s->note[j]=lastc[k]?lastn[k][j]:60;
        s->vel=96;
    }
    t->p[P_SLEN]=(int16_t)len;gen_recipe[k].active=0;gen_undo_ready=(uint8_t)(k+1u);
}

static int trap_gen_undo(track_t *t)
{
    uint32_t k=trk_index(t);
    if(gen_undo_ready!=k+1u || undo.trk!=k+1u || undo.what!=UNDO_PAT || track_sig(t)!=undo.after)return 0;
    if(motion.count-motion_count(t)+undo.motion_backup.count>MOTION_MAX)return 0;
    fm1_irq_off();
    motion_reset(t);
    memcpy(t->step,undo.step,sizeof undo.step);t->p[P_SLEN]=undo.p[P_SLEN];t->p[P_SCALE]=undo.p[P_SCALE];
    gen_recipe[k]=undo.gen;lastc[k]=undo.last_n;memcpy(lastn[k],undo.last_note,4);
    memcpy(motion.event+motion.count,undo.motion_backup.event,undo.motion_backup.count*sizeof motion.event[0]);
    motion.count=(uint8_t)(motion.count+undo.motion_backup.count);
    if(undo.motion_backup.on)motion.on|=(uint8_t)(1u<<k);
    motion_rebase(t);fm1_irq_on();gen_undo_ready=0;undo.keep=0;return 1;
}

/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION OS contributors */

static int ux_undo_apply(void)
{
    track_t *t=TSEL;
    uint32_t k=trk_index(t),i,n=0,bit=1u<<k;
    if(ux_edit.vp) {
        int16_t *vp=ux_edit.vp;
        if(ux_edit.tr!=k || *vp!=ux_edit.after) { ux_edit.vp=0;return 0; }
        fm1_irq_off();*vp=ux_edit.before;fm1_irq_on();ux_edit.vp=0;
#if TRAP_FM6_TEST
        tf6_ui_commit(vp);
#endif
        if((uintptr_t)vp>=(uintptr_t)t->p && (uintptr_t)vp<(uintptr_t)(t->p+P_COUNT)) motion_capture(t,(uint32_t)(vp-t->p),*vp);
        else if((uintptr_t)vp>=(uintptr_t)song.g && (uintptr_t)vp<(uintptr_t)(song.g+G_COUNT)) g_capture(vp);
        ui.force=1;return 1;
    }
    if(!undo.keep || undo.trk!=k+1u ||
       (undo.after!=track_sig(t) && undo.after!=edit_sig(t))) return 0;
    for(i=0;i<motion.count;i++) n+=(motion.event[i].place>>6)!=k;
    if(n+undo.motion_backup.count>MOTION_MAX) return 0;
    xf_leave(k);fm1_irq_off();
    if(undo.what&UNDO_SOUND) {
        panic_req|=(uint8_t)bit;
        t->eng_req=undo.eng;t->preset=undo.preset;t->user=undo.user;
        memcpy(t->p,undo.p,sizeof undo.p);
#if TRAP_FM6_TEST
        memcpy(tf6_edit[k],undo.fm6,sizeof undo.fm6);tf6_ready[k]=1;tf6[k].valid=0;
#endif
    }
    if(undo.what&UNDO_PAT) {
        memcpy(t->step,undo.step,sizeof undo.step);
        gen_recipe[k]=undo.gen;lastc[k]=undo.last_n;memcpy(lastn[k],undo.last_note,4);
        pat_sig[k]=undo.pat;pat_from[k]=undo.from;pat_from_h[k]=undo.from_h;pat_last[k]=undo.patn;
    }
    for(i=n=0;i<motion.count;i++)
        if((motion.event[i].place>>6)!=k) motion.event[n++]=motion.event[i];
    for(i=0;i<undo.motion_backup.count;i++) motion.event[n++]=undo.motion_backup.event[i];
    motion.count=(uint8_t)n;
    motion.on=(uint8_t)((motion.on&~bit)|(undo.motion_backup.on&bit));
    undo.keep=0;fm1_irq_on();motion_rebase(t);ui.force=1;return 1;
}

static uint32_t ux_undo_button(uint32_t pressed)
{
    static uint32_t down_at,last_up;
    static uint8_t held,armed,track;
    static int8_t octave;
    uint32_t mask=1u<<panel.btn[B_OCTDN],now=fm1_ms;
    int blocked=ui.confirm || ui.uboot || bm_on || tm_dn || fm1_in.notes || (fm1_in.buttons&~mask);
    if(blocked) { held=armed=0;return pressed; }
    if(pressed&mask) {
        if(armed && track==song.sel && now-last_up<=300u) {
            armed=held=0;cla_st=0;song.octave=octave;
            ui_message(ux_undo_apply()?"UNDO":"NOTHING TO UNDO");
            return pressed&~mask;
        }
        armed=0;held=1;down_at=now;octave=(int8_t)song.octave;track=song.sel;
    }
    if(held && !(fm1_in.buttons&mask)) {
        held=0;armed=(uint8_t)(now-down_at<=250u);last_up=now;
    }
    return pressed;
}

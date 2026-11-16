/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION OS contributors
 * The FM6 engine of TRAPSTATION OS: the msfa core (fm6_core.c) on a track. Origins: CREDITS.md. */
#include "fm6_core.c"
#include "fm6_format.c"
#include "trap_fm6_factory.h"

static int16_t tf6_edit[NTRK][158] __attribute__((section(".pool")));

#define TF6_VOICES 16u

static fm6_note_t tf6_notes[TF6_VOICES] __attribute__((section(".noinit")));
static uint8_t tf6_owner[TF6_VOICES], tf6_vi[TF6_VOICES], tf6_used[TF6_VOICES];
static uint32_t tf6_age[TF6_VOICES], tf6_clock;
static fm6_note_t *tf6_note_of(track_t *t, voice_t *v) {
    uint32_t tr=(uint32_t)(t-trk),vi=(uint32_t)(v-t->v),i;
    for(i=0;i<TF6_VOICES;i++) if(tf6_used[i] && tf6_owner[i]==tr && tf6_vi[i]==vi) return &tf6_notes[i];
    return 0;
}
static fm6_note_t *tf6_allocate(track_t *t,voice_t *v) {
    uint32_t i,old=0;
    for(i=0;i<TF6_VOICES;i++) {
        if(!tf6_used[i] || !trk[tf6_owner[i]].v[tf6_vi[i]].active || trk[tf6_owner[i]].engine!=ENGI_FM6) { old=i; break; }
        if(tf6_age[i]<tf6_age[old]) old=i;
    }
    if(tf6_used[old] && trk[tf6_owner[old]].engine==ENGI_FM6) {
        voice_t *prev=&trk[tf6_owner[old]].v[tf6_vi[old]];
        prev->active=prev->gate=0;
    }
    tf6_used[old]=1;tf6_owner[old]=(uint8_t)(t-trk);tf6_vi[old]=(uint8_t)(v-t->v);tf6_age[old]=++tf6_clock;
    return &tf6_notes[old];
}
static uint8_t tf6_ready[NTRK];
static struct {
    uint8_t p[156], valid;
    fm6_lfo_t lfo;
    int32_t lv, ld, low, post;
} tf6[NTRK];

static void tf6_load(uint32_t tr, uint32_t slot)
{
    uint32_t i;
    tr %= NTRK; slot %= TF6_COUNT;
    uint8_t patch[156];
    fm6_unpack(TF6_PACKED[slot],patch);
    for (i=0;i<155u;i++) tf6_edit[tr][i]=patch[i];
    for (i=0;i<3u;i++) tf6_edit[tr][155+i]=TF6_FX[slot][i];
    tf6_ready[tr]=1;
    tf6[tr].valid=0;
    tf6[tr].low=tf6[tr].post=0;
}

static uint32_t tf6_slot_of(uint32_t tr)
{
    uint32_t s, i;
    tr %= NTRK;
    for (s=0;s<TF6_COUNT;s++) {
        for (i=145u;i<155u && tf6_edit[tr][i]==TF6_PATCH[s][i];i++) ;
        if (i==155u) return s;
    }
    return TF6_COUNT;
}

static void tf6_sync(track_t *t)
{
    uint32_t tr=(uint32_t)(t-trk), i, j;
    uint8_t p[156];
    if (!tf6_ready[tr]) tf6_load(tr,0);
    for(i=0;i<155u;i++) p[i]=(uint8_t)clamp(tf6_edit[tr][i],0,fm6_max(i));
    fm6_sanitize(p);
    if(tf6[tr].valid && !memcmp(p,tf6[tr].p,155)) return;
    if(!tf6[tr].valid || memcmp(p+FP_LFS,tf6[tr].p+FP_LFS,7))
        fm6_lfo_reset(&tf6[tr].lfo,p);
    memcpy(tf6[tr].p,p,156);
    for(i=0;i<6u;i++) {
        fm6_note_t *n=tf6_note_of(t,&t->v[i]);
        voice_t *v=&t->v[i];
        int32_t note=clamp((int32_t)v->note+p[FP_TRNSP]-24,0,127);
        if(!n || !v->active || !n->live) continue;
        n->fixed=0;
        for(j=0;j<6u;j++) {
            fm6_env_t e;
            const uint8_t *op=p+j*21u;
            fm6_op_env_init(&e,op,note,v->mvel?v->mvel:v->vel);
            n->env[j].outlevel=e.outlevel;
            n->env[j].rscale=e.rscale;
            fm6_env_advance(&n->env[j],op,n->env[j].ix);
            n->base[j]=fm6_osc_base(op,note,0);
            n->fixed|=(uint8_t)((op[FP_MODE]&1u)<<j);
        }
        fm6_penv_advance(&n->penv,p,n->penv.ix);
    }
    tf6[tr].valid=1;
}
static void tf6_on(track_t *t, voice_t *v)
{
    uint32_t tr=(uint32_t)(t-trk);
    const uint8_t *p;
    tf6_sync(t); p=tf6[tr].p;
    fm6_note_t *n=tf6_note_of(t,v);
    if(!n) n=tf6_allocate(t,v);
    fm6_note_init(n,p,(int32_t)v->note+p[FP_TRNSP]-24,
                  v->mvel?v->mvel:v->vel,1);
    fm6_lfo_key(&tf6[tr].lfo);
}
static void tf6_block(track_t *t)
{
    uint32_t tr=(uint32_t)(t-trk);
    tf6_sync(t);
    tf6[tr].lv=fm6_lfo_sample(&tf6[tr].lfo);
    tf6[tr].ld=fm6_lfo_delay(&tf6[tr].lfo);
}
static int tf6_done(track_t *t, voice_t *v)
{
    uint32_t tr=(uint32_t)(t-trk);
    fm6_note_t *n=tf6_note_of(t,v);
    return !n || !n->live || fm6_note_done(n,tf6[tr].p,tf6[tr].p[FP_ALG]);
}
static void tf6_render(track_t *t,voice_t *v,int32_t *out,uint32_t len,const vmod_t *m)
{
    uint32_t tr=(uint32_t)(t-trk),i;
    fm6_note_t *n=tf6_note_of(t,v);
    const uint8_t *p=tf6[tr].p;
    int32_t bus[FM6_N], dt[6]={0}, lf;
    if(!n || len!=FM6_N) return;
    if(!v->gate && n->down) fm6_note_key(n,p,0);
    lf=fm6_note_logfreq(m->pitch16+(p[FP_TRNSP]-24)*16,m->fine);
    if(!fm6_note_compute(n,p,bus,tf6[tr].lv,tf6[tr].ld,lf,p[FP_ALG],p[FP_FB],dt,0)) return;
    for(i=0;i<len;i++) {
        int32_t x=bus[i]-n->dc;
        int32_t a=m->amp0+((m->amp1-m->amp0)*(int32_t)(i+1)>>5);
        n->dc+=x>>10;

        out[i]+=(int32_t)(((int64_t)x*a)/(167772160LL));
    }
}
static void tf6_post(track_t *t,int32_t *out,uint32_t len)
{
    uint32_t tr=(uint32_t)(t-trk),i;
    int32_t drive=clamp(tf6_edit[tr][155],0,100);
    int32_t hz=clamp(tf6_edit[tr][156],40,20000);
    int32_t level=clamp(tf6_edit[tr][157],0,100);

    int32_t w=(hz*6434)>>10, a=((w<<14)/(44100+w))*2;
    for(i=0;i<len;i++) {
        int32_t x,sat,y;
        tf6[tr].low+=(int32_t)(((int64_t)(out[i]-tf6[tr].low)*a)>>15);
        x=(int32_t)(((int64_t)tf6[tr].low*(100+drive*18))/100);
        sat=32768-1073741824/(32768+(x<0?-x:x));
        if(x<0) sat=-sat;
        y=(int32_t)(((int64_t)tf6[tr].low*(100-drive)+(int64_t)sat*drive)/100);
        tf6[tr].post+=(int32_t)(((int64_t)(y-tf6[tr].post)*a)>>15);
        out[i]=tf6[tr].post*level/100;
    }
}
static const preset_t TF6_PRESETS[]={{"MEMPHIS BELL",{0},{0,0,127,0},0,0}};
static const engine_t ENG_TRAP_FM6={
    .name="FM6", .page_title={"FM6","OPERATOR"},
    .presets=TF6_PRESETS,.npresets=1,.poly=6,.ownenv=1,
    .note_on=tf6_on,.block=tf6_block,.done=tf6_done,.render=tf6_render,.post=tf6_post
};

/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION OS contributors */

#include "trap_tape_tables.h"
#define TAPE_RING 512u
_Static_assert(FS==44100u, "regenerate Tape/VHS coefficients for a different sample rate");
_Static_assert(2u*(127u*TAPE_WOW_K/300u+127u*TAPE_FLUT_K/6000u)<(TAPE_RING-2u)*256u, "Tape delay fits at maximum depth / lowest rates");
static int32_t tape_buf[2][TAPE_RING] __attribute__((section(".pool")));
static struct {
    uint32_t w, wp, fp, warm;
    int32_t mix, gate, wd, fd, alpha, sat, hiss, wear, jitter, drive, makeup, smix;
    int32_t lp[2], hisslp[2], pop[2], clicklp[2];
    int32_t seed;
    uint16_t cluster, next;
    uint8_t live;
} tape;

static void tape_reset(void)
{
    memset(&tape,0,sizeof tape);
    tape.seed=(int32_t)0x6d2b79f5u;
    tape.alpha=32768;
    tape.drive=4096;tape.makeup=32768;
}
static int32_t tape_interp(const int32_t *buf,uint32_t w,int32_t delay)
{
    uint32_t whole=(uint32_t)delay>>8, f=(uint32_t)delay&255u;
    int32_t a=buf[(w-whole)&(TAPE_RING-1u)], b=buf[(w-whole-1u)&(TAPE_RING-1u)];
    return a+(b-a)*(int32_t)f/256;
}
static int32_t tape_shape(int32_t x,int32_t drive,int32_t makeup,int32_t amount)
{
    int32_t y=softclip((int32_t)(((int64_t)x*drive)>>12));
    y=(int32_t)(((int64_t)y*makeup)>>15);
    return x+(int32_t)(((int64_t)y-x)*amount/32768);
}

static __attribute__((noinline)) void tape_block(int32_t *bl,int32_t *br,uint32_t n,int32_t ngl,int32_t ngr)
{
    static const uint32_t WMHZ[5]={300,500,800,1000,1500}, FMHZ[4]={6000,8000,10000,12000};
    int32_t c[TF_COUNT], age, sat, wow, flut, hiss, wear, cut, want, gate;
    uint32_t i,o,wi,fi,wt,ft, peak=0;
    for(i=0;i<TF_COUNT;i++) c[i]=clamp(tape_cfg[i],TF_D[i].min,TF_D[i].max);
    age=c[TF_AGE]; sat=c[TF_SAT]>age?c[TF_SAT]:age;
    wow=c[TF_WOW]>age*2/3?c[TF_WOW]:age*2/3;
    flut=c[TF_FLUT]>age/3?c[TF_FLUT]:age/3;
    hiss=c[TF_HISS]>age/2?c[TF_HISS]:age/2;
    wear=c[TF_WEAR]; cut=c[TF_CUT]*256;
    if(7*256-age*6*256/127<cut) cut=7*256-age*6*256/127;
    if(7*256-wear*4*256/127<cut) cut=7*256-wear*4*256/127;
    want=c[TF_ON] && (sat||wow||flut||hiss||wear||cut<7*256||c[TF_DUST]||c[TF_CRKL]);
    if(!want && !tape.mix) { tape.live=0; return; }
    if(!tape.seed) tape_reset();
    if(!tape.live) { tape.warm=TAPE_RING; tape.live=1; }
    wi=(uint32_t)(((uint64_t)WMHZ[c[TF_WRATE]]<<32)/(FS*1000u));
    fi=(uint32_t)(((uint64_t)FMHZ[c[TF_FRATE]]<<32)/(FS*1000u));
    wt=(uint32_t)wow*TAPE_WOW_K/WMHZ[c[TF_WRATE]];
    ft=(uint32_t)flut*TAPE_FLUT_K/FMHZ[c[TF_FRATE]];
    for(i=0;i<n;i++) {
        int32_t a=clamp(bl[i],-1048576,1048576),b=clamp(br[i],-1048576,1048576);
        uint32_t p=(uint32_t)(a<0?-a:a),q=(uint32_t)(b<0?-b:b);
        if(p>peak)peak=p; if(q>peak)peak=q;
    }
    gate=(song.playing||peak>32u)?32768:0;
    for(o=0;o<n;o+=32u) {
        uint32_t end=n-o>32u?o+32u:n;
        int32_t ci=cut>>8,cf=cut&255,alpha,drive,makeup,sm,hg;
        tape.sat+=clamp(sat*256-tape.sat,-512,512);
        tape.hiss+=clamp(hiss*256-tape.hiss,-512,512);
        tape.wear+=clamp(wear*256-tape.wear,-512,512);
        alpha=TAPE_LP[ci]+((TAPE_LP[ci<7?ci+1:7]-TAPE_LP[ci])*cf/256);
        drive=4096+(tape.sat*12288)/(127*256);
        makeup=(8192*32768)/softclip(8192*drive/4096);
        sm=tape.sat*258/256;
        ci=tape.hiss>>8;cf=tape.hiss&255;
        hg=TAPE_HISS[ci]+((TAPE_HISS[ci<127?ci+1:127]-TAPE_HISS[ci])*cf/256);
        tape.jitter+=((int32_t)noise32(&tape.seed)/65536-tape.jitter)/32;
        uint32_t f_inc=fi+(int32_t)((int64_t)fi*tape.jitter/655360);
        for(i=o;i<end;i++) {
            int32_t x[2]={clamp(bl[i],-1048576,1048576),clamp(br[i],-1048576,1048576)};
            int32_t ws=sine_i(tape.wp),fs=sine_i(tape.fp),delay,ww,gn[2]={ngl,ngr};
            uint32_t rnd=noise32(&tape.seed);
            tape.wp+=wi;tape.fp+=f_inc;
            tape.wd+=clamp((int32_t)wt-tape.wd,-1,1);
            tape.fd+=clamp((int32_t)ft-tape.fd,-1,1);
            tape.alpha+=clamp(alpha-tape.alpha,-16,16);
            tape.gate+=clamp(gate-tape.gate,-8,8);
            tape.drive+=clamp(drive-tape.drive,-8,8);
            tape.makeup+=clamp(makeup-tape.makeup,-32,32);
            tape.smix+=clamp(sm-tape.smix,-32,32);
            if(tape.warm) tape.warm--;
            else tape.mix+=clamp((want?32768:0)-tape.mix,-32,32);
            delay=tape.wd+(int32_t)(((int64_t)tape.wd*ws)>>15)+
                  tape.fd+(int32_t)(((int64_t)tape.fd*fs)>>15);
            delay=clamp(delay,0,(TAPE_RING-2u)*256u);
            ww=32768-((tape.wear/256)*(32768+ws)/8192);
            if(tape.gate>64 && c[TF_DUST] && rnd<(uint32_t)c[TF_DUST]*30665u) {
                int32_t a=(300+(int32_t)((rnd>>12)&1023u))*c[TF_DUST]/127;
                tape.pop[0]+=(rnd&1u)?a:-a;tape.pop[1]+=(rnd&2u)?a:-a;
            }
            if(tape.gate>64 && c[TF_CRKL] && rnd>(0xffffffffu-(uint32_t)c[TF_CRKL]*9199u) && !tape.cluster)
                tape.cluster=(uint16_t)(2u+((rnd>>8)&3u));
            if(tape.next)tape.next--;
            if(tape.cluster && !tape.next) {
                int32_t a=(700+(int32_t)((rnd>>16)&2047u))*c[TF_CRKL]/127;
                tape.pop[0]+=(rnd&4u)?a:-a;tape.pop[1]+=(rnd&8u)?a:-a;
                tape.cluster--;tape.next=(uint16_t)(80u+((rnd>>10)&127u));
            }

            for(uint32_t ch=0;ch<2u;ch++) {
                int32_t y,noise=0; uint32_t q=ch?(rnd<<16|rnd>>16):rnd;
                tape_buf[ch][tape.w]=x[ch];
                y=delay?tape_interp(tape_buf[ch],tape.w,delay):x[ch];
                if(tape.smix) y=tape_shape(y,tape.drive,tape.makeup,tape.smix);
                if(tape.alpha<32768) {
                    tape.lp[ch]+=(int32_t)(((int64_t)(y-tape.lp[ch])*tape.alpha)>>15);
                    y=tape.lp[ch];
                } else tape.lp[ch]=y;
                if(ww!=32768) y=(int32_t)(((int64_t)y*ww)>>15);
                tape.hisslp[ch]+=((int32_t)(q>>16)-32768-tape.hisslp[ch])/8;
                if(hg) noise=(tape.hisslp[ch]*hg)>>15;
                if(tape.pop[ch]|tape.clicklp[ch]) {
                    tape.pop[ch]-=tape.pop[ch]/128;
                    if(tape.pop[ch]>-128 && tape.pop[ch]<128)tape.pop[ch]=0;
                    tape.clicklp[ch]+=(tape.pop[ch]-tape.clicklp[ch])/8;
                    noise+=tape.pop[ch]-tape.clicklp[ch];
                }
                if(noise) {
                    noise=(int32_t)(((int64_t)noise*tape.gate)>>15);
                    noise=(int32_t)(((int64_t)noise*gn[ch])>>15);
                    y+=noise;
                }
                if(tape.mix!=32768) y=x[ch]+(int32_t)(((int64_t)y-x[ch])*tape.mix/32768);
                if(ch)br[i]=y;else bl[i]=y;
            }
            tape.w=(tape.w+1u)&(TAPE_RING-1u);
        }
    }
}

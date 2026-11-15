/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION OS contributors */

static uint8_t ph_fixed;

typedef struct { const char *rh[2], *pt[2]; const char *alt; } bg_style_t;
static const bg_style_t BG[4]={
  {{"x-------x---x---","x---------x-----"},{"0.......0...0...","0..........L...."},"L508"},
  {{"x---x-------x---","x---x-------x---"},{"0...0.......0...","0...0.......3..."},"3C05"},
  {{"x---------x-----","x-------x---x---"},{"0..........0....","0.......c...0..."},"c00C"},
  {{"x-----x-----x---","x-------x-------"},{"0.....0.....1...","0.......6......."},"1661"},
};
static int32_t bg_semi(char c)
{
    return c>='0'&&c<='9'?c-'0':c>='a'&&c<='c'?c-'a'+10:c>='A'&&c<='L'?-(c-'A'+1):0;
}

static void bg_rows(uint32_t sty,uint32_t v,char rh[2][17],char pt[2][17])
{
    const bg_style_t *g=&BG[sty%4u];
    for(uint32_t b=0;b<2u;b++){
        uint32_t k,h[16],nh=0;
        memcpy(rh[b],g->rh[b],17);memcpy(pt[b],g->pt[b],17);
        for(k=0;k<16u;k++)if(rh[b][k]=='x'||rh[b][k]=='~')h[nh++]=k;
        if((v&1u) && nh>1u && rh[b][h[1]-1u]=='-'){
            k=h[1];rh[b][k-1u]=rh[b][k];pt[b][k-1u]=pt[b][k];rh[b][k]='-';pt[b][k]='.';
        }
        if((v&2u) && b==0u){
            for(k=15;k>0u && rh[b][k]!='x' && rh[b][k]!='~';k--);
            if(k)rh[b][k]='~';
        }
        if(v&4u){
            for(k=15;k>0u && rh[b][k]!='x' && rh[b][k]!='~';k--);
            for(uint32_t j=k+1u;j<16u && rh[b][j]=='-';j++)rh[b][j]='.';
        }
        if(v&8u)
            for(k=0;k<16u;k++){
                if(pt[b][k]==g->alt[0])pt[b][k]=g->alt[1];
                else if(pt[b][k]==g->alt[2])pt[b][k]=g->alt[3];
            }
        for(k=0;k<16u;k++)
            if(rh[b][k]=='~' && (!k || rh[b][k-1u]=='.'))rh[b][k]='x';
    }
}

static uint8_t jz_deg[4]={0,3,1,4};
static void jz_chord(const track_t *t,uint32_t b,uint32_t q,uint32_t *semi,uint32_t *mode)
{
    static const uint8_t PAR[4]={0,5,2,7};
    const uint8_t *DEG=jz_deg;
    static const uint8_t MINL[4][4]={{CH_MIN7},{CH_MIN9,CH_MIN7},{CH_MIN11,CH_MIN9,CH_MIN7},{CH_MIN13,CH_MIN11,CH_MIN9,CH_MIN7}};
    uint32_t mask=SCALE_MASK[clamp(t->p[P_SCALE],0,(int32_t)NELEM(SCALE_MASK)-1)],deg[12],cnt=0,d,j,k;
    if(q>=4u){*semi=PAR[b%4u];*mode=q==4u?CH_MIN7:CH_MIN9;return;}
    if(mask==0xFFFu)mask=SCALE_MASK[2];
    for(j=0;j<12u;j++)if((mask>>j)&1u)deg[cnt++]=j;
    d=DEG[b%4u]%cnt;*semi=deg[d];
    {
        uint32_t third=deg[(d+2u)%cnt]+12u*((d+2u)/cnt)-deg[d],fifth=deg[(d+4u)%cnt]+12u*((d+4u)/cnt)-deg[d];
        uint32_t seventh=deg[(d+6u)%cnt]+12u*((d+6u)/cnt)-deg[d];
        const uint8_t *cand;uint32_t nc;
        static const uint8_t MAJL[2]={CH_MAJ9,CH_MAJ7},DOM[1]={CH_DOM7},HD[1]={CH_HALFDIM};
        if(fifth==6u){cand=HD;nc=1;}
        else if(third==3u){cand=MINL[q];nc=q+1u;}
        else if(seventh==11u){cand=q?MAJL:MAJL+1;nc=q?2u:1u;}
        else {cand=DOM;nc=1;}
        *mode=cand[nc-1u];
        for(k=0;k<nc;k++){
            uint32_t ok=1;
            for(j=0;j<CHORD_MAX && CHORD_SHAPE[cand[k]-CH_MAJ][j]>=0;j++)
                ok&=(mask>>((*semi+(uint32_t)CHORD_SHAPE[cand[k]-CH_MAJ][j])%12u))&1u;
            if(ok){*mode=cand[k];break;}
        }
    }
}

static char bg_enc(int32_t i) { return i>=0&&i<=9?(char)('0'+i):i>=10&&i<=12?(char)('a'+i-10):i<0&&i>=-12?(char)('A'-i-1):'0'; }
static int32_t bg_iv(uint32_t mask)
{
    static const int8_t IV[9]={0,12,3,7,5,8,2,-2,4};
    static const uint8_t W[9]={13,10,3,3,2,2,1,1,1};
    uint8_t at[9],w[9];uint32_t n=0,k;
    for(k=0;k<9u;k++)if((mask>>(uint32_t)((IV[k]%12+12)%12))&1u){at[n]=(uint8_t)k;w[n++]=W[k];}
    return n?IV[tr_pick(at,w,n)]:0;
}

static void bg_bar(uint32_t mask,uint32_t kick,uint32_t bar2,char *rh,char *pt)
{
    static const uint8_t AT[6]={4,6,8,10,12,14},W[6]={11,10,8,9,19,9};
    uint32_t k,n,first=(bar2 && tr_pct(50u))?12u:0u;
    memset(rh,'.',16);memset(pt,'.',16);rh[16]=pt[16]=0;
    rh[first]='x';pt[first]='0';
    if(kick && tr_pct(75u)){
        for(k=1,n=0;k<16u && n<4u;k++)if(((kick>>k)&1u) && k>first){rh[k]='x';pt[k]=bg_enc(bg_iv(mask));n++;}
    } else
        for(n=2u+tr_rnd(2u);n;n--){k=tr_pick(AT,W,6u);if(k>first){rh[k]='x';pt[k]=bg_enc(bg_iv(mask));}}
    for(k=first+1u;k<16u;k++)if(rh[k]=='.')rh[k]='-';
    if(tr_pct(30u) && rh[8]=='-' && rh[9]!='x'){
        for(k=8;k<16u && rh[k]=='-';k++)rh[k]='.';
    }
    if(tr_pct(20u))
        for(k=15;k>first+1u;k--)if(rh[k]=='x' && rh[k-1u]=='-'){rh[k]='~';break;}
    for(k=0;k<16u;k++)
        if(rh[k]=='~' && (!k || rh[k-1u]=='.'))rh[k]='x';
}

static void bg_random(const track_t *t,char rh[4][17],char pt[4][17])
{
    uint32_t mask=SCALE_MASK[clamp(t->p[P_SCALE],0,(int32_t)NELEM(SCALE_MASK)-1)],k,b,kick[4]={0,0,0,0},tries;
    if(mask==0xFFFu)mask=SCALE_MASK[2];
    for(b=0;b<4u;b++)for(k=1;k<16u;k++)
        if(k!=8u && trk[0].step[16u*b+k].time==ST_NOTE && trk[0].step[16u*b+k].n)kick[b]|=1u<<k;
    bg_bar(mask,kick[0],0u,rh[0],pt[0]);
    for(b=1;b<4u;b++)
        for(tries=0;;tries++){
            bg_bar(mask,kick[b],1u,rh[b],pt[b]);
            if(tries>=3u || memcmp(rh[b-1u],rh[b],17) || memcmp(pt[b-1u],pt[b],17))break;
        }
}
static void jz_draw(void)
{
    static const uint8_t PROG[8][4]={{0,3,1,4},{0,5,3,4},{1,4,0,5},{0,3,4,0},{5,3,0,4},{0,6,5,4},{0,2,5,4},{3,4,2,5}};
    memcpy(jz_deg,PROG[tr_rnd(8u)],4);
    ph_jqual=(int16_t)tr_rnd(6u);
    ph_jrhy=(int16_t)tr_rnd(4u);
    ph_oct=0;
}
static void trap_phonk_generate(track_t *t,uint32_t kind)
{
    uint32_t len,i,start;
    char brh[4][17],bpt[4][17];
    uint8_t jzn[4][CHORD_MAX];uint32_t jzc[4]={0};
    if(kind==PH_BMAKE || kind==PH_JMAKE){len=64u;t->p[P_SLEN]=64;}
    else len=(uint32_t)clamp(t->p[P_SLEN],1,NSTEP);
    start=kind==PH_FMAKE?(len>4u?len-4u:0u):0u;
    if(kind==PH_BMAKE){
        if(ph_fixed){
            char r2[2][17],p2[2][17];
            bg_rows((uint32_t)ph_bstyle%4u,(uint32_t)ph_var&15u,r2,p2);
            for(uint32_t bb=0;bb<4u;bb++){memcpy(brh[bb],r2[bb&1u],17);memcpy(bpt[bb],p2[bb&1u],17);}
        } else {ph_oct=0;bg_random(t,brh,bpt);}
    }
    if(kind==PH_JMAKE){
        gen_recipe_t jr={1,0,0,0,(int8_t)ph_oct,(uint8_t)t->p[P_ROOT],(uint8_t)t->p[P_SCALE],0,(uint8_t)t->p[P_VOIC],
                         (uint8_t)trk_vmode(t),(uint8_t)len,GEN_V2};
        for(uint32_t c=0;c<4u;c++){
            uint32_t semi,mode;
            jz_chord(t,c,(uint32_t)ph_jqual%6u,&semi,&mode);
            jzc[c]=gen2_voice(&jr,(uint32_t)clamp(48+t->p[P_ROOT]+12*ph_oct+(int32_t)semi,0,127),mode,c?jzn[c-1u]:0,c?jzc[c-1u]:0,jzn[c]);
        }
    }
    for(i=start;i<NSTEP;i++) {
        step_t *s=&t->step[i];
        if(kind==PH_FMAKE && i>=len) break;
        step_clear(s);
        if(i>=len)continue;
        if(kind==PH_BMAKE) {
            uint32_t b=(i/16u)&3u,k=i%16u;char c=brh[b][k];
            if(c=='.'){s->time=ST_REST;continue;}
            if(c=='-'){s->time=ST_TIE;continue;}
            int32_t pitch=60+t->p[P_ROOT]+12*ph_oct+bg_semi(bpt[b][k]);
            s->time=ST_NOTE;s->n=1;s->note[0]=(uint8_t)clamp(pitch,0,127);s->vel=(uint8_t)(i%4u?100:116);
            if(c=='~' && i)t->step[i-1u].flags|=SF_SLIDE;
        } else if(kind==PH_JMAKE) {
            uint32_t seg=len==64u?16u:8u,b=(i/seg)%4u,pos=i%seg,rhythm=(uint32_t)ph_jrhy%4u;
            int strike=pos==0u || (rhythm==1u && i%2u==0) || (rhythm==2u && (i%4u==0 || i%4u==3)) || (rhythm==3u && i%2u==0);
            if(!strike){s->time=rhythm==3u?ST_REST:ST_TIE;continue;}
            s->time=ST_NOTE;s->n=(uint8_t)jzc[b];memcpy(s->note,jzn[b],CHORD_MAX);s->vel=(uint8_t)(i%4u?92:100);
        } else {

            static const uint8_t CRES[4]={64,80,96,112},BURST[4]={2,2,3,4};
            uint32_t tk=trk_index(t),type=(uint32_t)ph_fill%5u,kk=i-start+4u-(len-start),h;
            if(type==4u && (kk&1u))continue;
            s->time=ST_NOTE;s->n=lastc[tk]?lastc[tk]:1;
            if(s->n>4u)s->n=4;
            for(uint32_t k=0;k<s->n;k++)
                s->note[k]=(uint8_t)clamp((int32_t)(lastc[tk]?lastn[tk][k]:60)+(type==1u?2*(int32_t)kk:0),0,127);
            s->vel=(uint8_t)(type==3u?40u+8u*kk:CRES[kk]);
            h=type==2u?BURST[kk]:type==4u?3u:2u;
            s->flags=(uint8_t)((h-1u)<<SF_RATCH_SH);
            step_set_roll(s,type==1u?1:0,RAMP_FLAT,type!=3u && kk==(type==4u?2u:3u));
            if(type==4u)s->roll|=SR_TRIP;
        }
    }
    if(kind!=PH_FMAKE){uint32_t tk=trk_index(t);lastc[tk]=t->step[0].n;memcpy(lastn[tk],t->step[0].note,4);}
}
static int ph_track_ok(const track_t *t,uint32_t kind)
{
    return !acid_on(t) && (kind==PH_FMAKE || (kind==PH_BMAKE ? an_is_kick(t) || !chord_kit(t) : !chord_kit(t) && trk_vmode(t)==V_POLY));
}

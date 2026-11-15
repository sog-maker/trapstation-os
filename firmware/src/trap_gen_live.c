/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION OS contributors */

static const uint8_t GEN_SCL[8]={2,7,3,2,1,3,2,2};
static const uint8_t GEN_SEV[8]={0,0,1,1,1,1,0,1};
static const uint8_t GEN_PROG[8][5][4]={
  {{0,5,3,4},{0,3,5,4},{0,4,3,5},{0,2,5,4},{0,5,1,4}},
  {{0,0,5,4},{0,4,0,5},{0,5,4,0},{0,2,0,4},{0,3,5,4}},
  {{1,4,0,5},{0,3,1,4},{5,1,4,0},{0,5,1,4},{1,5,0,4}},
  {{0,4,5,3},{0,5,3,4},{0,3,4,5},{5,3,0,4},{0,2,5,3}},
  {{0,2,5,3},{3,2,1,0},{0,5,1,4},{5,3,0,4},{1,4,0,5}},
  {{0,3,0,3},{0,6,2,3},{1,4,0,0},{2,1,0,6},{0,2,3,6}},
  {{0,5,2,6},{0,3,5,4},{0,6,5,6},{5,6,0,4},{5,6,0,0}},
  {{0,3,5,4},{0,5,2,6},{3,4,0,0},{0,6,5,4},{5,4,0,3}}
};

#define GEN_BII 7u
static const uint8_t GEN_PROG2[2][5][4]={
  {{0,5,3,4},{0,GEN_BII,6,0},{0,4,5,6},{0,3,0,4},{0,6,5,4}},
  {{0,0,5,4},{0,4,0,5},{0,3,4,0},{0,GEN_BII,0,4},{0,5,3,4}}
};
static uint32_t gen_scale(const gen_recipe_t *r)
{
    uint32_t m=SCALE_MASK[r->scale];
    if(m!=0xFFFu)return m;
    return r->style<4u?SCALE_MASK[r->style>=2u?1:2]:SCALE_MASK[GEN_SCL[r->style%8u]];
}

static void gen_harmony(const gen_recipe_t *r,uint32_t d,uint32_t b,uint32_t *semi,uint32_t *mode)
{
    uint32_t deg[12],cnt=0,mask=gen_scale(r);
    for(uint32_t j=0;j<12;j++)if((mask>>j)&1u)deg[cnt++]=j;
    if(d==GEN_BII && r->reserved==GEN_V2){*semi=1u;*mode=r->chrd?r->chrd:CH_MAJ;return;}
    d%=cnt;
    *semi=deg[d];
    *mode=r->chrd;
    if(!*mode){
      uint32_t third=deg[(d+2u)%cnt]+12u*((d+2u)/cnt)-deg[d];
      uint32_t fifth=deg[(d+4u)%cnt]+12u*((d+4u)/cnt)-deg[d];
      uint32_t seventh=deg[(d+6u)%cnt]+12u*((d+6u)/cnt)-deg[d];
      *mode=fifth==6u?CH_HALFDIM:third==3u?CH_MIN:CH_MAJ;
      if(GEN_SEV[r->style%8u] && fifth!=6u)*mode=third==3u?(r->style==2u&&b==0?CH_MIN9:CH_MIN7):seventh==11u?CH_MAJ7:CH_DOM7;
    }
}

static uint32_t gen2_seg(const gen_recipe_t *r){return r->len==64u?16u:8u;}
static uint32_t gen2_nch(const gen_recipe_t *r){uint32_t s=gen2_seg(r),n=(r->len+s-1u)/s;return n>4u?4u:n;}
static void gen2_chord(const gen_recipe_t *r,uint32_t c,uint32_t *semi,uint32_t *mode)
{
    uint32_t v=r->variation/4u%5u,d=r->style<2u?GEN_PROG2[r->style][v][c%4u]:GEN_PROG[r->style%8u][v][c%4u];
    gen_harmony(r,d,c%4u,semi,mode);
}

static uint32_t gen2_colour(uint32_t mode,uint32_t which)
{
    switch(mode){
    case CH_MIN: return which==1u?CH_SUS2:CH_MADD9;
    case CH_MAJ: case CH_MAJ7: return which==1u?CH_SUS2:CH_ADD9;
    case CH_MIN7: return which==1u?CH_SUS2:CH_MIN9;
    case CH_DOM7: return which==1u?CH_SUS4:CH_DOM7;
    case CH_MIN9: return which==1u?CH_SUS2:CH_MIN7;
    case CH_HALFDIM: return which==1u?CH_MIN7:CH_MIN;
    default: return mode;
    }
}

static int gen2_step(const gen_recipe_t *r,uint32_t i,uint32_t *root,uint32_t *mode,uint32_t *colour,uint32_t *b)
{
    uint32_t seg=gen2_seg(r),nch=gen2_nch(r),pos=i%seg,local=r->variation%4u,semi;
    uint32_t strike=pos==0u || (r->rhythm==1u && i%2u==0) || (r->rhythm==2u && (i%4u==0||i%4u==3)) || (r->rhythm==3u && i%2u==0);
    *b=(i/seg)%4u;*colour=0;
    if(local==1u && *b==1u && pos==seg/2u){strike=1;*colour=1;}
    if(local==2u && *b==nch-1u && pos==seg/2u){strike=1;*colour=2;}
    if(local==3u && i%8u==7u)strike=1;
    if(!strike)return 0;
    gen2_chord(r,*b,&semi,mode);
    if(*colour && !r->chrd)*mode=gen2_colour(*mode,*colour);
    *root=(uint32_t)clamp(48+r->root+12*r->oct+(int32_t)semi,0,127);
    return 1;
}

static uint32_t gen2_voice(const gen_recipe_t *r,uint32_t root,uint32_t mode,const uint8_t *prev,uint32_t pn,uint8_t *out)
{
    int32_t iv[CHORD_MAX+1],rr=(int32_t)root,o12=12*r->oct,up[CHORD_MAX],best[CHORD_MAX],bc=0x7FFFFFFF,rp;
    uint32_t n,m=0,j,k,inv,pc;uint16_t cm;
    if(r->voic!=VC_CLOSE || r->vmode!=V_POLY)
        return chord_make_cfg(root,out,&rp,&cm,mode,r->voic,gen_scale(r),r->root,r->vmode);
    n=chord_tones_cfg(mode,&rr,iv,gen_scale(r),r->root);
    pc=(uint32_t)((rr%12+12)%12);
    for(j=0;j<n && m<CHORD_MAX-1u;j++){
        int32_t x=n<=3u?iv[j]%12:iv[j];
        if(n>3u && (x==0 || x==7))continue;
        x%=12;
        for(k=0;k<m && up[k]!=x;k++);
        if(k==m)up[m++]=x;
    }
    if(!m)return chord_make_cfg(root,out,&rp,&cm,mode,r->voic,gen_scale(r),r->root,r->vmode);
    for(inv=0;inv<m;inv++)
        for(int32_t oc=48;oc<84;oc+=12){
            int32_t v[CHORD_MAX],c=0,t;
            for(j=0;j<m;j++)v[j]=up[(inv+j)%m]+(inv+j>=m?12:0);
            for(j=1;j<m;j++)for(k=j;k>0 && v[k-1u]>v[k];k--){t=v[k];v[k]=v[k-1u];v[k-1u]=t;}
            for(j=0;j<m;j++)v[j]+=(int32_t)pc+oc+o12;
            if(v[0]<55+o12 || v[m-1u]>75+o12)continue;
            if(!pn)c=v[0]-(60+o12)<0?(60+o12)-v[0]:v[0]-(60+o12);
            else for(j=0;j<m;j++){
                int32_t dm=0x7FFF;
                for(k=1;k<pn;k++){int32_t d=v[j]-(int32_t)prev[k];if(d<0)d=-d;if(d<dm)dm=d;}
                c+=dm;
            }
            if(c<bc){bc=c;for(j=0;j<m;j++)best[j]=v[j];}
        }
    if(bc==0x7FFFFFFF)return chord_make_cfg(root,out,&rp,&cm,mode,r->voic,gen_scale(r),r->root,r->vmode);
    out[0]=(uint8_t)clamp(40+(int32_t)((pc+8u)%12u)+o12,0,127);
    for(j=0;j<m;j++)out[j+1u]=(uint8_t)clamp(best[j],0,127);
    return m+1u;
}

static uint32_t gen2_original(const gen_recipe_t *r,uint32_t i,uint8_t *notes,uint32_t *root,uint32_t *mode)
{
    uint32_t colour,b,c,semi,m,pn=0,n;uint8_t prev[CHORD_MAX];
    if(!gen2_step(r,i,root,mode,&colour,&b))return 0;
    for(c=0;c<=b;c++){
        uint8_t cur[CHORD_MAX];
        gen2_chord(r,c,&semi,&m);
        n=gen2_voice(r,(uint32_t)clamp(48+r->root+12*r->oct+(int32_t)semi,0,127),m,prev,pn,cur);
        memcpy(prev,cur,sizeof prev);pn=n;
    }
    if(!colour){memcpy(notes,prev,CHORD_MAX);return pn;}
    return gen2_voice(r,*root,*mode,prev,pn,notes);
}
static int gen_plan(const gen_recipe_t *r,uint32_t i,uint32_t *root,uint32_t *mode)
{
    if(r->reserved==GEN_V2){uint32_t colour,b;return gen2_step(r,i,root,mode,&colour,&b);}
    uint32_t b=i*4u/r->len,semi;
    uint32_t local=r->variation%4u,prev=i?(i-1u)*4u/r->len:99u;
    uint32_t strike=b!=prev || (r->rhythm==1u && i%2u==0) ||
      (r->rhythm==2u && (i%4u==0||i%4u==3)) || (r->rhythm==3u && i%2u==0);
    if(local==1u && i%8u==7u)strike=1;
    if(local==2u && i%8u==3u)strike=1;
    if(local==3u && i%4u==3u)strike=1;
    if(!strike)return 0;
    gen_harmony(r,GEN_PROG[r->style%8u][r->variation/4u%5u][b],b,&semi,mode);
    int32_t reg=local && b==local?12:0;
    *root=(uint32_t)clamp(60+r->root+12*r->oct+(int32_t)semi+reg,0,127);
    return 1;
}

static uint32_t gen_chord_step(const gen_recipe_t *r,uint32_t b)
{
    if(r->reserved==GEN_V2)return b<gen2_nch(r)?b*gen2_seg(r):NSTEP;
    return b*r->len/4u;
}

static uint32_t gen_chord_at(const gen_recipe_t *r,uint32_t i)
{
    return r->reserved==GEN_V2?(i/gen2_seg(r))%4u:i*4u/r->len;
}
static uint32_t gen_original(const gen_recipe_t *r,uint32_t i,uint8_t *notes,uint32_t *root,uint32_t *mode)
{
    int32_t rp;uint16_t cm;
    if(i>=r->len)return 0;
    if(r->reserved==GEN_V2)return gen2_original(r,i,notes,root,mode);
    if(!gen_plan(r,i,root,mode))return 0;
    return chord_make_cfg(*root,notes,&rp,&cm,*mode,r->voic,SCALE_MASK[r->scale],r->root,r->vmode);
}
static uint32_t gen_live_notes(const track_t *t,const step_t *s,uint32_t i,uint8_t *out)
{
    uint32_t k=trk_index(t),root,mode,n;uint8_t original[4];int32_t rp;uint16_t cm;
    const gen_recipe_t *r=&gen_recipe[k];
    if(!r->active || !gen_live[k] || s->time!=ST_NOTE || chord_kit(t))return 0;
    n=gen_original(r,i,original,&root,&mode);
    if(!n || n!=s->n || memcmp(original,s->note,n))return 0;
    if(r->reserved==GEN_V2 && t->p[P_CHRD]==r->chrd && t->p[P_VOIC]==r->voic){
        memcpy(out,original,n);
        return n;
    }
    mode=t->p[P_CHRD]?(uint32_t)t->p[P_CHRD]:mode;
    return chord_make_as(t,root,out,&rp,&cm,mode);
}

/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION contributors
 * Blueprint-2 UI test integration. All labels/values come from gb_view_t.
 * No boot, DSP, storage or parameter behaviour changes. Uses existing strips/fonts. */
#define SD_RED GB_RGB(198,45,57)
#define SD_WHITE GB_RGB(242,242,242)
#define SD_LIGHT GB_RGB(201,201,201)
#define SD_MID GB_RGB(146,146,146)
#define SD_DARK GB_RGB(36,36,36)
#define SD_PANEL GB_RGB(48,48,48)
#define SD_BLACK GB_RGB(17,17,17)
static uint32_t sd_group(const gb_view_t *v)
{
    const page_t *p=cur_page();
    if(v->stg)return 1;
    if(v->bm||v->sv||v->perf||v->fx||str_eq(v->title,"BANKS"))return 0;
    if(p->fam==FAM_ARP)return 1;
    return p->scope==SC_ENGINE||p->fam==FAM_ENV||p->fam==FAM_LFO||p->fam==FAM_FLT?2:0;
}
static void sd_theme(const gb_view_t *v)
{
    uint32_t g=sd_group(v),i;
    ux.bg=g?SD_BLACK:SD_LIGHT;ux.surf=g?SD_PANEL:SD_WHITE;
    ux.text=g?SD_WHITE:SD_BLACK;ux.theme=SD_RED;ux.accent=SD_DARK;
    ux.mid=g?SD_LIGHT:GB_RGB(82,82,82);ux.dim=SD_MID;ux.line=g?GB_RGB(82,82,82):SD_MID;
    ux.sel=SD_RED;ux.tint=SD_PANEL;ux.ink=SD_BLACK;ux.raise=SD_MID;
    ux.key=SD_MID;ux.lane=SD_PANEL;ux.grid=GB_RGB(68,68,68);ux.rec=SD_RED;
    ux.light=ux.mono=0;ux.gen++;
    for(i=0;i<NTRK;i++)gb_col[i]=i==v->sel?SD_RED:SD_MID;
}
static void sd_header(const gb_view_t *v)
{
    uint32_t g=sd_group(v);char b[20],n[8];const char *tag=v->stg?"D LIVE":g==1?"B MIDI":g==2?"C SYNTH":"A DAW";
    cv_rect(0,0,240,60,g==1?SD_BLACK:SD_LIGHT);
    cv_rect(4,3,232,19,SD_DARK);cv_rect(4,3,3,19,SD_RED);
    b[0]='T';b[1]=(char)('1'+v->sel%NTRK);b[2]=' ';b[3]=0;
    str_cpy(b+3,v->fx?"FX":MACHINES[v->mach%NMACH].pill,sizeof b-3);
    cv_text_on(12,5,&AF_S,b,SD_WHITE,SD_DARK);fmt_int(n,v->bpm);
    cv_text_r(227,5,&AF_S,n,SD_WHITE,SD_DARK);
    if(v->rec)cv_rect(173,8,5,6,SD_RED);
    if(v->play){int32_t j;for(j=0;j<7;j++)cv_rect(182,8+j,j<4?j+1:7-j,1,SD_WHITE);}
    else cv_rect(182,9,6,6,SD_MID);
    cv_text_on(10,27,&AF_M,v->title,g==1?SD_WHITE:SD_BLACK,g==1?SD_BLACK:SD_LIGHT);
    cv_rrect(10,46,73,13,3,SD_RED,g==1?SD_BLACK:SD_LIGHT);
    cv_text_in(10,46,73,&AF_S,tag,SD_WHITE,SD_RED);
    if(!v->stg)cv_text_r(228,46,&AF_S,g==1?"PATTERN LAB":g==2?"INSTRUMENT":"WORKSPACE",g==1?SD_LIGHT:SD_DARK,g==1?SD_BLACK:SD_LIGHT);
}
static void sd_controls(const gb_view_t *v,uint32_t group)
{
    uint32_t k;uint16_t bg=group?SD_BLACK:SD_LIGHT;
    cv_rect(0,170,240,44,bg);
    for(k=0;k<4;k++)if(v->mask&(1u<<k)){
        int32_t x=8+(int32_t)(k%2)*116,y=172+(int32_t)(k/2)*21;
        if(group==1){x=8+(int32_t)k*57;y=172;cv_rrect(x,y,53,39,12,SD_PANEL,bg);
            cv_text_in(x,y,53,&AF_S,v->lab[k],SD_LIGHT,SD_PANEL);
            cv_text_in(x+2,y+16,49,&AF_S,v->val[k],SD_WHITE,SD_PANEL);
            cv_rect(x+8,y+34,37,2,SD_MID);cv_rect(x+8,y+34,tl_r(v,k)*37/1000,2,SD_RED);
        }else{cv_rrect(x,y,108,19,3,SD_WHITE,bg);cv_rect(x+2,y+3,2,13,v->hot==k+1||v->alt[k]?SD_RED:SD_MID);
            cv_free_text(x+7,y+3,&AF_S,v->lab[k],SD_DARK,SD_WHITE,49);
            cv_text_r(x+104,y+3,&AF_S,v->val[k],SD_BLACK,SD_WHITE);
        }
    }
}
static void sd_banks(const gb_view_t *v)
{
    uint32_t i,b=(uint32_t)v->cur[0]%5u,s;int16_t *vp;const param_desc_t *d=page_desc(cur_page(),1,&vp);
    s=(uint32_t)(v->cur[1]-(d?d->min:0))%8u;
    for(i=0;i<5;i++){char n[2]={(char)('A'+i),0};int32_t x=10+(int32_t)i*45;
        cv_rrect(x,63,42,17,3,i==b?SD_RED:SD_PANEL,SD_LIGHT);cv_text_in(x,65,42,&AF_S,n,SD_WHITE,i==b?SD_RED:SD_PANEL);}
    for(i=0;i<8&&d&&d->names;i++){int32_t x=10+(int32_t)(i%4)*56,y=86+(int32_t)(i/4)*39;uint16_t c=i==s?SD_RED:SD_DARK;
        cv_rrect(x,y,52,34,4,c,SD_LIGHT);cv_free_text(x+3,y+11,&AF_S,d->names[i],SD_WHITE,c,46);}
}
static void sd_variation(const gb_view_t *v)
{
    uint32_t i,sel=v->cur[0]>0?(uint32_t)(v->cur[0]-1)%20u:0;
    for(i=0;i<20;i++){char n[8];int32_t x=10+(int32_t)(i%10)*22,y=70+(int32_t)(i/10)*27;uint16_t c=i==sel?SD_RED:SD_PANEL;
        fmt_int(n,i+1);cv_rrect(x,y,20,23,3,c,SD_BLACK);cv_text_in(x,y+4,20,&AF_S,n,SD_WHITE,c);}
    cv_text_in(10,132,220,&AF_S,"VAR / LIVE / UNDO / MAKE",SD_LIGHT,SD_BLACK);
    cv_text_in(10,151,220,&AF_S,"STATIC VARIATION INDEX",SD_MID,SD_BLACK);
}
static void sd_instrument(const gb_view_t *v)
{
    int32_t k,x,py=120;uint32_t env=str_eq(v->title,"ENV")||str_eq(v->title,"FILTER ENV")||str_eq(v->title,"MOD C")||str_eq(v->title,"MOD D");
    cv_rect(6,62,160,105,SD_DARK);
    for(k=1;k<8;k++)cv_rect(6+k*20,62,1,105,SD_PANEL);
    for(k=1;k<5;k++)cv_rect(6,62+k*21,160,1,SD_PANEL);
    if(str_eq(MACHINES[v->mach%NMACH].pill,"FMSYN")&&str_eq(v->title,"OSC 1")){
        static const uint8_t edge[8]={9,5,13,11,15,17,19,25};
        uint32_t e=edge[(uint32_t)v->cur[0]&7u];
        int32_t ax=28,bx=112,ay=132,cx=28,cy=78,dx=112,dy=78,j;
        const char *name[4]={"A","B","C","D"};
        int32_t xx[4],yy[4];
        if(e&16u){cx=69;cy=101;dx=69;dy=68;}
        if(e&1u)cv_line_t(cx+11,cy+19,ax+11,ay,SD_RED,2);
        if(e&2u)cv_line_t(cx+11,cy+19,bx+11,ay,SD_RED,2);
        if(e&4u)cv_line_t(dx+11,dy+19,ax+11,ay,SD_RED,2);
        if(e&8u)cv_line_t(dx+11,dy+19,bx+11,ay,SD_RED,2);
        if(e&16u)cv_line_t(dx+11,dy+19,cx+11,cy,SD_RED,2);
        xx[0]=ax;xx[1]=bx;xx[2]=cx;xx[3]=dx;yy[0]=yy[1]=ay;yy[2]=cy;yy[3]=dy;
        for(j=0;j<4;j++){uint16_t c=j>1?SD_RED:SD_PANEL;cv_rrect(xx[j],yy[j],23,19,3,c,SD_DARK);cv_text_in(xx[j],yy[j]+3,23,&AF_S,name[j],SD_WHITE,c);}
    }else if(env){int32_t a=14+tl_r(v,0)*29/1000,d=a+10+tl_r(v,1)*42/1000,sy=160-tl_r(v,2)*75/1000;
        cv_line_t(10,160,a,70,SD_RED,2);cv_line_t(a,70,d,sy,SD_RED,2);cv_line_t(d,sy,133,sy,SD_RED,2);cv_line_t(133,sy,161,160,SD_RED,2);
    }else if(str_eq(v->title,"ENV DEST")){cv_text_in(8,76,154,&AF_S,v->val[0],SD_WHITE,SD_DARK);cv_line_t(28,111,140,111,SD_RED,2);cv_text_in(8,132,154,&AF_M,v->val[1],SD_WHITE,SD_DARK);
    }else{for(x=0;x<152;x++){int32_t yy,amp=34;
        if(MACHINES[v->mach%NMACH].drum)amp=34*(152-x)/152;
        yy=114-tl_sin((uint32_t)x*(12u+(uint32_t)tl_r(v,0)/80u))*amp/256;
        if(str_eq(v->title,"FILTER")||str_eq(v->title,"COLOR"))yy=154-(x<tl_r(v,0)*152/1000?60:60*(152-x)/152);
        if(x)cv_line_t(10+x-1,py,10+x,yy,SD_RED,1);py=yy;}}
    cv_rect(6,174,160,37,SD_DARK);
    for(k=0;k<16;k++)cv_rect(10+k*9,180,7,24,(v->sq_hit>>k)&1?SD_RED:SD_MID);
    for(k=0;k<4;k++)if(v->mask&(1u<<k)){int32_t y=62+k*38;cv_rrect(173,y,61,35,3,SD_PANEL,SD_BLACK);
        cv_free_text(178,y+2,&AF_S,v->lab[k],SD_LIGHT,SD_PANEL,51);
        cv_free_text(178,y+17,&AF_S,v->val[k],SD_WHITE,SD_PANEL,51);
        cv_rect(178,y+31,51,2,SD_MID);cv_rect(178,y+31,tl_r(v,k)*51/1000,2,SD_RED);}
}
static void sd_pattern(const gb_view_t *v)
{
    uint32_t i;char b[24];
    if(str_eq(v->title,"ACE HATS")){
        str_cpy(b,v->hg_trk<NTRK?"MAKE -> T  HIHAT":"NO HIHAT TRACK",sizeof b);
        if(v->hg_trk<NTRK)b[9]=(char)('1'+v->hg_trk);
        cv_text_in(10,64,220,&AF_S,b,v->hg_trk<NTRK?SD_LIGHT:SD_RED,SD_BLACK);
    }else cv_text_in(10,64,220,&AF_S,"CURRENT TRACK / PATTERN",SD_LIGHT,SD_BLACK);
    for(i=0;i<16;i++){int32_t x=10+(int32_t)(i%8)*28,y=86+(int32_t)(i/8)*37;uint16_t c=(v->sq_hit>>i)&1u?SD_RED:SD_PANEL;
        char n[8];fmt_int(n,v->sq_at+i+1);cv_rrect(x,y,24,29,3,c,SD_BLACK);cv_text_in(x,y+7,24,&AF_S,n,SD_WHITE,c);
        if(v->sq_pos==i+1)cv_rect(x,y+31,24,2,SD_WHITE);}
}
#include "trap_stage_ui.c"
static void sd_scene(const gb_view_t *v,uint16_t bg)
{
    uint32_t g=sd_group(v);
    if(v->bm){d99_bootmenu(v,bg);return;}
    if(v->sv){d99_save(v,bg);return;}
    if(v->perf){tl_perf(v,bg);sd_header(v);return;}
    if(v->stg){tl_stage(v,bg);return;}
    if(v->smp_row){tl_sampler(v,bg);sd_header(v);return;}
    if(str_eq(v->title,"PIANO ROLL")){tl_pianoroll(v,bg);sd_header(v);return;}
    if(str_eq(v->title,"SEQUENCER"))tl_overview(v,bg);
    else if(str_eq(v->title,"BANKS"))sd_banks(v);
    else if(str_eq(v->title,"CHORD VAR"))sd_variation(v);
    else if(str_eq(v->title,"LUCKY DRUMS")||str_eq(v->title,"ACE HATS")||str_eq(v->title,"LOW ROLLER")||str_eq(v->title,"GOLDEN CHORDS"))sd_pattern(v);
    else if(g==2)sd_instrument(v);
    else tl_picture(v,bg);
    sd_header(v);
    if(g!=2&&!str_eq(v->title,"SEQUENCER"))sd_controls(v,g);
    tl_footer(v,bg);
}

static void sd_strip(void)
{
    uint32_t i;for(i=0;i<cv_w*cv_h;i++){uint16_t c=swap16(cv_px[i]);uint32_t r=(c>>11)*255/31,g=((c>>5)&63)*255/63,b=(c&31)*255/31;
        if(!(r>g+g/3&&r>b+b/3&&r>70)){uint32_t y=(r*3+g*6+b)/10;c=GB_RGB(y,y,y);}else c=GB_RGB(r,r*45/198,r*57/198);
        cv_px[i]=swap16(c);}
}

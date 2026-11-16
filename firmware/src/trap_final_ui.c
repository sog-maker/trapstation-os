/* SPDX-License-Identifier: GPL-3.0-only
 * Approved final Workstation masters, 11.11.2026. Lossless RGB565 assets
 * decoded into the existing LCD strip. No full framebuffer, no audio work. */
#include "ui_final.h"
static uint8_t ws_cache[WS_BLOCK_SIZE];
static uint32_t ws_cached=~0u;

static int ws_unpack(uint32_t block)
{
    uint32_t p=WS_BLOCKS[block],end=WS_BLOCKS[block+1u],q=0;
    while(p<end) {
        uint32_t tok=WS_DATA[p++],n=tok>>4,b=255u;
        if(n==15u)while(b==255u&&p<end){b=WS_DATA[p++];n+=b;}
        if(p+n>end||q+n>WS_BLOCK_SIZE)return 0;
        for(uint32_t i=0;i<n;i++)ws_cache[q++]=WS_DATA[p++];
        if(p==end)break;
        if(p+2u>end)return 0;
        uint32_t off=WS_DATA[p]|((uint32_t)WS_DATA[p+1u]<<8);p+=2u;
        if(!off||off>q)return 0;
        n=(tok&15u)+4u;b=255u;
        if((tok&15u)==15u)while(b==255u&&p<end){b=WS_DATA[p++];n+=b;}
        if(q+n>WS_BLOCK_SIZE)return 0;
        for(uint32_t i=0;i<n;i++){ws_cache[q]=ws_cache[q-off];q++;}
    }
    return q==((WS_RAW_SIZE-block*WS_BLOCK_SIZE)>WS_BLOCK_SIZE?WS_BLOCK_SIZE:WS_RAW_SIZE-block*WS_BLOCK_SIZE);
}
static uint32_t ws_byte(uint32_t *at)
{
    uint32_t p=(*at)++,block=p/WS_BLOCK_SIZE;
    if(block!=ws_cached){if(!ws_unpack(block))return 0;ws_cached=block;}
    return ws_cache[p%WS_BLOCK_SIZE];
}
static uint16_t ws_word(uint32_t *at){uint32_t a=ws_byte(at);return (uint16_t)(a|(ws_byte(at)<<8));}
static uint32_t ws_rowid(const final_img_t *im,uint32_t y)
{
    const uint16_t *p=im->rows;
    for(;;){uint32_t id=*p++,n=1;if(id&32768u){n=(id&32767u)+1u;id=*p++;}if(y<n)return id;y-=n;}
}
static void ws_image(uint32_t id,int32_t x0,int32_t y0)
{
    const final_img_t *im=&WS_IMAGES[id];
    int32_t a=-cv_oy-y0,z=a+(int32_t)cv_h;
    if(a<0)a=0;if(z>im->h)z=im->h;
    for(int32_t y=a;y<z;y++) {
        uint32_t at=WS_OFFSET[ws_rowid(im,(uint32_t)y)],x=0;
        while(x<im->w) {
            uint32_t tok=ws_byte(&at),n;uint16_t col=0;
            if(!tok){x+=ws_byte(&at);continue;}
            n=(tok&128u)?(tok&127u)+1u:tok;
            if(!(tok&128u))col=ws_word(&at);
            for(uint32_t k=0;k<n;k++,x++) {
                if(tok&128u)col=ws_word(&at);
                int32_t xx=x0+(int32_t)x;
                if((uint32_t)xx<cv_w)cv_px[(y0+y+cv_oy)*cv_w+xx]=col;
            }
        }
    }
}
static uint16_t ws_pixel(uint32_t id,uint32_t x,uint32_t y)
{
    const final_img_t *im=&WS_IMAGES[id];uint32_t at=WS_OFFSET[ws_rowid(im,y)],xx=0;
    while(xx<im->w){uint32_t t=ws_byte(&at),n;uint16_t c=0;
        if(!t){n=ws_byte(&at);if(x<xx+n)return 0;xx+=n;continue;}
        n=t&128u?(t&127u)+1u:t;
        if(!(t&128u))c=ws_word(&at);
        for(uint32_t k=0;k<n;k++,xx++){if(t&128u)c=ws_word(&at);if(xx==x)return c;}
    }return 0;
}

static void ws_dial(uint32_t id,int32_t cx,int32_t cy,int32_t ratio,int32_t base)
{
    int32_t r=(WS_IMAGES[id].w-1u)/2u,ang=-140+clamp(ratio,0,1000)*280/1000-base;
    uint32_t ph=(uint32_t)((ang+720)*1024/360);int32_t sn=tl_sin(ph),cs=tl_sin(ph+256u);
    for(int32_t y=-r;y<=r;y++) {
        if((uint32_t)(cy+y+cv_oy)>=cv_h)continue;
        for(int32_t x=-r;x<=r;x++)if(x*x+y*y<=r*r){
            int32_t sx=(x*cs+y*sn)/256+r,sy=(-x*sn+y*cs)/256+r;
            sx=clamp(sx,0,2*r);sy=clamp(sy,0,2*r);
            cv_px[(cy+y+cv_oy)*cv_w+cx+x]=ws_pixel(id,(uint32_t)sx,(uint32_t)sy);
        }
    }
}
static void ws_effect(const gb_view_t *v,int reverb)
{
    gh_status(v,"FX / MASTER");
    ws_image(reverb?WS_REVERB:WS_DELAY,0,34);
    for(uint32_t k=0;k<3;k++)ws_dial(WS_DIAL,204,80+(int32_t)k*34,v->ratio[k],18);
    if(!v->play)return;
    if(reverb) {
        static const int16_t xx[5]={25,133,58,147,25},yy[5]={113,81,139,121,113};
        uint32_t q=(v->brd?v->brd-1u:0u)*4u/960u,s=(v->brd?v->brd-1u:0u)*4u%960u;
        gh_dot(xx[q]+(xx[q+1]-xx[q])*(int32_t)s/960,yy[q]+(yy[q+1]-yy[q])*(int32_t)s/960,GH_DARK);
    } else {
        uint32_t at=(uint32_t)v->echo_ph*8u/12u,tail=1u+(uint32_t)v->ratio[1]*6u/1000u;
        for(uint32_t k=0;k<tail;k++) {
            uint32_t c=(at+8u-k)%8u;
            ws_image(k?WS_ECHO_RED:WS_ECHO_WHITE,20+(int32_t)c*17,76+(int32_t)(k%5u)*12);
        }
    }
}
static void ws_clock(const gb_view_t *v)
{
    char b[24];ws_image(WS_CLOCK,0,34);
    gh_t(23,77,7,str_eq(v->val[2],"INT")?"CLOCK BUS / INTERNAL":str_eq(v->val[2],"USB")?"CLOCK BUS / USB":"CLOCK BUS / TRS",GH_GREY,GH_HEX(0x1b1b1b),GA_S);
    fmt_int(b,v->bpm);
    gh_t(25,98,23,b,GH_WHITE,GH_HEX(0x1b1b1b),GA_S);
    ws_dial(WS_CLOCK_DIAL,203,84,v->ratio[0],13);
    for(uint32_t k=0;k<16u;k++) {
        uint32_t tall=!(k&3u),on=v->play&&v->clk_step==k;
        ws_image(tall?(on?WS_CLOCK_ON_27:WS_CLOCK_OFF_27):(on?WS_CLOCK_ON_20:WS_CLOCK_OFF_20),15+(int32_t)k*13,115);
    }
    str_cpy(b,"SWING ",sizeof b);str_cpy(b+6,v->val[1],sizeof b-6u);gh_t(16,171,7,b,GH_GREY,GH_DARK,GA_S);
    str_cpy(b,"CPU ",sizeof b);str_cpy(b+4,v->val[3],sizeof b-4u);gh_t(223,171,7,b,GH_GREY,GH_DARK,GA_E);
}
static void ws_chordgen(const gb_view_t *v)
{
    const track_t *t=TSEL;char b[24];uint32_t len=t->p[P_SLEN]?t->p[P_SLEN]:16u;
    ws_image(WS_CHORD,0,34);
    for(uint32_t k=0;k<4u;k++) {
        uint32_t a=k*len/4u,z=(k+1u)*len/4u;const step_t *s=0;
        for(uint32_t j=a;j<z&&j<NSTEP;j++)if(t->step[j].time==ST_NOTE&&t->step[j].n){s=&t->step[j];break;}
        int on=v->play&&((uint32_t)(v->brd?v->brd-1u:0u)*4u/960u==k);
        ws_image(on?WS_CHORD_RED:WS_CHORD_OFF,14+(int32_t)k*54,65);
        b[0]='0';b[1]=(char)('1'+k);b[2]=0;gh_t(19+(int32_t)k*54,74,7,b,on?GH_WHITE:GH_GREY,on?GH_RED:GH_P30,GA_S);
        str_cpy(b,s?TL_NOTE[s->note[0]%12u]:"\x97",sizeof b);
        if(s)for(uint32_t q=1;q<s->n&&q<4u;q++)if((s->note[q]+12u-s->note[0])%12u==3u){str_cpy(b+str_len(b),"m",sizeof b-str_len(b));break;}
        gh_t(38+(int32_t)k*54,91,12,b,GH_WHITE,on?GH_RED:GH_P30,GA_M);
        if(s)for(uint32_t q=0;q<s->n&&q<4u;q++) {
            int32_t y=140-(int32_t)(s->note[q]%24u)*28/24;
            cv_rect(18+(int32_t)k*53,y,43,2,q?GH_RED:GH_PINK);
        }
    }
    str_cpy(b,v->val[0],sizeof b);str_cpy(b+str_len(b)," / ",sizeof b-str_len(b));str_cpy(b+str_len(b),v->val[1],sizeof b-str_len(b));
    gh_tc(45,166,7,b,GH_GREY,GH_DARK,106);
}

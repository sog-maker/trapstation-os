

#include "ui_approved.h"
static void ap_image(uint32_t id)
{
    const approved_img_t *im=&AP_IMAGES[id];
    int32_t y0=-cv_oy,y1=y0+(int32_t)cv_h,y;
    for(y=y0;y<y1&&y<240;y++) {
        uint32_t k,x=0;
        for(k=im->row[y];k<im->row[y+1];k++) {
            uint32_t v=im->run[k],n=(v>>16)+1u;
            uint16_t *dst=cv_px+(y+cv_oy)*cv_w+x;
            for(uint32_t j=0;j<n;j++) dst[j]=(uint16_t)v;
            x+=n;
        }
    }
}
static void ap_reel(int32_t x,int32_t y,int32_t r,uint32_t ph)
{
    gh_disc(x,y,r,GH_DARK,GH_P30);
    for(uint32_t k=0;k<6;k++) {
        uint32_t a=ph+k*1024u/6u;
        cv_line_t(x+tl_sin(a)*4/256,y-tl_sin(a+256u)*4/256,
                  x+tl_sin(a)*(r-2)/256,y-tl_sin(a+256u)*(r-2)/256,GH_BG,2);
    }
    gh_disc(x,y,3,GH_INK,GH_DARK);
}
static void ap_record(uint32_t ph)
{
    int32_t sn=tl_sin(ph),cs=tl_sin(ph+256),x,y;
    for(y=98;y<=124;y++) {
        if(y+cv_oy<0||y+cv_oy>=(int32_t)cv_h)continue;
        for(x=81;x<=129;x++) {
            int32_t dx=x-105,dy=(y-111)*100/57;
            if(dx*dx+dy*dy>24*24)continue;
            int32_t sx=(dx*cs+dy*sn)/256,sy=(-dx*sn+dy*cs)/256;
            uint16_t col=sx>=0&&sy<=0&&sx*13<=-sy*20?GH_CARD:GH_RED;
            cv_px[(y+cv_oy)*cv_w+x]=(uint16_t)(col>>8|col<<8);
        }
    }
    gh_disc(105,111,2,GH_GREY,GH_RED);
}
static void ap_motion(const gb_view_t*v,uint32_t gi)
{
    if(!v->an_f||v->an_f>=17)return;
    uint32_t f=v->an_f-1u;
    cv_rect(11,58,218,12,GH_DARK);
    static const char*const label[4]={"GENERATING / DRUM GENERATOR","GENERATING / HIHAT GENERATOR","GENERATING / BASSLINE GENERATOR","GENERATING / CHORDS GENERATOR"};
    gh_t(120,67,7,label[gi],GH_WHITE,GH_DARK,GA_M);
    if(gi==0) {
        uint32_t k=f/2u%3u;

        cv_rect(18+k*68,72,64,1,GH_WHITE);cv_rect(18+k*68,155,64,1,GH_WHITE);
        cv_rect(18+k*68,72,1,84,GH_WHITE);cv_rect(81+k*68,72,1,84,GH_WHITE);
        cv_rect(19+(int32_t)f*127/10,158,10,7,GH_WHITE);
    } else if(gi==1) {
        for(uint32_t r=0;r<3;r++) {
            uint32_t k=(f*(r+1u)+r*3u)%16u;
            for(uint32_t n=0;n<(r==2u?3u:1u);n++)
                cv_rrect(37+(int32_t)((k+n)%16u)*115/10,78+r*30,8,16,2,n?GH_RED:GH_WHITE,GH_DARK);
        }
    } else if(gi==2) {
        cv_rect(19,77,164,75,GH_INK);
        int16_t ys[155];int32_t amp=14+tl_sin(f*33u)*8/256;
        for(int32_t x=0;x<155;x++)ys[x]=(int16_t)(115+tl_sin((uint32_t)(x*15-(int32_t)f*33))*amp/256);
        gh_curve(24,155,ys,GH_RED,2);
        gh_t(102,147,7,"SUB / BUILDING",GH_GREY,GH_INK,GA_M);
    } else {
        static const char*const ch[4]={"Cm7","Abmaj7","Fm9","G7"};
        cv_rect(16,74,209,92,GH_INK);
        for(uint32_t k=0;k<4;k++) {
            int32_t x=20+(int32_t)k*51;int on=k==f/4u;
            cv_rrect(x,79,47,31,2,on?GH_RED:GH_P30,GH_INK);
            gh_t(x+24,99,8,ch[k],GH_WHITE,on?GH_RED:GH_P30,GA_M);
            for(uint32_t n=0;n<3;n++) cv_rrect(x+3,147-n*11,40,3,1,on&&n<=f%4u?GH_WHITE:GH_GREY,GH_INK);
        }
    }

    cv_rrect(84,183,152,32,2,GH_LBL,GH_BG);
    gh_t(160,203,10,"GENERATING",GH_WHITE,GH_LBL,GA_M);
}
static void ap_scene(const gb_view_t*v,uint32_t id)
{
    cv_rect(0,0,240,240,GH_BG);
    ap_image(id);
    gh_status(v,0);gh_footer(v,id<4?GG_MIDI:GG_DAW);
    if(id<4) {
        cv_rrect(4,183,76,32,2,GH_CARD,GH_BG);
        gh_knob(19,200,12,gen_amount[id]*10u,1,GH_CARD);
        gh_t(36,195,7,"Gamble",GH_INK,GH_CARD,GA_S);
        char b[8];fmt_int(b,gen_amount[id]);uint32_t n=str_len(b);b[n]='%';b[n+1]=0;
        gh_t(36,209,11,b,GH_INK,GH_CARD,GA_S);
        cv_rrect(84,183,54,32,2,GH_DARK,GH_BG);gh_t(111,203,9,"UNDO",GH_WHITE,GH_DARK,GA_M);
        cv_rrect(142,183,94,32,2,GH_RED,GH_BG);gh_t(189,203,12,"MAKE",GH_WHITE,GH_RED,GA_M);
        ap_motion(v,id);
    } else {
        gh_cards(v);
        if(v->play) {
            uint32_t ph=(uint32_t)v->fxang*1024u/36u;
            if(id==4){ap_reel(64,116,10,ph);ap_reel(176,116,10,ph+48u);}
            if(id==5){ap_reel(49,111,17,ph);ap_reel(122,111,17,ph+48u);}
            if(id==6)ap_record(ph);
            cv_rect(16,164,212,12,GH_DARK);gh_t(20,174,6,"PLAY",GH_WHITE,GH_DARK,GA_S);
        }
    }
}

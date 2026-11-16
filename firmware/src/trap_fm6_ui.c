/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION OS contributors */

static void gh_fm6(const gb_view_t *v) {
    const page_t *pg=cur_page();
    const int16_t *p=tf6_edit[v->sel];
    uint32_t o=v->fm6_op,base=(5u-o)*21u,id=pg->id[0];
    char label[32];
    for(uint32_t n=0;n<6;n++) {
        uint16_t col=n==o?GH_RED:GH_DARK;
        cv_rect(4+39*n,34,35,17,col);
        label[0]='O';label[1]='P';label[2]='1'+n;label[3]=0;
        gh_t(21+39*n,46,8,label,GH_WHITE,col,GA_M);
    }
    gh_disp(56,104);gh_mark=1;
    if(id==134u || id==18u) {
        uint8_t bus[3]={0},edge[6]={0},depth[6]={0},max=0,counts[6]={0},seen[6]={0};
        int16_t x[6],y[6];const uint8_t *alg=FM6_ALG[p[134]&31];
        for(uint32_t k=0;k<6;k++) {
            uint32_t in=(alg[k]>>4)&3u,out=alg[k]&3u;
            edge[k]=in?bus[in]:0;
            if(alg[k]&4u)bus[out]|=1u<<k;else bus[out]=1u<<k;
        }
        for(int k=5;k>=0;k--) for(int j=0;j<k;j++) if(edge[k]&(1u<<j)) {
            if(depth[j]<=depth[k])depth[j]=depth[k]+1;
            if(depth[j]>max)max=depth[j];
        }
        for(uint32_t k=0;k<6;k++)counts[depth[k]]++;
        for(uint32_t k=0;k<6;k++) {uint32_t d=depth[k];x[k]=12+(++seen[d])*216/(counts[d]+1);y[k]=69+(max-d)*76/(max?max:1);}
        for(uint32_t k=0;k<6;k++) {
            for(uint32_t j=0;j<k;j++)if(edge[k]&(1u<<j)) {
                cv_line(x[j],y[j],x[k],y[k],GH_RED);gh_edge_dot(v,x[j],y[j],x[k],y[k]);
            }
            if(!(alg[k]&3u))cv_line(x[k],y[k]+6,x[k],156,GH_GREY);
        }
        for(uint32_t k=0;k<6;k++) {
            uint16_t col=5u-k==o?GH_RED:GH_LBL;
            if(alg[k]&64u) {cv_line(x[k]+8,y[k]-9,x[k]+15,y[k]-9,GH_RED);cv_line(x[k]+15,y[k]-9,x[k]+15,y[k]+4,GH_RED);cv_line(x[k]+15,y[k]+4,x[k]+8,y[k]+4,GH_RED);}
            cv_rect(x[k]-9,y[k]-6,18,12,col);label[0]='6'-k;label[1]=0;
            gh_t(x[k],y[k]+4,8,label,GH_WHITE,col,GA_M);
        }
    } else if(id==0u || id==4u || id==126u) {
        uint32_t b=id==126u?126u:base,total=0;
        int16_t x[5]={18},y[5];uint32_t duration[4];
        for(uint32_t k=0;k<4;k++) {duration[k]=8+99-p[b+k];total+=duration[k];}
        y[0]=152-p[b+7]*84/99;
        uint32_t at=0;
        for(uint32_t k=0;k<4;k++) {at+=duration[k];x[k+1]=18+at*202/total;y[k+1]=152-p[b+4+k]*84/99;cv_line(x[k],y[k],x[k+1],y[k+1],GH_RED);cv_rect(x[k+1]-1,y[k+1]-1,3,3,GH_WHITE);}
        gh_path_dot(v,x,y,5);
    } else if(id==16u || id==8u) {
        for(uint32_t n=0;n<6;n++) {int32_t x=24+33*n,h=p[(5-n)*21+16]*80/99;cv_rect(x,68,20,80,GH_LBL);cv_rect(x,148-h,20,h,n==o?GH_RED:GH_GREY);}
    } else {
        int32_t last=0;
        for(int32_t x=0;x<202;x++) {
            uint32_t phase=(uint32_t)(x*(id==137u||id==139u?2+p[137]/25:4))*1024u/202u;
            int32_t wave=tl_sin(phase);
            if(id==137u || id==139u) {
                uint32_t q=phase&1023u;
                switch(p[142]) {
                case 0:wave=q<512u?-256+(int32_t)q:768-(int32_t)q;break;
                case 1:wave=256-(int32_t)q/2;break;
                case 2:wave=(int32_t)q/2-256;break;
                case 3:wave=q<512u?256:-256;break;
                case 5:wave=tl_sin((phase&~127u)*37u);break;
                }
            } else if(id==201u) wave=wave*64/(64+x*2);
            else if(id==205u) wave=(wave+tl_sin(phase*7u)/2)*64/(64+x);
            else if(id==155u) wave=clamp(wave*(100+p[155])/100,-256,256);
            int32_t y=108-wave*32/256;
            if(x)cv_line(18+x-1,last,18+x,y,GH_RED);
            gh_ys[x]=y;last=y;
        }
        gh_marker(v,18,202);
    }
    str_cpy(label,"OCT+ + K1: OP",sizeof label);label[13]='1'+o;label[14]=0;
    gh_t(7,173,8,label,GH_INK,GH_BG,GA_S);
    if(id==126u)gh_t(232,173,7,v->fm6_pitch?"K2: LEVEL":"K2: RATE",GH_INK,GH_BG,GA_E);
    gh_cards(v);
}

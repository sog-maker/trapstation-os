
#define BANK_EXTRA_N 8u
#define BANK_EXTRA_COUNT 4u
static const char *const N_EXTRA[4][8]={
    {"D808","REECE","PLUCK","DBELL","RLEAD","KICK","BELL","HAT"},
    {"RHODS","MELLO","JBASS","LOKEY","SOFT8","BRUSH","VRIM","CLAVI"},
    {"DKEYS","WMPAD","LPIAN","DUSTK","TAPSN","VCLK","MUTE","SOFTB"},
    {"JNPAD","STRNG","DREAM","DSUB","W808","LCLAP","AIR","GLASS"},
};
static const param_desc_t EXTRA_D[4][2]={
    {PE("SOUND",N_EXTRA[0],0),PE("LOAD",N_GO,0)},
    {PE("SOUND",N_EXTRA[1],0),PE("LOAD",N_GO,0)},
    {PE("SOUND",N_EXTRA[2],0),PE("LOAD",N_GO,0)},
    {PE("SOUND",N_EXTRA[3],0),PE("LOAD",N_GO,0)},
};

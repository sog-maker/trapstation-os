/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

static const char *const N_LWAVE[] = {"SIN", "TRI", "SAW", "SQR", "S&H"};

static const char *const N_LSYNC[] = {"OFF", "4BAR", "2BAR", "1/1", "1/2", "1/4", "1/8", "8T", "1/16", "16T", "1/32"};
static const uint8_t LSYNC_DIV[11] = {0, 9, 8, 7, 6, 0, 1, 4, 2, 5, 3};

static const char *const N_FXRATE[] = {"4BAR", "2BAR", "1/1", "1/2", "1/4", "1/8", "8T", "1/16", "16T", "1/32"};
static const char *const N_FXWAVE[] = {"SIN", "TRI", "UP", "DOWN", "SQR", "S&H"};
static const char *const N_FXDST[] = {"OFF", "AMP", "DJ", "C.MIX", "CTUNE", "CFDBK", "CDAMP", "DTIME", "DFDBK", "DTONE",
                                      "RTIME", "RDAMP", "GRIT", "D.SND", "R.SND", "D.OUT", "R.OUT", "PAN"};
enum { FXD_DSND = 13, FXD_PAN = 17 };
static const char *const N_LTRIG[] = {"NOTE", "FREE"};
static const char *const N_LPOL[] = {"BI", "UNI"};
static const char *const N_AMODE[] = {"OFF", "UP", "DN", "UPDN", "RND", "ORD", "REPEAT",
                                       "DNUP", "UP+8", "CONV", "DIVG", "PINKY", "THUMB", "WALK", "CHORD"};
static const char *const N_SAMODE[] = {"UP", "DOWN", "UPDN", "CYCLE", "RAND", "PATRN"};
static const char *const N_SARATE[] = {"1/64", "1/32", "16T", "1/16", "8T", "1/8", "1/4"};
static const char *const N_DIV[] = {"1/4", "1/8", "1/16", "1/32", "8T", "16T", "1/2", "1/1", "2BAR", "4BAR"};

static const char *const N_SDIV[] = {"1/4", "1/8", "1/16", "1/32", "8T", "16T", "1/2", "1/1", "2BAR", "4BAR", "1/64"};

static const char *const N_DTIME[] = {"1/4", "1/8", "1/16", "1/32", "8T", "16T", "1/2", "1/1", "2BAR", "4BAR",
                                      "1/8D", "1/16D", "1/4D"};
static const char *const N_SCALE[] = {"CHR", "MAJ", "MIN", "DOR", "MIX", "PEN", "MPEN", "HARM",
                                    "PHRY", "LYD", "LOC", "MEL", "BLUES", "WHOLE", "DIMHW", "DIMWH"};
static const char *const N_ONOFF[] = {"OFF", "ON"};

static const char *const N_FTYPE[] = {"LP", "BP", "HP"};

#define ESYNC_N 25u
static const char *const ESYNC_NAMES[ESYNC_N + 1u] = {
    "0", "1/64T", "1/64", "1/32T", "1/64D", "1/32", "1/16T", "1/32D", "1/16", "1/8T", "1/16D", "1/8", "1/4T", "1/8D",
    "1/4", "1/2T", "1/4D", "1/2", "1/1T", "1/2D", "1/1", "1/1D", "2BAR", "3BAR", "4BAR", 0};
static const uint16_t ESYNC_UNITS[ESYNC_N] = {0, 4, 6, 8, 9, 12, 16, 18, 24, 32, 36, 48, 64, 72,
                                              96, 128, 144, 192, 256, 288, 384, 576, 768, 1152, 1536};
static const param_desc_t ESYNC_DESC[3] = {
    {"ATK", F_INT, 0, 127, 10, ESYNC_NAMES, 0},
    {"DEC", F_INT, 0, 127, 70, ESYNC_NAMES, 0},
    {"REL", F_INT, 0, 127, 60, ESYNC_NAMES, 0},
};
static uint32_t esync_idx(int32_t v) { return (uint32_t)(v & 127) * ESYNC_N >> 7; }

static const char *const N_QUANT[] = {"OFF", "SNAP", "WHITE", "SEQ", "SEQ+T"};

static const char *const N_CHRD[] = {"OFF", "DIA3", "DIA7", "MAJ", "MIN", "DOM7", "MAJ7", "MIN7", "SUS4", "POW", "MIN9", "m7b5", "MAJ9", "MIN11", "MIN13",
                                     "SUS2", "ADD9", "mADD9"};
static const char *const N_VOIC[] = {"CLOSE", "OPEN", "INV1", "INV2", "+OCT"};
static const char *const N_VOICE[] = {"POLY", "MONO", "LEG", "UNI"};
static const char *const N_GLMODE[] = {"RATE", "TIME"};
static const char *const N_PRIO[] = {"LAST", "LOW", "HIGH"};
static const char *const N_ALLOC[] = {"ROT", "REUSE"};
static const char *const N_ORDER[] = {"NOTE", "PLAY"};
static const char *const N_CLOCK[] = {"INT", "USB", "TRS"};
static const char *const N_MIDI_INPUT[] = {"USB", "TRS"};

static const char *const N_ROUTE[] = {"CH1-4", "SEL", "CH5-8", "CH9-12", "CH13-16", "TRACK"};
static const char *const N_NOTE[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const char *const N_DASH[] = {"--"};
static const char *const N_GO[] = {"--", "GO"};
static const char *const N_SLCR[] = {"OFF", "GATE", "STUT"};
static const char *const N_SLDIV[] = {"1/8", "1/16", "1/32", "8T", "16T", "32T"};

static const char *const N_ITYPE[] = {"OFF", "SOFT", "HARD", "FOLD", "FUZZ", "CRUSH", "PHASR", "FLANG", "CHOR"};

static const char *const N_IBITS[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15",
                                      "16", 0};
static const char *const N_IRATE[] = {"689", "919", "1.1k", "1.4k", "1.8k", "2.2k", "2.8k", "3.7k", "4.4k", "5.5k",
                                      "7.4k", "8.8k", "11k", "15k", "22k", "44k", 0};

static const char *const N_MSRC[] = {"OFF", "LFO", "ENV", "VEL", "KEY", "RAND", "MODW", "AT", "EXPR", "S&H", "SLEW", "FENV"};
static const char *const N_HOLD[] = {"HOLD"};
static const char *const N_MDST[] = {"OFF", "PITCH", "CUT", "SHP", "AMP", "PAN", "SCRUB", "-", "DLY", "REV", "RATE",
                                     "L.PIT", "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8", "DEPTH", "VEL", "TIME",

                                     "ATK", "DEC", "SUS", "REL", "E.FLT", "E.PIT", "E.SHP", "E.R2", "L.FLT", "L.SHP", "L.AMP",
                                     "DTIME", "DFDBK", "DTONE", "RTIME", "RDAMP", "GRIT", "RES", "F.AMT", "F.ATK",
                                     "F.DEC", "F.SUS", "F.REL", "C.MIX", "CTUNE", "CFDBK", "CDAMP", "DJ",
                                     "LEVEL", "CRUSH", "GATE",
                                     "TYPE", "WAVE", "PHASE", "L.FAD", "SYNC", "L.TRG", "POL", "VOL",
                                     "C.ATK", "C.DEC", "D.ATK", "D.DEC", "FB", "DTUN"};
static const char *const N_ENGNAME[] = {"ANALOG", FELUCCA_FM4 ? "DIGITAL" : "-", "PHASE", "LOFI", "SAMPLE", "VOICE", "TRIO", "WHEEL", "GRAIN", "PHYS",
                                             "DRUM", "NOISE", "FM6",
};

#define PD(l, f, mn, mx, df) {l, f, mn, mx, df, 0, 0}
#define PE(l, n, df) {l, F_ENUM, 0, (int16_t)(sizeof(n) / sizeof(n[0]) - 1), df, n, 0}

static const param_desc_t TP[P_COUNT] = {
    [P_LEVEL] = PD("LVL", F_DB, 0, 127, 104),
    [P_ATK] = PD("ATK", F_TIME, 0, 127, 10),
    [P_DEC] = PD("DEC", F_TIME, 0, 127, 70),
    [P_SUS] = PD("SUS", F_PCT, 0, 127, 90),
    [P_REL] = PD("REL", F_TIME, 0, 127, 60),
    [P_ED_FLT] = PD("FLT", F_BIPCT, -64, 63, 0),
    [P_ED_PIT] = PD("PIT", F_BIPCT, -64, 63, 0),
    [P_ED_SHP] = PD("SHP", F_BIPCT, -64, 63, 0),
    [P_ED_FX] = PD("R2", F_BIPCT, -64, 63, 0),
    [P_LRATE] = PD("RATE", F_LFOHZ, 0, 127, 60),
    [P_LWAVE] = PE("WAVE", N_LWAVE, 0),
    [P_LPHASE] = PD("PHS", F_INT, 0, 127, 0),
    [P_LFADE] = PD("FADE", F_TIME, 0, 127, 0),
    [P_LD_PIT] = PD("PIT", F_BIPCT, -64, 63, 0),
    [P_LD_FLT] = PD("FLT", F_BIPCT, -64, 63, 0),
    [P_LD_SHP] = PD("SHP", F_BIPCT, -64, 63, 0),
    [P_LD_AMP] = PD("AMP", F_PCT, 0, 127, 0),
#if FELUCCA_SIMPLE
    [P_AMODE] = PE("MODE", N_SAMODE, 0),
#else
    [P_AMODE] = PE("MODE", N_AMODE, 0),
#endif
#if FELUCCA_SIMPLE
    [P_ARATE] = PE("RATE", N_SARATE, 3),
#else
    [P_ARATE] = PE("RATE", N_DIV, 2),
#endif
    [P_AOCT] = PD("OCT", F_INT, 1, 4, 1),
    [P_AGATE] = PD("GATE", F_PCT, 1, 127, 64),
#if FELUCCA_SIMPLE
    [P_ASWING] = PD("FADE", F_PCT, 0, 100, 0),
#else
    [P_ASWING] = PD("SWG", F_PCT, 0, 100, 0),
#endif
    [P_APROB] = PD("CHANCE", F_PCT, 0, 100, 100),
#if FELUCCA_SIMPLE
    [P_AHOLD] = PE("ARP", N_ONOFF, 0),
#else
    [P_AHOLD] = PE("HOLD", N_ONOFF, 0),
#endif
    [P_AORDER] = PE("ORD", N_ORDER, 0),
    [P_ROOT] = PD("ROOT", F_NOTE, 0, 11, 0),
    [P_SCALE] = PE("SCL", N_SCALE, 0),
    [P_QUANT] = PE("QNT", N_QUANT, 0),
    [P_TRANS] = PD("PITCH", F_SEMI, -24, 24, 0),
    [P_SLEN] = PD("LEN", F_STEPS, 1, 64, 16),
    [P_SDIV] = PE("DIV", N_SDIV, 2),
    [P_SSWING] = PD("SWG", F_PCT, 0, 100, 0),
    [P_SGATE] = PD("GATE", F_PCT, 1, 127, 64),
    [P_DIST] = PD("SCRUB", F_INT, 0, 127, 0),
    [P_CHOR] = PD("CHO", F_PCT, 0, 127, 0),
    [P_DLY] = PD("DLY", F_PCT, 0, 127, 0),
    [P_REV] = PD("REV", F_PCT, 0, 127, 0),
    [P_VOICE] = PE("VCE", N_VOICE, 0),
    [P_GLIDE] = PD("GLD", F_TIME, 0, 127, 0),
    [P_GLMODE] = PE("GLMOD", N_GLMODE, 0),
    [P_PRIO] = PE("PRIO", N_PRIO, 0),
    [P_ALLOC] = PE("ALLOC", N_ALLOC, 0),
    [P_DETUNE] = PD("DTUNE", F_INT, 0, 127, 40),
    [P_PAN] = PD("PAN", F_BIPCT, -64, 63, 0),
    [P_MUTE] = PE("MUTE", N_ONOFF, 0),
    [P_SLCR] = PE("SLCR", N_SLCR, 0),
    [P_SLPAT] = PD("PAT", F_INT, 1, 16, 1),
    [P_SLRATE] = PE("RATE", N_SLDIV, 1),
    [P_SLDEPTH] = PD("DEPTH", F_PCT, 0, 127, 127),
#define MSLOT(k) [P_M##k##SRC] = PE("SRC" #k, N_MSRC, 0), [P_M##k##DST] = PE("DST" #k, N_MDST, 0), \
                 [P_M##k##AMT] = PD("AMT" #k, F_BIPCT, -64, 63, 0)
    [P_M1SRC] = PE("SRC", N_MSRC, 1), [P_M1DST] = PE("DEST", N_MDST, 0), [P_M1AMT] = PD("AMT", F_BIPCT, -64, 63, 0),
    [P_M2SRC] = PE("SRC2", N_MSRC, 0), [P_M2DST] = PE("DEST", N_MDST, 0), [P_M2AMT] = PD("AMT", F_BIPCT, -64, 63, 0),
    MSLOT(3), MSLOT(4),
#undef MSLOT

#define FMOP(k) [P_FM##k##_ATK] = PD("ATK", F_TIME, 0, 127, 0), \
                [P_FM##k##_DEC] = PD("DEC", F_TIME, 0, 127, 0), \
                [P_FM##k##_SUS] = PD("SUS", F_PCT, 0, 127, 127), \
                [P_FM##k##_REL] = PD("REL", F_TIME, 0, 127, 0), \
                [P_FM##k##_LEVEL] = PD("LVL", F_PCT, 0, 127, 127)
    [P_FLT_CUT] = PD("CUT", F_PCT, 0, 127, 127), [P_FLT_RES] = PD("RES", F_PCT, 0, 127, 0),
    [P_FLT_DRV] = PD("DRV", F_PCT, 0, 127, 0), [P_FLT_KTR] = PD("KTR", F_PCT, 0, 127, 0),
    [P_FM1_LEVEL] = PD("LVL", F_PCT, 0, 127, 127),
    [P_FE_ATK] = PD("ATK", F_TIME, 0, 127, 0), [P_FE_DEC] = PD("DEC", F_TIME, 0, 127, 40),
    [P_FE_SUS] = PD("SUS", F_PCT, 0, 127, 0), [P_FE_AMT] = PD("AMT", F_BIPCT, -64, 63, 0),
    [P_FE_REL] = PD("REL", F_TIME, 0, 127, 40),

    [P_FM3_ATK] = PD("C.ATK", F_TIME, 0, 127, 0), [P_FM3_DEC] = {"C.DEC", F_TIME, 0, 127, 90, N_HOLD, 0},
    [P_FM3_SUS] = PD("SUS", F_PCT, 0, 127, 127), [P_FM3_REL] = PD("REL", F_TIME, 0, 127, 0),
    [P_FM3_LEVEL] = PD("FB", F_PCT, 0, 127, 0),
    [P_FM4_ATK] = PD("D.ATK", F_TIME, 0, 127, 0), [P_FM4_DEC] = {"D.DEC", F_TIME, 0, 127, 127, N_HOLD, 0},
    [P_FM4_SUS] = PD("SUS", F_PCT, 0, 127, 127), [P_FM4_REL] = PD("REL", F_TIME, 0, 127, 0),
    [P_FM4_LEVEL] = PD("DTUN", F_PCT, 0, 127, 0),
#undef FMOP
    [P_CHRD] = PE("CHRD", N_CHRD, 0),
    [P_VOIC] = PE("VOIC", N_VOIC, 0),

#define LANE(k, l) [P_LN0 + (k)] = PD(l, F_PCT, 0, 127, 127)
    LANE(3, "HATCL"), LANE(4, "HATOP"), LANE(5, "TOM"), LANE(6, "RIM"), LANE(7, "BELL"),
#undef LANE

    [P_LN0] = PD("LEVEL", F_INT, 0, 127, 100), [P_LN1] = PD("CRUSH", F_INT, 0, 127, 0),

    [P_LN2] = PD("GATE", F_INT, 0, 16, 2),
    [P_LSYNC] = PE("SYNC", N_LSYNC, 0),
    [P_LTRIG] = PE("TRIG", N_LTRIG, 0),
    [P_LPOL] = PE("POL", N_LPOL, 0),
    [P_SQNT] = PE("QNTZ", N_ONOFF, !FELUCCA_SIMPLE),
    [P_SPRD] = PD("SPRD", F_PCT, 0, 127, 0),

    [P_ITYPE] = PE("INSRT", N_ITYPE, 0),
    [P_IA] = PD("INS A", F_PCT, 0, 127, 64),
    [P_IB] = PD("INS B", F_PCT, 0, 127, 96),
    [P_IC] = PD("INS C", F_PCT, 0, 127, 96),
    [P_IMIX] = PD("MIX", F_PCT, 0, 127, 127),
    [P_FTYPE] = PE("TYPE", N_FTYPE, 0),
    [P_ESYNC] = PE("ESYNC", N_ONOFF, 0),
};

#define PU(l, f, df, n, u) {l, f, 0, 127, df, n, u}
static const param_desc_t INS_DESC[3][3] = {
    {PU("DRIVE", F_PCT, 64, 0, 0), PU("TONE", F_CUTOFF, 96, 0, 0), PU("LEVEL", F_DB, 96, 0, 0)},
    {PU("BITS", F_INT, 64, N_IBITS, 0), PU("RATE", F_INT, 96, N_IRATE, "Hz"), PU("LPF", F_CUTOFF, 96, 0, 0)},
    {PU("RATE", F_LFOHZ, 64, 0, 0), PU("DEPTH", F_PCT, 96, 0, 0), PU("FDBK", F_PCT, 96, 0, 0)},
};
#undef PU
static const param_desc_t *ins_desc(int32_t type, uint32_t k)
{
    if (type <= 0 || type >= (int32_t)NELEM(N_ITYPE))
        return &TP[P_IA + k];
    return &INS_DESC[type < 5 ? 0 : type == 5 ? 1 : 2][k];
}

static const param_desc_t GP[G_COUNT] = {
    [G_BPM] = PD("BPM", F_BPM, 40, 240, 120),
    [G_SWING] = PD("SWG", F_PCT, 0, 100, 0),
    [G_CLOCK] = PE("CLK", N_CLOCK, 0),
    [G_TUNE] = PD("TUNE", F_INT, -50, 50, 0),
    [G_DTIME] = PD("TIME", F_PCT, 0, 127, 80),
    [G_DFDBK] = PD("FDBK", F_PCT, 0, 120, 60),
    [G_DCOLOR] = PD("TONE", F_PCT, 0, 127, 64),
    [G_DMIX] = PD("MIX", F_PCT, 0, 127, 90),
    [G_RSIZE] = PD("TIME", F_PCT, 0, 127, 90),
    [G_RDAMP] = PD("DAMP", F_PCT, 0, 127, 60),
    [G_CRATE] = PD("TUNE", F_PCT, 0, 127, 40),
    [G_CDEPTH] = PD("FDBK", F_PCT, 0, 127, 60),

    [G_MIDI] = PD("DJ", F_BIPCT, -64, 63, 0),
    [G_SYNC] = PD("MIX", F_PCT, 0, 127, 0),

    [G_ROUTE] = PE("ROUT", N_ROUTE, 0),
    [G_INFO] = PD("DAMP", F_PCT, 0, 127, 0),
    [G_F1RATE] = PE("RATE", N_FXRATE, 4),
    [G_F1DEST] = PE("DEST", N_FXDST, 0),
    [G_F1AMT] = PD("AMT", F_BIPCT, -64, 63, 0),
    [G_F2RATE] = PE("RATE", N_FXRATE, 4),
    [G_F2DEST] = PE("DEST", N_FXDST, 0),
    [G_F2AMT] = PD("AMT", F_BIPCT, -64, 63, 0),
    [G_F3RATE] = PE("RATE", N_FXRATE, 4),
    [G_F3DEST] = PE("DEST", N_FXDST, 0),

    [G_RTYPE] = PD("GRIT", F_PCT, 0, 127, 0),
    [G_F3AMT] = PD("AMT", F_BIPCT, -64, 63, 0),
    [G_AMP] = PD("AMP", F_PCT, 0, 127, 127),
    [G_PAN] = PD("PAN", F_BIPCT, -64, 63, 0),
    [G_F1WAVE] = PE("WAVE", N_FXWAVE, 0),
    [G_F2WAVE] = PE("WAVE", N_FXWAVE, 0),
    [G_F3WAVE] = PE("WAVE", N_FXWAVE, 0),
};

static const param_desc_t AD_SUS = {"-", F_INT, 0, 127, 90, 0, 0}, AD_REL = {"-", F_INT, 0, 127, 60, 0, 0};
static const param_desc_t AD_NONE = {"-", F_INT, 0, 127, 0, 0, 0};

static const param_desc_t AN_GLIDE_D = {"GLIDE", F_INT, 0, 127, 0, 0, 0}, AN_SUB_D = {"SUB", F_INT, 0, 127, 0, 0, 0};
/* .. an 808's ACCENT strength (P_ED_FX, unused on an 808: eng_analog.c, after X0X) */
#define AN_ACC_DEF 80
static const param_desc_t AN_ACC_D = {"AMT", F_INT, 0, 127, AN_ACC_DEF, 0, 0};

static const char *const N_SACC[] = {"-", "ACC"};
static const char *const N_SRATE[] = {"OFF", "X2", "X3", "X4"};
static const char *const N_SRAMP[] = {"FLAT", "UP", "DOWN"};
static const param_desc_t SFN_D[2][4] = {
    {PE("ACC", N_SACC, 0), {"AMT", F_INT, 0, 127, AN_ACC_DEF, 0, 0}, PD("SWING", F_INT, 0, 100, 0), PD("-", F_INT, 0, 0, 0)},
    {PE("RATE", N_SRATE, 0), PD("STAIR", F_INT, -3, 3, 0), PE("RAMP", N_SRAMP, 0), PE("LAST", N_SACC, 0)},
};
static int16_t sfn_val[4];
static int an_is_kick(const track_t *t)
{
    uint32_t m = (uint32_t)t->p[P_E7] % AN_N;
    return eng_idx(t->eng_req) == ENGI_ANALOG && (m == AN_KICK || m == AN_BASS);
}

static int flt_blend(const track_t *t)
{
    uint32_t e = eng_idx(t->eng_req);
    return FELUCCA_SIMPLE && (e == ENGI_MONO || (e == ENGI_DIGITAL && t->p[P_E7] % 6 == 0));
}
static int ln_on(const track_t *t, uint32_t id)
{
    uint32_t e = eng_idx(t->eng_req);
    return id == P_LN0 ? e == ENGI_ANALOG || e == ENGI_SAMPLER || e == ENGI_MONO : id == P_LN1 ? e == ENGI_ANALOG || e == ENGI_MONO : id == P_LN2 ? e == ENGI_SAMPLER : 1;
}
static const param_desc_t *track_desc(const track_t *t, uint32_t id)
{
    if ((id == P_SUS || id == P_REL) && trk_machine(t)->drum == 1)
        return id == P_SUS ? &AD_SUS : &AD_REL;
    if (id >= P_E0 && id <= P_E7) {
        const engine_t *e = ENGINES[eng_idx(t->eng_req)];
        const param_desc_t *d = e->desc ? e->desc(t, id - P_E0) : 0;
        return d ? d : &e->edit[id - P_E0];
    }
    if (FELUCCA_SIMPLE && id == P_LN1 && flt_blend(t))
        return &MONO_TYPE;
    if (FELUCCA_SIMPLE && id >= P_LN0 && id <= P_LN2 && !ln_on(t, id))
        return &AD_NONE;
    if (FELUCCA_SIMPLE && id == P_LN0 && eng_idx(t->eng_req) == ENGI_MONO)
        return &MONO_SAT;
    if (id >= P_IA && id <= P_IC)
        return ins_desc(t->p[P_ITYPE], id - P_IA);
    if (t->p[P_ESYNC] && (id == P_ATK || id == P_DEC || id == P_REL))
        return &ESYNC_DESC[id == P_ATK ? 0 : id == P_DEC ? 1 : 2];
    if (FELUCCA_SIMPLE && id == P_GLIDE && eng_idx(t->eng_req) == ENGI_ANALOG)
        return &AN_GLIDE_D;
    if (FELUCCA_SIMPLE && id == P_DIST && an_is_kick(t))
        return &AN_SUB_D;
    if (FELUCCA_SIMPLE && id == P_ED_FX && eng_idx(t->eng_req) == ENGI_ANALOG)
        return &AN_ACC_D;
    return &TP[id];
}

static const param_desc_t *param_desc_of(uint32_t e, uint32_t id)
{
    return id >= P_E0 ? &ENGINES[e]->edit[id - P_E0] : &TP[id];
}

static int32_t param_fit(const param_desc_t *d, int32_t v) { return clamp(v, d->min, d->max); }

static int enum_skip(const param_desc_t *d, int32_t v) { return d->names == N_MDST && (v == 6 || v == 7); }

static int32_t enum_step(const param_desc_t *d, int32_t from, int32_t v)
{
    int32_t dir = v > from ? 1 : -1;
    while (v != from && enum_skip(d, v))
        v = v + dir > d->max || v + dir < d->min ? from : v + dir;
    return v;
}

static const uint8_t DIV_ORDER[10] = {9, 8, 7, 6, 0, 1, 4, 2, 5, 3};
static const uint8_t SLDIV_ORDER[6] = {0, 3, 1, 4, 2, 5};
static const uint8_t DTIME_ORDER[13] = {9, 8, 7, 6, 12, 0, 10, 1, 11, 4, 2, 5, 3};

static const uint8_t MDST_ORDER[] = {0, 12, 13, 14, 15, 16, 17, 18, 62, 63, 64, 65, 66, 67, 51, 52, 53, 6, 2, 40, 41, 42, 43, 44, 45, 23, 24, 25,
                                     26, 61, 5, 10, 8, 34, 35, 36, 9, 37, 38, 39, 46, 47, 48,
                                     49, 50, 1, 4};
static int32_t mdst_rank(int32_t v)
{
    int32_t r;
    for (r = 0; r < (int32_t)sizeof MDST_ORDER; r++)
        if (MDST_ORDER[r] == v)
            return r;
    return 0;
}

static int mdst_live(const track_t *t, int32_t v)
{
    uint32_t e = eng_idx(t->eng_req);
    if (v == 6)
        return e == ENGI_SAMPLER;
    if (v >= 12 && v <= 18)
        return track_desc(t, P_E0 + (uint32_t)v - 12u)->label[0] != '-';
    if (v >= 51 && v <= 53)
        return track_desc(t, P_LN0 + (uint32_t)v - 51u)->label[0] != '-';
    if (v >= 62 && v <= 67)
        return e == ENGI_DIGITAL && t->p[P_E7] % 6 == 0;
    if (v == 2 || (v >= 23 && v <= 26) || (v >= 40 && v <= 45)) {
        if (e == ENGI_ANALOG)
            return 0;
        if (v == 25 || v == 26)
            return track_desc(t, v == 25 ? P_SUS : P_REL)->label[0] != '-';
    }
    return 1;
}
static const uint8_t *enum_order(const param_desc_t *d)
{
    return d->names == N_DIV ? DIV_ORDER : d->names == N_SLDIV ? SLDIV_ORDER : d->names == N_DTIME ? DTIME_ORDER : 0;
}
static int32_t enum_rank(const param_desc_t *d, int32_t v)
{
    const uint8_t *o = enum_order(d);
    int32_t r;
    if (FELUCCA_SIMPLE && d->names == N_MDST)
    {
        int32_t k, r = mdst_rank(v), below = 0, all = 0;
        for (k = 1; k < (int32_t)sizeof MDST_ORDER; k++)
            if (mdst_live(TSEL, MDST_ORDER[k])) {
                all++;
                below += k <= r;
            }
        return d->min + below * (d->max - d->min) / (all ? all : 1);
    }
    for (r = 0; o && r < d->max - d->min; r++)
        if (o[r] == v - d->min)
            break;
    return o ? r + d->min : v;
}

static int32_t param_turn(const param_desc_t *d, int32_t v, int32_t steps)
{
    const uint8_t *o = enum_order(d);
    if (FELUCCA_SIMPLE && d->names == N_MDST)
    {
        int32_t r = mdst_rank(v), dir = steps < 0 ? -1 : 1, left = steps < 0 ? -steps : steps, k;
        for (k = r + dir; left && k >= 0 && k < (int32_t)sizeof MDST_ORDER; k += dir)
            if (MDST_ORDER[k] == 0 || mdst_live(TSEL, MDST_ORDER[k])) {
                r = k;
                left--;
            }
        return MDST_ORDER[r];
    }
    if (o)
        return o[clamp(enum_rank(d, v) - d->min + steps, 0, d->max - d->min)] + d->min;
    if (d->names == ESYNC_NAMES) {
        int32_t k = clamp((int32_t)esync_idx(v) + steps, 0, ESYNC_N - 1);
        return (k * 128 + ESYNC_N - 1) / ESYNC_N;
    }
    return enum_step(d, v, clamp(v + steps, d->min, d->max));
}

static uint8_t fmt_named;

static void param_format(const param_desc_t *d, int32_t v, char *val, const char **unit)
{
    *unit = "";
    fmt_named = d->fmt == F_ENUM;
    switch (d->fmt) {
    case F_PCT:
        fmt_int(val, d->max == 100 ? v : (v * 100 + 63) / 127);
        *unit = "%";
        break;
    case F_BIPCT:

        fmt_int(val, v * 100 / 64);
        if (v > 0) {
            char t[8];
            fmt_int(t, v * 100 / 63);
            val[0] = '+';
            str_cpy(val + 1, t, 6);
        }
        *unit = "%";
        break;
    case F_TIME: {
        uint32_t ms10 = TIME_MS_X10[v & 127];
        if (d->names && v >= d->max) {
            str_cpy(val, d->names[0], 6);
            break;
        }
        if (ms10 < 100u) {
            fmt_fix(val, (int32_t)ms10, 1);
            *unit = "ms";
        } else if (ms10 < 10000u) {
            fmt_int(val, (int32_t)((ms10 + 5u) / 10u));
            *unit = "ms";
        } else {
            fmt_fix(val, (int32_t)(ms10 / 100u), 2);
            if (ms10 >= 100000u)
                fmt_fix(val, (int32_t)(ms10 / 1000u), 1);
            *unit = "s";
        }
        break;
    }
    case F_LFOHZ: {
        uint32_t h = LFO_HZ_X100[v & 127];
        if (h < 1000u)
            fmt_fix(val, (int32_t)h, 2);
        else
            fmt_fix(val, (int32_t)(h / 10u), 1);
        *unit = "Hz";
        break;
    }
    case F_CUTOFF: {
        uint32_t h = CUTOFF_HZ[v & 127];
        if (h < 1000u) {
            fmt_int(val, (int32_t)h);
            *unit = "Hz";
        } else {
            fmt_fix(val, (int32_t)(h / 100u), 1);
            *unit = "kHz";
        }
        break;
    }
    case F_DB:
        if (v <= 0) {
            str_cpy(val, "OFF", 6);
        } else {
            fmt_fix(val, LEVEL_DB_X10[v], 1);
            *unit = "dB";
        }
        break;
    case F_SEMI:
        fmt_int(val, v);
        if (v > 0) {
            char t[8];
            fmt_int(t, v);
            val[0] = '+';
            str_cpy(val + 1, t, 6);
        }
        *unit = "st";
        break;
    case F_ENUM:
        str_cpy(val, d->names[v < d->min ? d->min : v > d->max ? d->max : v], 6);
        if (d->unit)
            *unit = d->unit;
        break;
    case F_BPM:
        fmt_int(val, v);
        *unit = "BPM";
        break;
    case F_NOTE:
        str_cpy(val, N_NOTE[v % 12], 6);
        break;
    case F_ONOFF:
        str_cpy(val, N_ONOFF[v ? 1 : 0], 6);
        break;
    case F_STEPS:
        fmt_int(val, v);
        *unit = "STEP";
        break;
    default:
        if (d->names) {
            uint32_t k = 0;
            while (d->names[k])
                k++;
            str_cpy(val, d->names[(uint32_t)(clamp(v, d->min, d->max) - d->min) * k / (uint32_t)(d->max - d->min + 1)], 6);
        } else {
            fmt_int(val, v);
        }
        if (d->unit)
            *unit = d->unit;
        break;
    }
}

enum { FAM_HOME, FAM_ENV, FAM_LFO, FAM_FX, FAM_SCL, FAM_EDIT, FAM_GLO, FAM_SAVE, FAM_ARP, FAM_SEQ, FAM_TRK,
       FAM_FLT,

       FAM_COUNT };
enum { SC_TRACK, SC_GLOBAL, SC_ENGINE, SC_STEP, SC_TRK, SC_SFN, SC_CHGEN, SC_BANKA, SC_TAPE, SC_PHONK, SC_BANKS, SC_GEN85,
       SC_TRAPGEN, SC_OVIEW, SC_HATGEN, SC_BANKALL, SC_SLICE, SC_FM6 };
#define MIX_PAN 0x10u
enum { GR_NONE, GR_ADSR, GR_LFO, GR_STEPS, GR_ARP, GR_SCALE, GR_FX, GR_ROLL, GR_BROWSE, GR_SLOTS, GR_USER, GR_TRK,
       GR_SLCR, GR_MOD, GR_PATS, GR_SONG, GR_TOOLS, GR_CHORD, GR_SLICES, GR_EVENTS,
       GR_FMOP, GR_FMOP2, GR_FMENV, GR_INS, GR_PERF, GR_GBSEQ, GR_ACID, GR_STAGE };

typedef struct {
    const char *title;
    uint8_t fam, scope, graph;
    uint8_t id[4];
    uint8_t fxt;
} page_t;

enum { CG_STYLE, CG_RHYTHM, CG_OCT, CG_FOLLOW };
static int16_t chord_cfg[NTRK][4], chord_go;

typedef struct {
 uint8_t active,style,rhythm,variation;
 int8_t oct;
 uint8_t root,scale,chrd,voic,vmode,len,reserved;
} gen_recipe_t;
_Static_assert(sizeof(gen_recipe_t)==12u,"stable generation tail layout");

#define GEN_V2 2u
static gen_recipe_t gen_recipe[NTRK];
static int16_t gen_var[NTRK]={1,1,1,1,1,1},gen_live[NTRK]={1,1,1,1,1,1};
static uint8_t gen_fresh[NTRK],gen_undo_ready;
static int16_t eu_cfg[NTRK][3]={{16,4,0},{16,4,0},{16,4,0},{16,4,0},{16,4,0},{16,4,0}};
static int16_t gen_action;
static const param_desc_t G85_D[]={PD("VAR",F_INT,1,20,1),PE("LIVE",N_ONOFF,1),PE("UNDO",N_GO,0),PE("MAKE",N_GO,0),
 PD("LEN",F_INT,1,64,16),PD("HITS",F_INT,0,64,4),PD("ROT",F_INT,0,63,0)};

static const char *const N_TGLEN[] = {"16", "32", "64"};
static const param_desc_t TG_D = PE("MAKE", N_GO, 0);

enum { SL_SET, SL_STYLE, SL_MAKE, SL_MAN, SL_COUNT };
static const char *const N_SLSTYLE[] = {"1/4", "1/8", "1/16"};
static const param_desc_t SL_D[SL_COUNT] = {PD("SET", F_INT, 2, 16, 8), PE("STYLE", N_SLSTYLE, 2), PE("MAKE", N_GO, 0),
                                            PE("MANUAL", N_ONOFF, 0)};
static int16_t sl_cfg[SL_COUNT] = {8, 2, 0, 0};

static uint8_t smp_lay;
static uint32_t pg_kid(const page_t *pg, uint32_t k)
{
    static const uint8_t L1[4] = {P_E4, P_E5, P_E6, P_LN0}, L2[4] = {P_LN1, P_DIST, 0xFF, 0xFF};
    k &= 3u;
    if (FELUCCA_SIMPLE && smp_lay && pg->scope == SC_ENGINE && pg->id[0] == P_E0 && eng_idx(TSEL->eng_req) == ENGI_SAMPLER)
        return smp_lay == 1u ? L1[k] : L2[k];
    return pg->id[k];
}
static int16_t tg_go;

static const char *const N_HGP[] = {"BASIC", "ENDROL", "TRIP16", "ROLL48", "ROLL64", "STAIRU", "STAIRD", "TRIPL", "TRIP8",
                                    "GALLOP", "SNRGAP", "GHOST", "DRLBST", "DRLTRP", "MEM70", "MEMSHF", "PHONK", "DRIFT",
                                    "LOFI", "DROP"};
static const param_desc_t HG_D[4] = {PE("PRESET", N_HGP, 0), PD("VAR", F_INT, 1, 20, 1), PE("LEN", N_TGLEN, 1),
                                     PE("MAKE", N_GO, 0)};
static int16_t hg_cfg[4] = {0, 1, 1, 0};

static const char *const N_CSTYLE[] = {"MEMPH", "DRIFT", "JAZZ", "WAVE", "CHILL", "LOFI", "TRAP", "HIPHOP"};
static const char *const N_CRHYTHM[] = {"HOLD", "PULSE", "SYNC", "STAB"};
static const char *const N_CFOLLOW[] = {"OFF", "T1", "T2", "T3", "T4", "T5", "T6"};
static const param_desc_t CG_DESC[] = {
    PE("STYL", N_CSTYLE, 0), PE("RHY", N_CRHYTHM, 0), PD("OCT", F_INT, -2, 2, 0),
    PE("FROM", N_CFOLLOW, 0), PE("MAKE", N_GO, 0),
};
static int16_t bank_a_pick[NTRK], bank_a_go;
static const char *const N_BANKA[] = {"MBELL", "RBELL", "ORGAN", "DORG", "HPAD", "GPAD", "DLEAD", "808B"};
static const param_desc_t BANKA_D[2] = {PE("SOUND", N_BANKA, 0), PE("LOAD", N_GO, 0)};
#include "trap_bank_names.h"
static int16_t bank_extra_pick[4][NTRK],bank_extra_go;

static int16_t bk_bank[NTRK];
static const char *const N_BKL[] = {"A", "B", "C", "D", "E"};
static const param_desc_t BK_D[2] = {PE("BANK", N_BKL, 0), PE("LOAD", N_GO, 0)};

enum { TF_ON, TF_AGE, TF_SAT, TF_CUT, TF_WOW, TF_WRATE, TF_FLUT, TF_FRATE,
       TF_HISS, TF_DUST, TF_CRKL, TF_WEAR, TF_COUNT };
#define TAPE_DEFAULT {0,0,0,7,0,1,0,1,0,0,0,0}
static const int16_t TAPE_DEF[TF_COUNT]=TAPE_DEFAULT;
static int16_t tape_cfg[TF_COUNT]=TAPE_DEFAULT;
static const char *const N_TCUT[]={"4k", "6k", "8k", "10k", "12k", "14k", "16k", "OPEN"};
static const char *const N_TWR[]={"0.3", "0.5", "0.8", "1.0", "1.5"};
static const char *const N_TFR[]={"6", "8", "10", "12"};
static const param_desc_t TF_D[TF_COUNT]={
    PE("ON",N_ONOFF,0),PD("AGE",F_PCT,0,127,0),PD("SAT",F_PCT,0,127,0),PE("CUT",N_TCUT,7),
    PD("WOW",F_PCT,0,127,0),{"RATE",F_ENUM,0,4,1,N_TWR,"Hz"},
    PD("FLUT",F_PCT,0,127,0),{"RATE",F_ENUM,0,3,1,N_TFR,"Hz"},
    PD("HISS",F_PCT,0,127,0),PD("DUST",F_PCT,0,127,0),
    PD("CRKL",F_PCT,0,127,0),PD("WEAR",F_PCT,0,127,0),
};

enum { PH_TIME, PH_SCENE, PH_MASK, PH_APPLY, PH_BSTYLE, PH_OCT, PH_VAR,
       PH_BMAKE, PH_JQUAL, PH_JRHY, PH_JMAKE, PH_FILL, PH_FMAKE, PH_CAPTURE,
       PH_QMOVE, PH_QPITCH, PH_QPICK, PH_COUNT };
static int16_t ph_time=1, ph_scene, ph_masks[4]={36,47,63,32};
static int16_t ph_bstyle,ph_oct,ph_var,ph_jqual,ph_jrhy,ph_fill,ph_go;
static uint8_t ph_request;
static const char *const N_PHTIME[]={"0.25s","0.5s","1s","2s"};
static const char *const N_PHSCENE[]={"INTRO","VERSE","HOOK","OUTRO"};
static const char *const N_PHBASS[]={"DARK","CLASS","GANG","HORR"};

static const char *const N_PHJAZZ[]={"7","9","11","13","PAR7","PAR9"};
static const char *const N_PHFILL[]={"ROLL","RISE","BURST","GHOST","TRIP"};
static int16_t *qf_value(uint32_t id);
static const param_desc_t PH_D[PH_COUNT]={
 PE("BRAKE",N_PHTIME,1),PE("PART",N_PHSCENE,0),PD("MASK",F_INT,0,63,63),PE("LOAD",N_GO,0),
 PE("STYL",N_PHBASS,0),PD("OCT",F_INT,-2,1,0),PD("VAR",F_INT,0,15,0),PE("MAKE",N_GO,0),
 PE("CHRD",N_PHJAZZ,0),PE("RHY",N_CRHYTHM,0),PE("MAKE",N_GO,0),
 PE("TYPE",N_PHFILL,0),PE("MAKE",N_GO,0),PE("SAVE",N_GO,0),
 PD("MOVE",F_INT,0,64,0),{"PITCH",F_NOTE,0,127,0,0,0},PD("PICK",F_INT,0,64,0)
};
static int16_t *ph_value(uint32_t id) {
 switch(id) {
 case PH_TIME:return &ph_time; case PH_SCENE:return &ph_scene;
 case PH_MASK:return &ph_masks[(uint32_t)ph_scene%4u];
 case PH_BSTYLE:return &ph_bstyle;case PH_OCT:return &ph_oct;case PH_VAR:return &ph_var;
 case PH_JQUAL:return &ph_jqual;case PH_JRHY:return &ph_jrhy;case PH_FILL:return &ph_fill;
 case PH_QMOVE:case PH_QPITCH:case PH_QPICK:return qf_value(id);
 default:return &ph_go;
 }
}
static const char *const N_OVBAR[] = {"AUTO", "1", "2", "3", "4"};
static const param_desc_t OV_D = PE("BAR", N_OVBAR, 0);
static int16_t ov_bar;

static const char *const N_SQBAR[] = {"ALL", "1", "2", "3", "4"};
static const char *const N_BDUP[] = {"-", "1", "2", "3", "4"};
static const param_desc_t SB_D = PE("PLAY", N_SQBAR, 0), BD_D = PE("COPY TO", N_BDUP, 0);
static int16_t seq_bar, bar_dup;
#if TRAP_FM6_TEST && TRAP_UX_TEST
static uint8_t tf6_ui_op, tf6_ui_pitch;
static int16_t tf6_curves[NTRK][6];
static void tf6_ui_commit(int16_t *vp) {
    for(uint32_t t=0;t<NTRK;t++) for(uint32_t o=0;o<6;o++)
        if(vp==&tf6_curves[t][o]) {
            tf6_edit[t][o*21+11]=*vp/4;tf6_edit[t][o*21+12]=*vp%4;return;
        }
}
#endif
static const page_t PAGES[] = {
    {"ENV", FAM_ENV, SC_TRACK, GR_ADSR, {P_ATK, P_DEC, P_SUS, P_REL}},
    {"VOL PAN", FAM_ENV, SC_TRACK, GR_NONE, {P_LEVEL, P_PAN, 0xFF, 0xFF}},
    {"ENV DEST", FAM_ENV, SC_TRACK, GR_NONE, {P_M2DST, P_M2AMT, 0xFF, 0xFF}},

    {"LFO", FAM_LFO, SC_TRACK, GR_LFO, {P_LRATE, P_LWAVE, P_M1DST, P_M1AMT}},
    {"SYNC", FAM_LFO, SC_TRACK, GR_NONE, {P_LSYNC, P_LTRIG, P_LPOL, P_LFADE}},

    {"SENDS", FAM_FX, SC_TRACK, GR_NONE, {P_DLY, P_REV, 0xFF, 0xFF}},

    {"DELAY", FAM_FX, SC_GLOBAL, GR_NONE, {G_DTIME, G_DFDBK, G_DCOLOR, 0xFF}, 1},
    {"VERB", FAM_FX, SC_GLOBAL, GR_NONE, {G_RSIZE, G_RDAMP, G_RTYPE, 0xFF}, 1},
    {"LFO 1", FAM_FLT, SC_GLOBAL, GR_NONE, {G_F1RATE, G_F1WAVE, G_F1DEST, G_F1AMT}, 1},
    {"LFO 2", FAM_ENV, SC_GLOBAL, GR_NONE, {G_F2RATE, G_F2WAVE, G_F2DEST, G_F2AMT}, 1},
    {"LFO 3", FAM_LFO, SC_GLOBAL, GR_NONE, {G_F3RATE, G_F3WAVE, G_F3DEST, G_F3AMT}, 1},

    {"BANKS", FAM_EDIT, SC_BANKALL, GR_NONE, {0, 1, 2, 0xFF}},
#if TRAP_FM6_TEST
#if TRAP_UX_TEST
    {"COLOR", FAM_LFO, SC_FM6, GR_NONE, {155,156,157,200}},
    {"DELAY", FAM_LFO, SC_FM6, GR_NONE, {201,202,203,204}},
    {"REVERB", FAM_LFO, SC_FM6, GR_NONE, {205,206,207,208}},
    {"ROUTING", FAM_EDIT, SC_FM6, GR_NONE, {134,135,144,136}},
    {"OP TONE", FAM_EDIT, SC_FM6, GR_NONE, {18,19,17,20}},
    {"OP LEVEL", FAM_EDIT, SC_FM6, GR_NONE, {16,15,14,13}},
    {"OP RATES", FAM_FLT, SC_FM6, GR_NONE, {0,1,2,3}},
    {"OP LEVELS", FAM_FLT, SC_FM6, GR_NONE, {4,5,6,7}},
    {"KEY SCALE", FAM_FLT, SC_FM6, GR_NONE, {8,9,10,159}},
    {"LFO MOTION", FAM_ENV, SC_FM6, GR_NONE, {137,142,138,141}},
    {"LFO DEPTH", FAM_ENV, SC_FM6, GR_NONE, {139,140,143,255}},
    {"PITCH ENV", FAM_ENV, SC_FM6, GR_NONE, {126,127,128,129}},
#else
    {"FM6 ALG", FAM_EDIT, SC_FM6, GR_NONE, {134,135,136,144}},
    {"OP1 TONE", FAM_EDIT, SC_FM6, GR_NONE, {121,123,124,125}},
    {"OP1 RATE", FAM_ENV, SC_FM6, GR_NONE, {105,106,107,108}},
    {"OP1 LEVEL", FAM_ENV, SC_FM6, GR_NONE, {109,110,111,112}},
    {"OP1 SCALE", FAM_EDIT, SC_FM6, GR_NONE, {113,114,115,118}},
    {"OP1 SENS", FAM_EDIT, SC_FM6, GR_NONE, {116,117,119,120}},
    {"OP2 TONE", FAM_EDIT, SC_FM6, GR_NONE, {100,102,103,104}},
    {"OP2 RATE", FAM_ENV, SC_FM6, GR_NONE, {84,85,86,87}},
    {"OP2 LEVEL", FAM_ENV, SC_FM6, GR_NONE, {88,89,90,91}},
    {"OP2 SCALE", FAM_EDIT, SC_FM6, GR_NONE, {92,93,94,97}},
    {"OP2 SENS", FAM_EDIT, SC_FM6, GR_NONE, {95,96,98,99}},
    {"OP3 TONE", FAM_EDIT, SC_FM6, GR_NONE, {79,81,82,83}},
    {"OP3 RATE", FAM_ENV, SC_FM6, GR_NONE, {63,64,65,66}},
    {"OP3 LEVEL", FAM_ENV, SC_FM6, GR_NONE, {67,68,69,70}},
    {"OP3 SCALE", FAM_EDIT, SC_FM6, GR_NONE, {71,72,73,76}},
    {"OP3 SENS", FAM_EDIT, SC_FM6, GR_NONE, {74,75,77,78}},
    {"OP4 TONE", FAM_EDIT, SC_FM6, GR_NONE, {58,60,61,62}},
    {"OP4 RATE", FAM_ENV, SC_FM6, GR_NONE, {42,43,44,45}},
    {"OP4 LEVEL", FAM_ENV, SC_FM6, GR_NONE, {46,47,48,49}},
    {"OP4 SCALE", FAM_EDIT, SC_FM6, GR_NONE, {50,51,52,55}},
    {"OP4 SENS", FAM_EDIT, SC_FM6, GR_NONE, {53,54,56,57}},
    {"OP5 TONE", FAM_EDIT, SC_FM6, GR_NONE, {37,39,40,41}},
    {"OP5 RATE", FAM_ENV, SC_FM6, GR_NONE, {21,22,23,24}},
    {"OP5 LEVEL", FAM_ENV, SC_FM6, GR_NONE, {25,26,27,28}},
    {"OP5 SCALE", FAM_EDIT, SC_FM6, GR_NONE, {29,30,31,34}},
    {"OP5 SENS", FAM_EDIT, SC_FM6, GR_NONE, {32,33,35,36}},
    {"OP6 TONE", FAM_EDIT, SC_FM6, GR_NONE, {16,18,19,20}},
    {"OP6 RATE", FAM_ENV, SC_FM6, GR_NONE, {0,1,2,3}},
    {"OP6 LEVEL", FAM_ENV, SC_FM6, GR_NONE, {4,5,6,7}},
    {"OP6 SCALE", FAM_EDIT, SC_FM6, GR_NONE, {8,9,10,13}},
    {"OP6 SENS", FAM_EDIT, SC_FM6, GR_NONE, {11,12,14,15}},
    {"FM6 MODE", FAM_EDIT, SC_FM6, GR_NONE, {17,38,59,80}},
    {"FM6 MODE 2", FAM_EDIT, SC_FM6, GR_NONE, {101,122,255,255}},
    {"FM6 LFO", FAM_LFO, SC_FM6, GR_NONE, {137,138,139,140}},
    {"FM6 LFO 2", FAM_LFO, SC_FM6, GR_NONE, {141,142,143,255}},
    {"PITCH RATE", FAM_ENV, SC_FM6, GR_NONE, {126,127,128,129}},
    {"PITCH LEVEL", FAM_ENV, SC_FM6, GR_NONE, {130,131,132,133}},
    {"FM6 COLOR", FAM_FLT, SC_FM6, GR_NONE, {155,156,157,255}},
#endif
#endif
    {"OSC 1", FAM_EDIT, SC_ENGINE, GR_NONE, {P_E0, P_E1, P_E2, P_E3}},
    {"SLICE", FAM_EDIT, SC_SLICE, GR_NONE, {SL_SET, SL_STYLE, SL_MAKE, SL_MAN}},
    {"OSC 2", FAM_EDIT, SC_ENGINE, GR_NONE, {P_E4, P_E5, P_E6, P_LN0}},
    {"CRUSH", FAM_EDIT, SC_ENGINE, GR_NONE, {P_LN1, P_DIST, P_GLIDE, 0xFF}},
    {"MOD C", FAM_EDIT, SC_ENGINE, GR_NONE, {P_FM3_ATK, P_FM3_DEC, P_E4, P_E6}},
    {"MOD D", FAM_EDIT, SC_ENGINE, GR_NONE, {P_FM4_ATK, P_FM4_DEC, P_E5, P_FM3_LEVEL}},
    {"TUNE", FAM_EDIT, SC_ENGINE, GR_NONE, {P_FM4_LEVEL, P_TRANS, 0xFF, 0xFF}},
    {"COLOR", FAM_EDIT, SC_ENGINE, GR_NONE, {P_FLT_CUT, P_DEC, P_SPRD, P_FLT_DRV}},
    {"FILTER", FAM_FLT, SC_TRACK, GR_NONE, {P_FLT_CUT, P_FLT_RES, P_FTYPE, P_FE_AMT}},
    {"FILTER ENV", FAM_FLT, SC_TRACK, GR_NONE, {P_FE_ATK, P_FE_DEC, P_FE_SUS, P_FE_REL}},

    {"TAPE", FAM_GLO, SC_TAPE, GR_NONE, {TF_ON, TF_AGE, TF_SAT, TF_CUT}, 1},
    {"VHS", FAM_GLO, SC_TAPE, GR_NONE, {TF_WOW, TF_WRATE, TF_FLUT, TF_FRATE}, 1},
    {"VINYL", FAM_GLO, SC_TAPE, GR_NONE, {TF_HISS, TF_DUST, TF_CRKL, TF_WEAR}, 1},
    {"AMP", FAM_GLO, SC_GLOBAL, GR_NONE, {G_AMP, G_PAN, 0xFF, 0xFF}, 1},
    {"DJ", FAM_GLO, SC_GLOBAL, GR_NONE, {G_MIDI, 0xFF, 0xFF, 0xFF}, 1},
    {"PERF", FAM_GLO, SC_GLOBAL, GR_PERF, {0xFF, 0xFF, 0xFF, 0xFF}, 1},
    {"COMB", FAM_GLO, SC_GLOBAL, GR_NONE, {G_SYNC, G_CRATE, G_CDEPTH, G_INFO}, 1},
    {"SEQ", FAM_SEQ, SC_TRACK, GR_GBSEQ, {P_SLEN, P_SDIV, P_APROB, 0xFF}},

    {"LUCKY DRUMS", FAM_ARP, SC_TRAPGEN, GR_NONE, {0xFF, 0xFF, 0xFF, 3}},
    {"ACE HATS", FAM_ARP, SC_HATGEN, GR_NONE, {0xFF, 0xFF, 0xFF, 3}},
    {"LOW ROLLER", FAM_ARP, SC_PHONK, GR_NONE, {0xFF, 0xFF, 0xFF, PH_BMAKE}},
    {"GOLDEN CHORDS", FAM_ARP, SC_PHONK, GR_NONE, {0xFF, 0xFF, 0xFF, PH_JMAKE}},
    {"SCALE", FAM_ARP, SC_TRACK, GR_NONE, {P_ROOT, P_SCALE, P_QUANT, P_GLIDE}},
    {"ARP", FAM_ARP, SC_TRACK, GR_NONE, {P_AHOLD, P_AMODE, P_ARATE, P_ASWING}},
    {"CHORD", FAM_ARP, SC_TRACK, GR_NONE, {P_CHRD, P_VOIC, P_ROOT, P_SCALE}},
    {"CHORD GEN", FAM_ARP, SC_CHGEN, GR_NONE, {CG_STYLE, CG_RHYTHM, CG_OCT, 4}},
    {"CHORD VAR",FAM_ARP,SC_GEN85,GR_NONE,{0,1,2,3}},
    {"QUICK FILL", FAM_ARP, SC_PHONK, GR_NONE, {PH_QPICK, PH_QMOVE, PH_QPITCH, PH_FMAKE}},
    {"EUCLID",FAM_ARP,SC_GEN85,GR_NONE,{4,5,6,7}},

    {"CLOCK", FAM_TRK, SC_GLOBAL, GR_NONE, {G_BPM, G_SWING, G_CLOCK, 0xFF}},
    {"SCENES", FAM_TRK, SC_PHONK, GR_NONE, {PH_SCENE, 0xFF, PH_CAPTURE, PH_APPLY}, 2},

    {"PERFORMANCE", FAM_TRK, SC_GLOBAL, GR_STAGE, {0xFF, 0xFF, 0xFF, 0xFF}, 2},
    {"SEQUENCER", FAM_TRK, SC_OVIEW, GR_NONE, {0, 1, 2, 0xFF}},
    {"PIANO ROLL", FAM_TRK, SC_OVIEW, GR_NONE, {0, 0xFF, 0xFF, 0xFF}},
    {"MIX 1-2", FAM_TRK, SC_TRK, GR_TRK, {0, MIX_PAN | 0, 1, MIX_PAN | 1}},

    {"MIX 3-4", FAM_TRK, SC_TRK, GR_TRK, {2, MIX_PAN | 2, 3, MIX_PAN | 3}},
    {"MIX 5-6", FAM_TRK, SC_TRK, GR_TRK, {4, MIX_PAN | 4, 5, MIX_PAN | 5}},

    {"STAGE", FAM_SEQ, SC_TRACK, GR_ACID, {0xFF, 0xFF, 0xFF, 0xFF}},
    {"ACID", FAM_SEQ, SC_TRACK, GR_ACID, {0xFF, 0xFF, 0xFF, 0xFF}},
};
#define NPAGES (sizeof(PAGES) / sizeof(PAGES[0]))

static const page_t SFN_PAGES[2] = {
    {"ACCENT", FAM_SEQ, SC_SFN, GR_NONE, {0, 1, 2, 0xFF}},
    {"ROLL", FAM_SEQ, SC_SFN, GR_NONE, {0, 1, 2, 3}},
};
static uint8_t mod_ui_slot;

static uint8_t alt_mode;
static const uint8_t *alt_ids(const page_t *pg)
{
    (void)pg;
    return 0;
}

static uint32_t page_id_an(const page_t *pg, uint32_t id)
{
#if TRAP_FM6_TEST
    if(pg->scope==SC_ENGINE && TSEL->eng_req==ENGI_FM6) return 0xFFu;
#endif
    if (pg->scope == SC_ENGINE && str_eq(pg->title, "COLOR")) {
        uint32_t e = eng_idx(TSEL->eng_req);
        if (an_is_kick(TSEL))
            return id == P_FLT_CUT ? P_E3 : id == P_DEC ? P_E1 : id == P_FLT_DRV ? P_E6 : 0xFFu;
        if (!(e == ENGI_MONO || (e == ENGI_DIGITAL && TSEL->p[P_E7] == 0))) return 0xFFu;
        if (e == ENGI_MONO && id == P_SPRD) return 0xFFu;
        return id;
    }
    if (FELUCCA_SIMPLE && pg->scope == SC_ENGINE && eng_idx(TSEL->eng_req) == ENGI_SAMPLER &&
        (pg->id[0] == P_E4 || pg->id[0] == P_LN1))
        return 0xFFu;
    if (FELUCCA_SIMPLE && pg->scope == SC_ENGINE) {
        int syn = eng_idx(TSEL->eng_req) == ENGI_DIGITAL && TSEL->p[P_E7] % 6 == 0;
        if (pg->id[0] == P_FM3_ATK || pg->id[0] == P_FM4_ATK || pg->id[0] == P_FM4_LEVEL)
            return syn ? id : 0xFFu;
        if (syn && pg->id[0] == P_E4)
            return 0xFFu;
    }
    if (FELUCCA_SIMPLE && id == P_DIST && pg->scope == SC_ENGINE)
        return eng_idx(TSEL->eng_req) == ENGI_SAMPLER || an_is_kick(TSEL) ? id : 0xFFu;
    if (FELUCCA_SIMPLE && id == P_GLIDE && pg->scope == SC_ENGINE && pg->id[0] == P_LN1)
        return eng_idx(TSEL->eng_req) == ENGI_ANALOG ? id : 0xFFu;
    if (FELUCCA_SIMPLE && id == P_FTYPE && flt_blend(TSEL))
        return P_LN1;
    if (!FELUCCA_SIMPLE || pg->scope != SC_ENGINE || (id != P_LN0 && id != P_LN1 && id != P_E0))
        return id;
    if (id == P_LN1 && flt_blend(TSEL))
        return 0xFFu;
    if (id == P_E0)
        return eng_idx(TSEL->eng_req) == ENGI_SAMPLER ? P_TRANS : id;
    if (id == P_LN0 && eng_idx(TSEL->eng_req) == ENGI_DIGITAL && TSEL->p[P_E7] % 6 == 0)
        return P_TRANS;
    if (id == P_LN1 && eng_idx(TSEL->eng_req) == ENGI_SAMPLER)
        return P_LN2;
    return ln_on(TSEL, id) ? id : 0xFFu;
}
static const param_desc_t *page_desc(const page_t *pg, uint32_t slot, int16_t **valp)
{
    uint32_t id;
#if TRAP_FM6_TEST
    if(pg->scope==SC_FM6) {
        static param_desc_t d[4];
        static const char *const opnames[21]={"RATE 1","RATE 2","RATE 3","RATE 4","LEVEL 1","LEVEL 2","LEVEL 3","LEVEL 4","B POINT","L DEPTH","R DEPTH","L CURVE","R CURVE","RATE SC","AM SENS","VEL SEN","LEVEL","FIXED","COARSE","FINE","DETUNE"};
        static const char *const common[19]={"P RATE 1","P RATE 2","P RATE 3","P RATE 4","P LEVEL 1","P LEVEL 2","P LEVEL 3","P LEVEL 4","ALGO","FEEDBACK","OSC SYN","SPEED","DELAY","PITCH MOD","AMP MOD","LFO SYNC","WAVE","PIT SEN","TRANS"};
        id=pg->id[slot&3u]; *valp=0;
#if TRAP_UX_TEST
        if(id>=200u && id<=208u) {
            static const uint8_t ids[]={P_CHOR,G_DTIME,G_DFDBK,G_DCOLOR,P_DLY,G_RSIZE,G_RDAMP,G_RTYPE,P_REV};
            uint32_t q=id-200u,j=ids[q];
            if(q==0u||q==4u||q==8u) {*valp=&TSEL->p[j];return &TP[j];}
            *valp=&song.g[j];return &GP[j];
        }
        if(id==159u) {
            uint32_t o=5u-tf6_ui_op;
            *valp=&tf6_curves[song.sel][o];
            **valp=tf6_edit[song.sel][o*21+11]*4+tf6_edit[song.sel][o*21+12];
            d[slot]=(param_desc_t){"CURVES",F_INT,0,15,0,0,0};return &d[slot];
        }
        if(id<21u) id+=(5u-tf6_ui_op)*21u;
        if(id>=126u && id<130u && tf6_ui_pitch) id+=4u;
#endif
        if(id>=158u) return 0;
        if(!tf6_ready[song.sel]) tf6_load(song.sel,0);
        d[slot]=(param_desc_t){id<126u?opnames[id%21u]:id<145u?common[id-126u]:id==155u?"DRIVE":id==156u?"CUTOFF":"OUTPUT",F_INT,0,id<145u?(int16_t)fm6_max(id):id==156u?20000:100,0,0,0};
        if(id==156u) d[slot].min=40;
        *valp=&tf6_edit[song.sel][id];
        return &d[slot];
    }
#endif
    if (pg->scope == SC_PHONK) {
        id=pg->id[slot&3u];
        *valp=id<PH_COUNT?ph_value(id):0;
        return id<PH_COUNT?&PH_D[id]:0;
    }
    if (pg->scope == SC_TAPE) {
        id=pg->id[slot&3u];
        *valp=id<TF_COUNT?&tape_cfg[id]:0;
        return id<TF_COUNT?&TF_D[id]:0;
    }
    if (pg->scope == SC_SLICE) {
        id = pg->id[slot & 3u];
        *valp = id < SL_COUNT ? &sl_cfg[id] : 0;
        return id < SL_COUNT ? &SL_D[id] : 0;
    }
    if (pg->scope == SC_OVIEW) {
        if (str_eq(pg->title, "SEQUENCER") && (slot & 3u) == 1u) {
            *valp = &seq_bar;
            return &SB_D;
        }
        if (str_eq(pg->title, "SEQUENCER") && (slot & 3u) == 2u) {
            *valp = &bar_dup;
            return &BD_D;
        }
        *valp = (slot & 3u) == 0u ? &ov_bar : 0;
        return (slot & 3u) == 0u ? &OV_D : 0;
    }
    if (pg->scope == SC_HATGEN) {
        id = pg->id[slot & 3u];
        *valp = id == 3u ? &hg_cfg[3] : 0;
        return id == 3u ? &HG_D[3] : 0;
    }
    if (pg->scope == SC_TRAPGEN) {
        *valp = pg->id[slot & 3u] == 3u ? &tg_go : 0;
        return pg->id[slot & 3u] == 3u ? &TG_D : 0;
    }
    if(pg->scope==SC_GEN85) {
        id=pg->id[slot&3u];
        *valp=id==0?&gen_var[song.sel]:id==1?&gen_live[song.sel]:id>=4&&id<=6?&eu_cfg[song.sel][id-4]:&gen_action;
        return &G85_D[id==7?3:id];
    }
    if (pg->scope == SC_BANKALL) {
        uint32_t k = song.sel % NTRK, b = (uint32_t)bk_bank[k] % 5u;
        if (slot == 0u) { *valp = &bk_bank[k]; return &BK_D[0]; }
        if (slot == 2u) { *valp = &bank_extra_go; return &BK_D[1]; }
        if (slot != 1u) { *valp = 0; return 0; }
        *valp = b ? &bank_extra_pick[b - 1u][k] : &bank_a_pick[k];
        return b ? &EXTRA_D[b - 1u][0] : &BANKA_D[0];
    }
    if(pg->scope==SC_BANKS) {
        uint32_t b=pg->id[3]%4u;
        if(slot>=2u){*valp=0;return 0;}
        *valp=slot?&bank_extra_go:&bank_extra_pick[b][song.sel%NTRK];
        return &EXTRA_D[b][slot];
    }
    if (pg->scope == SC_BANKA) {
        id = pg->id[slot & 3u];
        *valp = id == 0u ? &bank_a_pick[song.sel % NTRK] : id == 1u ? &bank_a_go : 0;
        return id < 2u ? &BANKA_D[id] : 0;
    }
    if (pg->scope == SC_CHGEN) {
        id = pg->id[slot & 3u];
        *valp = id < 4u ? &chord_cfg[song.sel % NTRK][id] : id == 4u ? &chord_go : 0;
        return id <= 4u ? &CG_DESC[id] : 0;
    }
    if (pg->scope == SC_SFN) {
        if (pg->id[slot & 3u] == 0xFFu || (pg == &SFN_PAGES[0] && slot == 1u && eng_idx(TSEL->eng_req) != ENGI_ANALOG)) {
            *valp = 0;
            return 0;
        }
        *valp = &sfn_val[slot & 3u];
        return &SFN_D[pg == &SFN_PAGES[1]][slot & 3u];
    }
    id = page_id_an(pg, pg_kid(pg, slot));
    if (FELUCCA_SIMPLE && alt_mode && alt_ids(pg))
        id = alt_ids(pg)[slot];
    if (id == 0xFFu) {
        *valp = 0;
        return 0;
    }
    if (id & 0x80u) {
        *valp = &song.g[id & 0x7Fu];
        return &GP[id & 0x7Fu];
    }
    if (pg->scope == SC_STEP || pg->scope == SC_TRK) {
        *valp = 0;
        return 0;
    }
    if (pg->scope == SC_GLOBAL) {
        *valp = &song.g[id];
        return &GP[id];
    }
    if (pg->graph == GR_MOD)
        id += 3u * mod_ui_slot;
    *valp = &TSEL->p[id];
    return track_desc(TSEL, id);
}

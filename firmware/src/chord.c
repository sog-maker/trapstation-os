/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

enum { CH_OFF, CH_DIA3, CH_DIA7, CH_MAJ, CH_MIN, CH_DOM7, CH_MAJ7, CH_MIN7, CH_SUS4, CH_POW, CH_MIN9, CH_HALFDIM, CH_MAJ9, CH_MIN11, CH_MIN13,
       CH_SUS2, CH_ADD9, CH_MADD9 };
#define CH_LAST CH_MADD9
enum { VC_CLOSE, VC_OPEN, VC_INV1, VC_INV2, VC_BASS };
#define CHORD_MAX 4u
#define MCHORD_N 24u

static const int8_t CHORD_SHAPE[CH_LAST - CH_MAJ + 1][CHORD_MAX] = {
    {0, 4, 7, -1}, {0, 3, 7, -1}, {0, 4, 7, 10}, {0, 4, 7, 11}, {0, 3, 7, 10}, {0, 5, 7, -1}, {0, 7, 12, -1},
    {0, 3, 10, 14}, {0, 3, 6, 10}, {0, 4, 11, 14}, {0, 3, 10, 17}, {0, 3, 10, 21},
    {0, 2, 7, -1}, {0, 4, 7, 14}, {0, 3, 7, 14},
};

static uint8_t kb_chord[27][CHORD_MAX], kb_chn[27];
typedef struct { uint8_t id, ch, src, n, note[CHORD_MAX]; } mchord_t;
static mchord_t mchord[MCHORD_N];

static struct { uint8_t root, n, note[CHORD_MAX]; uint16_t mask; uint8_t gen; } chord_last[NTRK];

static int chord_kit(const track_t *t)
{
    const engine_t *e = ENGINES[eng_idx(t->eng_req)];
    return e->oneshot || (e->keys && e->keys(t, 0) >= 0);
}

static uint32_t chord_tones_cfg(uint32_t mode, int32_t *root, int32_t *iv, uint32_t scale, int32_t key)
{
    uint32_t n = 0, i;
    if (mode >= CH_MAJ) {
        for (i = 0; i < CHORD_MAX && CHORD_SHAPE[mode - CH_MAJ][i] >= 0; i++)
            iv[n++] = CHORD_SHAPE[mode - CH_MAJ][i];
        return n;
    }
    {
        uint32_t mask = scale, deg[12], c = 0, at = 0, guard = 12;
        int32_t r = key;
        if (mask == 0xFFFu)
            mask = SCALE_MASK[1];
        while (guard-- && !((mask >> (uint32_t)((*root - r + 120) % 12)) & 1u))
            (*root)--;
        for (i = 0; i < 12u; i++)
            if ((mask >> i) & 1u) {
                if (i == (uint32_t)((*root - r + 120) % 12))
                    at = c;
                deg[c++] = i;
            }
        for (i = 0; i < (mode == CH_DIA7 ? 4u : 3u); i++) {
            uint32_t k = at + 2u * i;
            iv[n++] = (int32_t)deg[k % c] + 12 * (int32_t)(k / c) - (int32_t)deg[at];
        }
        return n;
    }
}

static uint32_t chord_make_cfg(uint32_t root, uint8_t *out, int32_t *rp, uint16_t *maskp, uint32_t mode, uint32_t voic, uint32_t scale, int32_t key, uint32_t vmode)
{
    int32_t iv[CHORD_MAX + 1u], r = (int32_t)root, x;
    uint32_t n, i, j, m = 0;
    uint16_t mask = 0;
    *rp = r;
    *maskp = 0;
    if (!mode || mode > CH_LAST) {
        out[0] = (uint8_t)root;
        return 1;
    }
    n = chord_tones_cfg(mode, &r, iv, scale, key);
    for (i = 0; i < n; i++)
        mask |= (uint16_t)(1u << (iv[i] % 12));
    *rp = r;
    *maskp = mask;
    if (vmode != V_POLY) {
        out[0] = (uint8_t)clamp(r, 0, 127);
        return 1;
    }
    switch (voic) {
    case VC_OPEN:
        if (n >= 3u)
            iv[1] += 12;
        break;
    case VC_INV2:
    case VC_INV1:
        for (j = 0; j < (voic == VC_INV2 ? 2u : 1u); j++) {
            x = iv[0] + 12;
            for (i = 1; i < n; i++)
                iv[i - 1u] = iv[i];
            iv[n - 1u] = x;
        }
        break;
    case VC_BASS:
        if (mode == CH_MIN9 || mode == CH_MAJ9 || mode == CH_MIN11 || mode == CH_MIN13) { iv[0] = -12; break; }
        if (n == CHORD_MAX) {
            iv[2] = iv[3];
            n--;
        }
        for (i = n; i > 0; i--)
            iv[i] = iv[i - 1u];
        iv[0] = -12;
        n++;
        break;
    default:
        break;
    }
    for (i = 1; i < n; i++)
        for (j = i; j > 0 && iv[j - 1u] > iv[j]; j--) {
            x = iv[j];
            iv[j] = iv[j - 1u];
            iv[j - 1u] = x;
        }
    for (i = 0; i < n && m < CHORD_MAX; i++) {
        x = r + iv[i];
        if (x < 0 || x > 127 || (m && out[m - 1u] == (uint8_t)x))
            continue;
        out[m++] = (uint8_t)x;
    }
    if (!m) {
        out[0] = (uint8_t)clamp(r, 0, 127);
        return 1;
    }
    return m;
}

static uint32_t chord_make_as(const track_t *t,uint32_t root,uint8_t *out,int32_t *rp,uint16_t *maskp,uint32_t mode)
{
    return chord_make_cfg(root,out,rp,maskp,chord_kit(t)?CH_OFF:mode,
                         (uint32_t)t->p[P_VOIC],scale_mask(t),t->p[P_ROOT],trk_vmode(t));
}

static uint32_t chord_make(const track_t *t, uint32_t root, uint8_t *out, int32_t *rp, uint16_t *maskp)
{
    return chord_make_as(t, root, out, rp, maskp, (uint32_t)t->p[P_CHRD]);
}

static uint32_t chord_build(track_t *t, uint32_t root, uint8_t *out)
{
    int32_t r;
    uint16_t mask;
    uint32_t m = chord_make(t, root, out, &r, &mask), i, k = trk_index(t);
    if (m > 1u) {
        chord_last[k].root = (uint8_t)clamp(r, 0, 127);
        chord_last[k].mask = mask;
        chord_last[k].n = (uint8_t)m;
        for (i = 0; i < CHORD_MAX; i++)
            chord_last[k].note[i] = i < m ? out[i] : 0u;
        chord_last[k].gen++;
    }
    return m;
}

static mchord_t *mchord_of(uint32_t ch, uint32_t src, uint32_t id)
{
    uint32_t i;
    for (i = 0; i < MCHORD_N; i++)
        if (mchord[i].id == id && mchord[i].ch == ch && mchord[i].src == src)
            return &mchord[i];
    return 0;
}

static int mchord_held(uint32_t id, uint32_t note)
{
    uint32_t i, j;
    for (i = 0; i < MCHORD_N; i++)
        if (mchord[i].id == id)
            for (j = 0; j < mchord[i].n; j++)
                if (mchord[i].note[j] == note)
                    return 1;
    return 0;
}

static void mchord_forget(uint32_t track)
{
    uint32_t i;
    for (i = 0; i < MCHORD_N; i++)
        if (mchord[i].id == track + 1u)
            mchord[i].id = 0;
}

/* SPDX-License-Identifier: GPL-3.0-only */
/* Root follow adapted from Skiff 2.2, firmware/src/follow.c (db2ce89;
 * GPL-3.0-only, Felucca by Leo Kuroshita / Huegelton Instruments).
 * TRAPSTATION: source/root only, allow 808 KICK as follower, preserve all P_* IDs.
 * Event-rate work, no per-sample DSP. Keep note-on mapping until its exact off. */
typedef struct { uint16_t pc; uint8_t lo, root, held; } fol_src_t;
static fol_src_t fol_src[NTRK];
static uint8_t fol_map[NTRK][128] __attribute__((section(".pool")));

static uint32_t fol_root(uint32_t pc, uint32_t lo)
{
    uint32_t i, best = lo % 12u, top = 0;
    for (i = 0; i < 12u; i++) {
        uint32_t r = (lo+i)%12u, m, s;
        if (!((pc >> r)&1u)) continue;
        m = (pc >> r | pc << (12u-r)) & 0xFFFu;

        for (uint32_t shape=0; shape<=(uint32_t)(CH_MIN13-CH_MAJ); shape++) {
            uint32_t sm=0;
            for (uint32_t j=0; j<CHORD_MAX && CHORD_SHAPE[shape][j]>=0; j++)
                sm |= 1u << ((uint32_t)CHORD_SHAPE[shape][j]%12u);
            if (sm==m) return r;
        }
        s = ((m>>7)&1u)*3u + (((m>>3)|(m>>4))&1u)*5u +
            (((m>>10)|(m>>11))&1u)*2u + ((m>>2)&1u) + ((m>>6)&1u);
        if (s > top) { top = s; best = r; }
    }
    return best;
}
static void fol_add(uint32_t k, uint32_t note, int fresh)
{
    fol_src_t *s = &fol_src[k];
    if (fresh || !s->pc) { s->pc = 0; s->lo = 127; }
    s->pc |= (uint16_t)(1u << (note%12u));
    if (note < s->lo) s->lo = (uint8_t)note;
    s->root = (uint8_t)fol_root(s->pc, s->lo);
}
static __attribute__((noinline)) void fol_step(const track_t *t, const uint8_t *nn, uint32_t n)
{
    uint32_t i;
    if (chord_kit(t)) return;
    for (i=0; i<n; i++) fol_add(trk_index(t), nn[i], !i);
}
static __attribute__((noinline)) void fol_key(const track_t *t, uint32_t note, int on)
{
    fol_src_t *s = &fol_src[trk_index(t)];
    if (!on) { if (s->held) s->held--; return; }
    if (chord_kit(t)) return;
    fol_add(trk_index(t), note, !s->held);
    if (s->held < 255u) s->held++;
}
static uint32_t fol_note(const track_t *t, uint32_t note)
{
    uint32_t k = trk_index(t), l = (uint32_t)chord_cfg[k][CG_FOLLOW] - 1u;
    int32_t shift;

    if (l >= NTRK || l == k || chord_cfg[l][CG_FOLLOW] || !fol_src[l].pc ||
        chord_kit(&trk[l]) || (chord_kit(t) && !an_is_kick(t))) return note;
    shift = ((int32_t)fol_src[l].root - t->p[P_ROOT] + 120) % 12;
    if (shift > 6) shift -= 12;
    return (uint32_t)clamp((int32_t)note + shift, 0, 127);
}
static __attribute__((noinline)) uint32_t fol_on(track_t *t, uint32_t note)
{
    uint8_t *m = fol_map[trk_index(t)];
    uint32_t x = fol_note(t, note & 127u);
    if (m[note&127u] && m[note&127u] != x+1u) trk_note_off(t, note);
    if (chord_cfg[trk_index(t)][CG_FOLLOW] || m[note&127u]) m[note&127u] = (uint8_t)(x+1u);
    return x;
}
static __attribute__((noinline)) uint32_t fol_off(track_t *t, uint32_t note)
{
    uint8_t *m = fol_map[trk_index(t)];
    uint32_t o = m[note&127u], i;
    if (!o) return note;
    m[note&127u] = 0;
    for (i=0; i<128u; i++) if (m[i] == o) return 255u;
    return o-1u;
}
static void fol_clear_source(uint32_t k) { memset(&fol_src[k], 0, sizeof fol_src[k]); }
static __attribute__((noinline)) void fol_drop(const track_t *t)
{
    uint32_t k = trk_index(t);
    memset(fol_map[k], 0, sizeof fol_map[k]);
    fol_clear_source(k);
}
static __attribute__((noinline)) uint32_t fol_order(void)
{
    uint32_t i, l, led=0, ord=0, at=0, pass;
    for (i=0; i<NTRK; i++)
        if ((l=(uint32_t)chord_cfg[i][CG_FOLLOW]-1u) < NTRK && l != i) led |= 1u<<l;
    for (pass=0; pass<2u; pass++)
        for (i=0; i<NTRK; i++) if (((led>>i)&1u) != pass) { ord |= i<<at; at+=4u; }
    return ord;
}

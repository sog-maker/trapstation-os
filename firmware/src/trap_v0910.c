/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION contributors */

static step_t qf_orig[NSTEP];
static step_t qf_blk[4];
static uint8_t qf_trk, qf_pos, qf_n;
static int16_t qf_semi, qf_vpos;

static void qf_before(const track_t *t)
{
    memcpy(qf_orig, t->step, sizeof qf_orig);
}
static void qf_after(const track_t *t)
{
    uint32_t len = (uint32_t)clamp(t->p[P_SLEN], 1, NSTEP);
    qf_n = (uint8_t)(len < 4u ? len : 4u);
    qf_pos = (uint8_t)(len - qf_n);
    memcpy(qf_blk, &t->step[qf_pos], qf_n * sizeof(step_t));
    qf_trk = (uint8_t)(trk_index(t) + 1u);
    qf_semi = 0;
}
static int qf_valid(const track_t *t)
{
    uint32_t i;
    if (!qf_trk || qf_trk != trk_index(t) + 1u)
        return 0;
    for (i = 0; i < NSTEP; i++) {
        const step_t *want = i >= qf_pos && i < (uint32_t)qf_pos + qf_n ? &qf_blk[i - qf_pos] : &qf_orig[i];
        if (memcmp(&t->step[i], want, sizeof(step_t)))
            return 0;
    }
    return 1;
}
static uint32_t qf_at(void)
{
    return qf_valid(&trk[song.sel % NTRK]) ? (qf_pos + 1u) | (uint32_t)qf_n << 8 : 0u;
}
static int pr_valid(void);
static uint32_t pr_s0(void);
static int16_t *qf_value(uint32_t id)
{
    const track_t *t = TSEL;
    uint32_t s = pr_s0() + pr_sel - 1u, q;
    if (fxl_sel || !pr_valid()) {
        qf_vpos = 0;
        qf_semi = 0;
        return id == PH_QPITCH ? &qf_semi : &qf_vpos;
    }
    qf_vpos = (int16_t)(s + 1u);
    qf_semi = 127;
    for (q = 0; q < t->step[s].n && q < 4u; q++)
        if ((pr_selm == 0x0Fu || ((pr_selm >> q) & 1u)) && t->step[s].note[q] < qf_semi)
            qf_semi = t->step[s].note[q];
    return id == PH_QPITCH ? &qf_semi : &qf_vpos;
}


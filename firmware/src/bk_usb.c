/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 TRAPSTATION OS; based on BBL JAM BOX (Blair) and Felucca (Leo Kuroshita, Hügelton Instruments) */

#define BK_BEGIN 0x50u
#define BK_STAT 0x51u
#define BK_READ 0x52u
#define BK_PUT 0x53u
#define BK_COMMIT 0x54u
#define BK_END 0x55u
#define BK_VER 1u
#define BK_CHUNK 512u
#define BK_SMP 0x40u
#define BK_SREGION (SMP_TOP - SMP_BASE)
#define BK_SESS_MS 10000u
_Static_assert(7u + 6u + BK_CHUNK + 4u <= OTA_FRAME_MAX, "a PUT must fit ota_dec");

static uint8_t bk_msg[7u + 6u + BK_CHUNK], bk_wire[2u + ((7u + 6u + BK_CHUNK) * 8u + 6u) / 7u];
#if FELUCCA_MIRROR
#define bk_stage mir_buf
_Static_assert(MIR_N >= ST_PAYLOAD_MAX, "the mirror's ring stages a restore");
#else
static uint8_t bk_stage[ST_PAYLOAD_MAX] __attribute__((aligned(4)));
#endif
static uint8_t bk_mode, bk_id = 0xFFu;
static uint32_t bk_len, bk_commits;

static int bk_obj(uint32_t id)
{
    return id == OBJ_SETTINGS || id == OBJ_AUTOSAVE || (id >= OBJ_GB0 && id <= OBJ_JAM2);
}

static int bk_send(uint32_t cmd, const uint8_t *hd, uint32_t nh, const uint8_t *data, uint32_t nd)
{
    uint32_t n = nh + nd, i, s = 0, w;
    bk_msg[0] = 0;
    bk_msg[1] = 0x59;
    bk_msg[2] = (uint8_t)cmd;
    bk_msg[3] = (uint8_t)n;
    bk_msg[4] = (uint8_t)(n >> 8);
    bk_msg[5] = 0;
    for (i = 0; i < n; i++) {
        uint8_t b = i < nh ? hd[i] : data[i - nh];
        bk_msg[6 + i] = b;
        s += b;
    }
    bk_msg[6 + n] = (uint8_t)~s;
    bk_wire[0] = 0xF0;
    w = 1u + ota_pack7(bk_msg, 7u + n, bk_wire + 1);
    bk_wire[w++] = 0xF7;
    return ota_wire_send(bk_wire, w);
}

static int bk_handle(const uint8_t *m, uint32_t d)
{
    uint32_t cmd = m[2], n = d - 7u, id = n ? m[6] : 0xFFu, off = n >= 5u ? ota_rd32(m + 7) : 0u, rc = 0, i, k;
    const uint8_t *src = 0;
    uint8_t r[12];
    for (i = 0; i < sizeof r; i++)
        r[i] = 0;
    r[1] = (uint8_t)id;
    switch (cmd) {
    case BK_BEGIN:
        r[1] = BK_VER;
        ota_wr16(r + 2, BK_CHUNK);
        ota_wr32(r + 4, BK_SREGION);
        bk_send(cmd, r, 8, 0, 0);
        return 0;
    case BK_STAT:
        if (bk_mode)
            rc = 3;
        else if (n != 1u)
            rc = 1;
        else if (id == BK_SMP)
            ota_wr32(r + 2, BK_SREGION);
        else if (!bk_obj(id))
            rc = 1;
        else {
            int len = st_load(id, st_buf, ST_PAYLOAD_MAX);
            bk_id = 0xFFu;
            if (len <= 0)
                rc = 2;
            else {
                bk_id = (uint8_t)id;
                bk_len = (uint32_t)len;
                ota_wr32(r + 2, bk_len);
                ota_wr32(r + 6, st_crc32(st_buf, bk_len));
            }
        }
        r[0] = (uint8_t)rc;
        bk_send(cmd, r, 10, 0, 0);
        return 0;
    case BK_READ:
        k = n == 7u ? ota_rd16(m + 11) : 0u;
        if (bk_mode)
            rc = 3;
        else if (!k || k > BK_CHUNK)
            rc = 1;
        else if (id == BK_SMP) {
            if (off >= BK_SREGION || k > BK_SREGION - off)
                rc = 1;
            else
                src = SMP_XIP(SMP_BASE + off);
        } else if (id != bk_id || off >= bk_len || k > bk_len - off)
            rc = 1;
        else
            src = st_buf + off;
        r[0] = (uint8_t)rc;
        ota_wr32(r + 2, off);
        bk_send(cmd, r, 6, src, rc ? 0u : k);
        return 0;
    case BK_PUT:
        k = n >= 5u ? n - 5u : 0u;
        if (!bk_mode)
            rc = 3;
        else if (!k || k > BK_CHUNK || off >= ST_PAYLOAD_MAX || k > ST_PAYLOAD_MAX - off)
            rc = 1;
        else {
            for (i = 0; i < k; i++)
                bk_stage[off + i] = m[11 + i];
            bk_id = 0xFFu;
        }
        r[0] = (uint8_t)rc;
        ota_wr32(r + 2, off);
        bk_send(cmd, r, 6, 0, 0);
        return 0;
    case BK_COMMIT:
        if (!bk_mode)
            rc = 3;
        else if (n != 9u || !bk_obj(id) || !off || off > ST_PAYLOAD_MAX)
            rc = 1;
        else if (st_crc32(bk_stage, off) != ota_rd32(m + 11))
            rc = 2;
        else if (st_save(id, bk_stage, off) != 0)
            rc = 4;
        else
            bk_commits++;
        r[0] = (uint8_t)rc;
        bk_send(cmd, r, 2, 0, 0);
        return 0;
    default:
        r[1] = (uint8_t)bk_commits;
        bk_send(cmd, r, 2, 0, 0);
        return 1;
    }
}

#ifndef BK_NO_SESSION
static void bk_show(const char *line)
{
    lcd_fill(0, 0, 240, 240, T_BG);
    draw_text_box(0, 92, 240, &AF_M, bk_mode ? "RESTORE" : "BACKUP", T_TEXT, 1);
    draw_text_box(0, 124, 240, &AF_S, line, T_THEME, 1);
}

static void bk_pct(char *b, const char *t, uint32_t pct)
{
    uint32_t k = 0;
    while (*t)
        b[k++] = *t++;
    if (pct >= 100u)
        b[k++] = '1';
    if (pct >= 10u)
        b[k++] = (char)('0' + pct / 10u % 10u);
    b[k++] = (char)('0' + pct % 10u);
    b[k++] = '%';
    b[k] = 0;
}

static void bk_session(uint32_t d)
{
    uint32_t last, done, shown = 0xFFFFu;
    char b[20];
    bk_mode = ota_dec[6] == 1u;
#if FELUCCA_MIRROR
    mir_on = 0;
#endif
    bk_commits = 0;
    bk_id = 0xFFu;
    panic_req = (1u << NTRK) - 1u;
    fm1_irq_off();
    if (song.playing)
        seq_stop();
    fm1_irq_on();
    scr_wake_now();
    bk_show(bk_mode ? "WRITING" : "READING");
    done = bk_handle(ota_dec, d);
    for (last = ota_now_ms(); !done && ota_now_ms() - last < BK_SESS_MS;) {
        ota_idle();
        d = ota_take();
        if (!d)
            continue;
        last = ota_now_ms();
        if (d == 7u && ota_dec[2] == 0x11) {
            ota_reply_identity();
            continue;
        }
        if (ota_dec[2] < BK_BEGIN || ota_dec[2] > BK_END)
            continue;
        if (ota_dec[2] == BK_READ && d == 14u && ota_dec[6] == BK_SMP) {
            uint32_t pct = ota_rd32(ota_dec + 7) / (BK_SREGION / 100u);
            if (pct > 100u)
                pct = 100u;
            if (pct / 5u != shown) {
                shown = pct / 5u;
                bk_pct(b, "SAMPLES ", pct);
                bk_show(b);
            }
        } else if (ota_dec[2] == BK_STAT || ota_dec[2] == BK_COMMIT) {
            uint32_t c = bk_commits + (ota_dec[2] == BK_COMMIT);
            str_cpy(b, bk_mode ? "OBJECT " : "READING", sizeof b);
            if (bk_mode) {
                b[7] = (char)('0' + c / 10u % 10u);
                b[8] = (char)('0' + c % 10u);
                b[9] = 0;
            }
            bk_show(b);
        }
        done = bk_handle(ota_dec, d);
    }
    if (bk_commits) {
        uint32_t t0 = ota_now_ms();
        bk_show("RESTARTING");
        while (ota_now_ms() - t0 < 600u)
            ota_idle();
        bootguard.pending = 0;
        usb_detach();
        fm1_delay_ms(30);
        fm1_reboot();
    }
    lcd_fill(0, 0, 240, 240, T_BG);
    ui_message(done ? (bk_mode ? "NOTHING RESTORED" : "BACKUP DONE") : "BACKUP STOPPED");
    ui.force = 1;
}
#endif

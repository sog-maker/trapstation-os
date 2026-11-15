/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Blair (blairysbeatlab), BBL JAM BOX */

#define SMP_REGION (SMP_TOP - SMP_BASE)
#define SMP_SESS_MS 10000u
enum { SMP_BEGIN = 0x60, SMP_SUM, SMP_WRITE, SMP_END };

static int smp_handle(const uint8_t *m, uint32_t d)
{
    uint32_t cmd = m[2], n = d - 7u, off = n >= 4u ? ota_rd32(m + 6) : 0xFFFFFFFFu, rc = 0;
    const uint8_t *b = m + 10;
    uint8_t r[12];
    uint32_t i;
    for (i = 0; i < sizeof r; i++)
        r[i] = 0;
    switch (cmd) {
    case SMP_BEGIN:
        ota_wr32(r + 1, SMP_CAP);
        ota_send_msg(cmd, r, 5);
        return 0;
    case SMP_SUM:
        if (n != 4u || off >= SMP_REGION || (off & 0xFFFu))
            rc = 1;
        else
            ota_wr32(r + 5, st_crc32(SMP_XIP(SMP_BASE + off), 0x1000u));
        r[0] = (uint8_t)rc;
        ota_wr32(r + 1, off);
        ota_send_msg(cmd, r, 9);
        return 0;
    case SMP_WRITE:
        n -= 4u;
        if (d < 12u || n > 512u || (off & 0xFFu) || off >= SMP_REGION || n > SMP_REGION - off)
            rc = 1;
        else if (!(off & 0xFFFu) && st_erase(SMP_BASE + off))
            rc = 2;
        else if (st_prog(SMP_BASE + off, b, n))
            rc = 3;
        else if (memcmp(SMP_XIP(SMP_BASE + off), b, n))
            rc = 4;
        r[0] = (uint8_t)rc;
        ota_wr32(r + 1, off);
        ota_send_msg(cmd, r, 5);
        return 0;
    default:
        smp_scan();
        r[0] = smp_scan_ok ? 0 : 5;
        r[1] = smp_user;
        ota_wr32(r + 2, smp_scan_ok ? smp_u32(SMP_XIP(SMP_BASE + 8)) : 0u);
        ota_send_msg(cmd, r, 6);
        return 1;
    }
}

static void smp_show(uint32_t pct)
{
    char b[16];
    uint32_t k = 0;
    lcd_fill(0, 0, 240, 240, T_BG);
    draw_text_box(0, 92, 240, &AF_M, "SAMPLES", T_TEXT, 1);
    str_cpy(b, "LOADING ", sizeof b);
    while (b[k])
        k++;
    if (pct >= 100u)
        b[k++] = '1';
    if (pct >= 10u)
        b[k++] = (char)('0' + pct / 10u % 10u);
    b[k++] = (char)('0' + pct % 10u);
    b[k++] = '%';
    b[k] = 0;
    draw_text_box(0, 124, 240, &AF_S, b, T_THEME, 1);
}

static void smp_session(uint32_t d)
{
    uint32_t last, shown = 0xFFu, done;
    panic_req = (1u << NTRK) - 1u;
    fm1_irq_off();
    if (song.playing)
        seq_stop();
    smp_n = 0;
    smp_gen++;
    fm1_irq_on();
    scr_wake_now();
    smp_show(0);
    done = smp_handle(ota_dec, d);
    for (last = ota_now_ms(); !done && ota_now_ms() - last < SMP_SESS_MS;) {
        ota_idle();
        d = ota_take();
        if (!d)
            continue;
        last = ota_now_ms();
        if (d == 7u && ota_dec[2] == 0x11) {
            ota_reply_identity();
            continue;
        }
        if (ota_dec[2] < SMP_BEGIN || ota_dec[2] > SMP_END)
            continue;
        if (ota_dec[2] == SMP_WRITE && d >= 11u) {
            uint32_t pct = (ota_rd32(ota_dec + 6) >> 12) * 100u / (SMP_REGION >> 12);
            if (pct / 5u != shown) {
                shown = pct / 5u;
                smp_show(pct);
            }
        }
        done = smp_handle(ota_dec, d);
    }
    if (!done)
        smp_scan();
    lcd_fill(0, 0, 240, 240, T_BG);
    ui_message(smp_user ? "SAMPLES LOADED" : "NO SAMPLES");
    ui.force = 1;
}

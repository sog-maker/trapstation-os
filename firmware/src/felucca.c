/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

#ifndef FELUCCA_FLASH
#define FELUCCA_FLASH 1
#endif
#ifndef FELUCCA_OTA
#define FELUCCA_OTA 1
#endif
#ifndef FELUCCA_OTA_DRYRUN
#define FELUCCA_OTA_DRYRUN 0
#endif
#ifndef FELUCCA_CDC
#define FELUCCA_CDC 1
#endif
#ifndef FELUCCA_UAC
#define FELUCCA_MIRROR 1
#define FELUCCA_UAC 1
#endif
#ifndef FELUCCA_UAC_TONE
#define FELUCCA_UAC_TONE 0
#endif
#ifndef FELUCCA_UART
#define FELUCCA_UART 1
#endif
#ifndef FELUCCA_ID
#define FELUCCA_ID "FM-1_500"
#endif
#ifndef FELUCCA_CPU2
#define FELUCCA_CPU2 1

#endif
#define FM1_CPU1_USED FELUCCA_CPU2

#ifndef FELUCCA_VERSION
#define FELUCCA_VERSION "DEV"

#endif
#if FELUCCA_OTA && !FELUCCA_FLASH
#error "FELUCCA_OTA needs FELUCCA_FLASH"
#endif

#include <stdint.h>
#include "fm1_time.h"
#include "fm1_sys.h"
#include "fm1_irq.h"
#include "fm1_guard.h"
#if FELUCCA_CPU2
#include "fm1_cpu1.h"
#define C1_ON() fm1_c1_on
#define C1_RUN(fn, arg) fm1_cpu1_run(fn, arg)
#define C1_WAIT() fm1_cpu1_wait()
#endif
#include "fm1_input.h"
#include "fm1_timer.h"
#include "fm1_audio.h"
#include "fm1_adc.h"
#include "fm1_lcd_hw.h"
#include "felucca_tables.h"

#include "libc.c"
#include "lcd.c"
#include "gfx.c"

#include "core.h"
#include "engines.c"
#include "params.c"
#include "mod.c"
#include "voice.c"
#include "fx.c"

#include "usb.c"
#if FELUCCA_UART
#include "midi_uart.c"
#endif
#include "seq.c"
#include "audio.c"

#include "panel.c"
#include "ui.c"
#include "icons.c"
#include "ui_graph.c"
#include "ui_gb.c"
#include "ui_draw.c"
#include "ui_input.c"
#include "ui_buttons.c"

#if FELUCCA_FLASH
#include "fm1_flash.h"
#include "storage_hw.c"
#include "storage.c"
#endif
#include "persist.c"
#include "trap_kits.c"
#include "trap_browse.c"
#include "gb_store.c"
#if FELUCCA_OTA
#define FELUCCA_SMP_USB 1
#define FELUCCA_BK_USB 1
#include "ota.c"
#include "ota_hw.c"
#include "smp_usb.c"
#if FELUCCA_MIRROR
#include "mirror.c"
#endif
#include "bk_usb.c"
#endif
#if FELUCCA_CDC

static void cdc_task(void)
{
    while (ci_r != ci_w) {
        RING_PUBLISH();
#if FELUCCA_MIRROR
        mir_serial(cdc_in[ci_r % CI_N]);
#endif
        ci_r++;
    }
}
#endif
#include "master.c"
#include "main.c"

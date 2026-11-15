# TRAPSTATION OS – Lizenzen (Entwurf)

> Kein Rechtsrat. Vor dem ersten öffentlichen Release prüfen (Roadmap Phase 10).

## Code: GPL-3.0-only

TRAPSTATION OS wird ein veränderter Fork von **BBL JAM BOX** (GPL-3.0-only), der wiederum auf **Felucca**
(GPL-3.0-only, Leo Kuroshita / Hügelton Instruments) aufbaut. Damit steht der gesamte Firmware-, Tool- und
Web-Code unter **GPL-3.0-only** (Volltext: `LICENSE`). Wer Firmware weitergibt, muss den vollständigen Quellcode mitgeben.

- Übernommene Dateien behalten ihre Copyright-Zeilen und SPDX-Header.
- Neue Dateien bekommen: `SPDX-License-Identifier: GPL-3.0-only` und `Copyright (C) 2026 TRAPSTATION OS contributors`.
- Teile mit eigener Lizenz (z. B. MIT-Dateien aus Felucca-Engines, Fonts) behalten diese; Texte kommen nach `LICENSES/`.

## Nicht unter der GPL

- **Logo, Name und Artwork** in `assets/logo/` und das daraus generierte Boot-Logo: (C) 2026 M. Schlensog,
  alle Rechte vorbehalten. Forks müssen sie ersetzen.
- Das Boot-Logo von BBL JAM BOX (`assets/logo/MEE.jpg`, (C) Blair, alle Rechte vorbehalten) ist nicht frei. Es lag nur für den unveränderten Basis-Build im Baum und wurde am 2026-11-06 (Phase 2) samt `firmware/src/splash_logo.h` und `tools/gen_splash_logo.py` entfernt; Startbild und Web-Seiten zeigen jetzt das TRAPSTATION-Logo (`firmware/src/trap_logo.h`, `assets/logo/trapstation-web.png`, beide aus `assets/logo/trapstation-emblem.png` erzeugt). Builds vor diesem Stand (`builds/*_release-0.6.1-beta`, `builds/*_2d7b06d_dev`) enthalten Blairs Logo und werden nicht weitergegeben. Der unten angehängte BBL-Text erwähnt das Logo noch, weil er unverändert bleibt.

## Samples

Nur selbst synthetisierte, selbst aufgenommene oder CC0-Samples. Keine Samples aus kommerziellen Geräten
(Roland TR-808, Boss DR-660, E-mu SP-1200) oder fremden Tracks. Herkunft jedes Samples in `sounds/samples/ATTRIBUTION.md`.

## Offene Punkte

- [ ] Markenrecht: Name „TRAPSTATION“ und die Vierfarb-Ring-Optik erinnern an eine bekannte Konsolenmarke – vor
      öffentlichem Release prüfen.
- [ ] Lizenzen der übernommenen Fonts klären (Phase 1).
- [ ] Bei Übernahme von x0x-/Skiff-Modulen deren Drittlizenzen (`LICENSES/`, `LICENSING.md`) nachziehen.

---

## Geerbt von BBL JAM BOX (unverändert, Stand `v0.6.1-beta`)

Der folgende Text ist BBLs `LICENSING.md`. Er beschreibt die Lizenzen der übernommenen Dateien und gilt für sie
weiter. Die Pfade beziehen sich auf diesen Baum.

# BBL JAM BOX licensing

BBL JAM BOX is free software, licensed under the GNU General Public License, version 3 only
(`GPL-3.0-only`, full text in `LICENSE`). It is a **modified version of Felucca** by Leo Kuroshita
(@kurogedelic), Hügelton Instruments, which is under the same licence. A few bundled or ported parts keep
their own licences; they are listed below, and their licence texts are in `LICENSES/`.

Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments (Felucca)
Copyright (C) 2026 Blair (blairysbeatlab) (the BBL JAM BOX changes)

## This fork

This tree started from Felucca 1.6.1 (commit `a53ecf4`). Everything since is the BBL JAM BOX's work:
most of Felucca was removed (its engines other than the FM one, the menus, the web editor, the samples,
the tests) and the groovebox built on the rest. The git history records every change and its date. Files
written for the groovebox carry `Copyright (C) 2026 Blair (blairysbeatlab), BBL JAM BOX`; files
changed from Felucca's keep Leo Kuroshita's copyright line.

**Not under the GPL:** the logo on the power-on screen (`assets/logo/`, built into
`firmware/src/splash_logo.h` by `tools/gen_splash_logo.py`) is (C) 2026 Blair (blairysbeatlab), all rights
reserved. If you distribute a modified version, replace it with your own.

## What is GPL-3.0-only

Every file in this tree that carries an `SPDX-License-Identifier: GPL-3.0-only` header:

- the firmware: `firmware/` (app, HAL, update loader), except the MIT files below
- the build script and tools: `build.sh`, `tools/`, except `tools/gen_engine_tables.py` (MIT, below)
- the web pages (the installer, the Samples page, the front page and the browser emulator `web/emu/`),
  except the DotGothic16 font (below)

You may use, study, change and share it under the GPL. If you distribute it, or firmware derived from it,
you must also give your recipients its complete corresponding source under the same licence.

## Hügelton Instruments' work in this tree

| What | Licence | Where |
| --- | --- | --- |
| Felucca: the firmware, tools and web pages this fork is built on | GPL-3.0-only | `LICENSE` |
| The engine interface, the shared DSP helpers and the generator of their tables, from Felucca-Engines (<https://github.com/hugelton/Felucca-Engines>) | MIT | `firmware/src/engine.h`, `firmware/src/dsp.c`, `tools/gen_engine_tables.py`, `LICENSES/MIT-Felucca-Engines.txt` |
| The Fukiai icon font (<https://github.com/hugelton/Fukiai>): some of the firmware's icons, rasterised at build time by `tools/gen_aa_icons.py` | MIT | `assets/fonts/fukiai.ttf`, `LICENSES/MIT-Fukiai.txt`, `assets/fonts/FUKIAI-LICENSE.txt` |

The firmware built with the MIT files is GPL-3.0-only as a whole.

## Third-party material

| What | Licence | Where |
| --- | --- | --- |
| BLB Pixel 7 Chunky font by blairysbeatlab: the UI text, rasterised into the firmware at build time (`tools/gen_aa_font.py` preset bbl-pixel; the generated tables are not offered as a font) | SIL OFL 1.1 | `assets/fonts/BLBPixel7-Chunky.ttf`, `LICENSES/OFL-BLBPixel7.txt` |
| Inter Tight font by The Inter Project Authors: the UI text of earlier builds (still the `inter-tight` preset), rasterised into the firmware at build time (`tools/gen_aa_font.py`; the generated tables are not offered as a font, and the font declares no Reserved Font Name) | SIL OFL 1.1 | `assets/fonts/InterTight[wght].ttf`, `LICENSES/OFL-InterTight.txt` (also `assets/fonts/OFL.txt`) |
| The second core: its start, mailbox and guards from X0X by charlesvestal (<https://github.com/charlesvestal/fm1-x0x>, with the dual-core findings of Melodee by keremimo, <https://github.com/keremimo/melodee>); its audio split, measurements and hardening after Jangada by zednaked (<https://github.com/zednaked/jangada>); each credited in its files | GPL-3.0 | `firmware/hal/fm1_cpu1.{h,S}`, `firmware/src/fx.c` |
| X0X by charlesvestal, a Felucca fork: the design of the browser emulator; its files credit it in their headers | GPL-3.0 | `web/emu/` |
| In `BBL Fader.exe` (the phone fader app; its own code, `web/fader/bbl_fader.py`, is GPL-3.0-only): segno (QR codes), the Python runtime and Tcl/Tk, packed by PyInstaller | BSD-3-Clause; PSF; Tcl/Tk's BSD-style licence; PyInstaller: GPL-2.0-or-later with its bootloader exception | `LICENSES/BSD-segno.txt`, `LICENSES/PSF-Python.txt`, `LICENSES/BSD-TclTk.txt`, `LICENSES/GPL-PyInstaller.txt` |
| DotGothic16 font by The DotGothic16 Project Authors (<https://github.com/fontworks-fonts/DotGothic16>): the browser emulator's knob labels (cut to printable ASCII; the font declares no Reserved Font Name) | SIL OFL 1.1 | `web/emu/fonts/DotGothic16-subset.woff`, `LICENSES/OFL-DotGothic16.txt` (also `web/emu/fonts/OFL.txt`) |
| JieLi AC79 SDK by JieLi Technology: three of its files go into every `.fwsc` package (below); none are in this tree | Apache-2.0 | `LICENSES/Apache-2.0.txt` |

`LICENSES/` also still holds the texts for parts of Felucca this fork removed (DaisySP, Rings, msfa);
they are kept so the folder matches Felucca's, and no code under them is in this build.

## JieLi SDK files in the packages (Apache-2.0)

A `.fwsc` package made by `tools/build.py` (with `tools/fm1pkg_make.py`; this is the package the web
installer installs) holds three unmodified files from the JieLi AC79 SDK
(<https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK>, `cpu/wl82/tools/`). They are read from your SDK checkout at
build time (see BUILDING.md); no SDK files are in this tree.

| File in the package | What it is |
| --- | --- |
| `uboot.boot` | the first-stage boot loader (SPL) |
| `cfg_tool.bin` | the chip configuration block |
| `eq_cfg_hw.bin` | the default hardware EQ table |

They are licensed under the Apache License, Version 2.0 (`LICENSES/Apache-2.0.txt`), not under the GPL, and
their copyright stays with JieLi Technology. The SDK has no NOTICE file. Apache-2.0 files may be distributed
together with GPL-3.0 code; the firmware itself stays GPL-3.0-only.

Every distribution of a package carries the licence texts with it: the release folder
(`tools/build.py --release`) and the site (`web/make_site.py`, next to the package in `firmware/` and linked
from the installer page).

## No vendor material

No M-VAVE material is part of this firmware. It links no vendor code and contains no vendor data, and the
packages hold only our firmware and the three SDK files above (`web/make_site.py` refuses any other package).
The installer's "Return to official V15" holds only the official file's size and SHA-256: you select the
official firmware file you downloaded yourself (M-VAVE publishes it as `sdk/firmware/FM-1_015.fwsc` in
<https://github.com/m-vave/fm1_open>), and it is checked and installed in your browser, never uploaded or
redistributed.

## Contributions

Contributions are welcome under GPL-3.0-only.

## Trademarks

"Felucca" and "Hügelton Instruments" are names of Hügelton Instruments. This firmware is based on Felucca;
it is not Felucca and is not made by Hügelton Instruments.

"M-VAVE" and "FM-1" are trademarks of their respective owners. This is independent firmware that runs on
FM-1 hardware. It is not affiliated with, endorsed by or supported by those owners.

## Radio

This firmware never enables the Bluetooth / Wi-Fi radio of the hardware (as Felucca).

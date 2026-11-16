# Credits

TRAPSTATION OS stands on the work of others. Thank you.

| Project | Author | Licence | What TRAPSTATION OS uses |
| --- | --- | --- | --- |
| [BBL JAM BOX](https://github.com/UnkleBlairy/BBL-JAM-BOX) 0.6.1 beta | Blair (blairysbeatlab) | GPL-3.0-only | The base firmware: groovebox workflow, 808 machines, sampler, FM engine changes, DJ filter and comb, sound and kit saves |
| [Felucca](https://github.com/hugelton/Felucca) | Leo Kuroshita, Hügelton Instruments | GPL-3.0-only, MIT (engines) | The foundation: hardware layer, USB, update loader and installer, sequencer, effects and screen code |
| [Felucca-Engines](https://github.com/hugelton/Felucca-Engines), [Fukiai](https://github.com/hugelton/Fukiai) | Hügelton Instruments | MIT | Engine interface and DSP helpers; icon font |
| [Skiff](https://github.com/illuminus5/skiff) | illuminus5 | GPL-3.0-only | Chord follow (root detection), adapted in `firmware/src/trap_follow.c`; an inspiration for the presets, the editor and the track layout |
| [X0X](https://github.com/charlesvestal/fm1-x0x) | charlesvestal | GPL-3.0 | The second core's start, mailbox and guards; an inspiration for the 808 focus and the accent |
| [Melodee](https://github.com/keremimo/melodee) | keremimo | GPL-3.0 | Dual-core findings |
| [Jangada](https://github.com/zednaked/jangada) | zednaked | GPL-3.0 | The second core's audio split and hardening |
| [FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter) | Leo Kuroshita | – | Recovery hardware for a FM-1 that no longer starts |
| [fm1_open](https://github.com/m-vave/fm1_open) | M-VAVE | – | Hardware reference and the official firmware for the way back |
| Fonts: BLB Pixel 7 Chunky (blairysbeatlab), Inter Tight (The Inter Project Authors) | | SIL OFL 1.1 | Screen and web text |
| [JieLi AC79 SDK](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK) | JieLi Technology | Apache-2.0 | Package format and three boot files in every package |

Felucca's contributors, whose work is part of the code this started from: keremimo, ChanceTheMaker, andreahaku,
spinkham, ugai, zednaked, jasonpersinger, renebohne, gospodindilly-ops.

TRAPSTATION OS: sounds, banks, generators, Piano Roll, Performance page, sampler slices, the Workstation design and
artwork by Schlensog Audio Lab.

## The FM6 engine

FM SYN runs on **msfa**, the synthesis core of [Dexed](https://github.com/asb2m10/dexed): Copyright 2012 Google Inc.,
Copyright 2016-2025 Pascal Gauthier, in the integer C port by Leo Kuroshita (@kurogedelic), Hügelton Instruments, under
the Apache License 2.0 ([LICENSES/Apache-2.0-msfa.txt](LICENSES/Apache-2.0-msfa.txt)): `firmware/src/fm6_core.c`. The
voice format helpers in `firmware/src/fm6_format.c` come from the same port's engine file (GPL-3.0-only). Both were
taken from [Skiff](https://github.com/illuminus5/skiff) at commit `0ea36820a548dadcb67d9ed29e03e80b32cf6034`.
TRAPSTATION's changes: the table of powers of two is stored smaller and rebuilt exactly, and the engine is bound to a
TRAPSTATION track (`firmware/src/eng_trap_fm6.c`) with its own pages (`firmware/src/trap_fm6_ui.c`).

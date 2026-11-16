# Developer notes

For people who want to build TRAPSTATION OS themselves or work on it. To install and use it, see the
[README](../../README.md) and [INSTALL.md](../../INSTALL.md).

## What is where

| Folder | What |
| --- | --- |
| `firmware/` | The firmware: `src/` (app), `hal/` (FM-1 hardware layer), `loader/` (update loader) |
| `web/` | The installer site: front page, installer, samples page, screen mirror, and `make_site.py` that builds it |
| `tools/` | The build (`build.py`), the generators it runs, the package maker, the site and repository helpers |
| `assets/` | Fonts, icons, palette and the screen artwork the build turns into firmware tables; the logo |
| `docs/` | User documentation: `manual/`, `cheatsheet/`, `design/`, and these notes |
| `LICENSES/` | Licence texts of the parts with their own licence |

## Requirements

The build runs on **Linux x86-64**, or Ubuntu under WSL on Windows.

- Python 3 with Pillow and fontTools: `pip3 install Pillow fonttools`
- The JieLi Linux toolchain (clang 4.0.1 for pi32v2): `tools/get_toolchain.sh` installs it to `~/.jieli/toolchain`
- The JieLi AC79 SDK (Apache-2.0); the package uses three of its files:

  ```
  git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
      https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK
  ```

Other locations: `JIELI_TOOLCHAIN=… AC79_SDK=… ./build.sh`. The paths on this page are examples: adjust them to your
machine. Under WSL a clone on the Windows side is reached as `/mnt/c/Users/<you>/…`; a clone in the Linux home (`~/…`)
builds much faster.

## Build

```
./build.sh --release 1.1.0.1
```

This makes `build/felucca-1.1.0.1.fwsc` (identity `FM-1_511`) and `build/release-1.1.0.1/` with the package, its
`SHA256SUMS` and the licence texts. The build is reproducible: the tag `v1.1.0.1` builds the published package byte for
byte (SHA-256 `35d43125dfeeea5e147ae1386f3d13d5538a14c728feeb4eeefb07ca195831c7`).

`./build.sh` without `--release` makes a development build (`build/felucca.fwsc`, identity `FM-1_500`).

Since 1.1 the build has the FM6 engine and its pages. The two switches behind it are set by `tools/build.py`
(`TRAP_FM6_TEST=1`, `TRAP_UX_TEST=1`); `TRAP_FM6_TEST=0 TRAP_UX_TEST=0 ./build.sh` builds the 1.0 engines and pages.
The 22 FM6 factory voices are data: `assets/fm6/trapstation-fm6-bank.json` (name, the 155 voice bytes in the DX7's
order, then DRIVE, CUTOFF, OUTPUT), turned into a header by `tools/gen_trap_fm6.py`.

Release numbers have one digit per place (`1.1.0.0`). The identity is `FM-1_5XY` from the first two places
(`1.1.x.x` → `FM-1_511`); `FM-1_57X` belongs to another firmware, so the scheme stops at 6.9.

Build options are environment variables (`0` or `1`), for example `FELUCCA_CPU2` (the second core, default 1) or
`FELUCCA_CDC` (the USB serial console, default 1); the defaults are in `firmware/src/felucca.c` and `core.h`.

## Install your own build

From a local copy of the installer site (Web MIDI needs `https` or `localhost`):

```
python3 web/make_site.py build/felucca-1.1.0.1.fwsc 1.1.0.1 build/site
python3 -m http.server 8009 --bind 127.0.0.1 --directory build/site
# open http://localhost:8009/ in Chrome or Edge
```

Or from the command line (`pip3 install mido python-rtmidi`):

```
python3 tools/fm1_install.py build/felucca-1.1.0.1.fwsc
python3 tools/fm1_install.py --info          # identity of the connected FM-1
```

## The online installer (GitHub Pages)

`tools/make_pages.sh PACKAGE.fwsc VERSION` builds the installer site onto the branch `gh-pages`; push it, and GitHub
Pages serves it (Settings > Pages > Deploy from a branch > `gh-pages`).

A fork first points the README, the installer's source links and `fmhub.json` at its own repository:
`tools/set_repo_url.sh sog-maker/trapstation-os`. Forks must also use their own name, logo and package identity.

## The scene renderer (Trap Station Desktop)

The desktop app's Ableton export bundles `render_scenes.exe`, a host build of the firmware's sequencer and engines.
Its source is here: `tools/render_scenes.c` with the host layer `web/emu/felucca_web.c` and `web/emu/shim/`. After a
`./build.sh` (for `build/gen`):

```
cc -O1 -ffp-contract=off -fwrapv -w -DFELUCCA_FM4=1 -DFELUCCA_SIMPLE=1 -DTRAP_FM6_TEST=1 -DTRAP_UX_TEST=1 \
   -Ibuild/gen -Ifirmware/src -Iweb/emu -Iweb/emu/shim -o build/render_scenes tools/render_scenes.c -lm
```

The app ships this build as `render_scenes_fm6.exe` (jams of 1.1) next to one without the two `TRAP_` switches, made from
the 1.0.0.0 tag (jams of 1.0). The same host layer, built with Emscripten, is the app's sound engine
(`preview/preview.c` in the app's repository).

For Windows, the same with a MinGW-w64 `gcc` gives `render_scenes.exe`.

## Artwork

The screen artwork comes from the approved design reference in [`docs/design/`](../design/) and the pinned exports in
`assets/ui/`. The installer logo is drawn from the boot screen's SVG by `tools/gen_pad_logo.py`.

## Contributing

Bug reports and pull requests are welcome, under GPL-3.0-only. One issue per bug, with the firmware version and what the
screen or the installer showed.

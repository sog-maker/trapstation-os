# TRAPSTATION OS

**Memphis DNA. Modern Phonk Power.** · [Deutsch](README.de.md)

TRAPSTATION OS is free firmware that turns the **M-VAVE FM-1** into a trap and phonk groovebox: booming 808s,
cowbells, dark bells, FM keys and pads, generators for drums, hats, bass and chords, a sampler with slices and a live
performance page.

![TRAPSTATION](assets/logo/fmhub-card.png)

**Current release: [1.0.0.0](https://github.com/sog-maker/trapstation-os/releases/latest)** · works on every FM-1, including the newer
units with the UC25HQ80 flash chip.

## Install the firmware

You need a computer with **Chrome or Edge**, a USB **data** cable and about 15 minutes.

1. Open the **online installer: <https://sog-maker.github.io/trapstation-os/>**
2. Connect the FM-1 (see below) and click **1. Install the firmware**.
3. Back up your sounds, kits and samples first (the installer offers it), then click **Install**.
4. Keep USB connected until it says **Done**. The FM-1 restarts with the pad logo and the Studio Console.

You can always go back to the official firmware (V15) or to another firmware with the same installer.
Step by step, with help when something goes wrong: **[INSTALL.md](INSTALL.md)**.

Also available from [The Firmware Hub](https://thefirmwarehub.com/), and as a file (`trapstation-os-1.0.0.0.fwsc`) on the
[releases page](https://github.com/sog-maker/trapstation-os/releases/latest).

## Connect the device

- Plug the FM-1 **straight into the computer**, with no USB hub, dock or monitor port in between.
- Use a USB **data** cable (some cheap cables only charge).
- Close other music apps that use the FM-1 (your DAW, M-VAVE's apps) while installing.
- On the computer the FM-1 appears as **TRAPSTATION FM-1**. In your DAW it is a normal USB-MIDI device.

## Desktop app (optional)

**[Trap Station Desktop](https://github.com/sog-maker/trapstation-desktop)** for Windows: convert audio files into samples
and load them onto the FM-1, browse the factory banks, mirror the screen and export your jam to Ableton Live.
Download it from its releases page and follow its README.

## What it can do

- **Six tracks plus an FX track**, 64 steps, parameter locks, trig conditions, scenes and a last-jam resume.
- **Sounds:** 808 kit machines with glide and sub, FM SYN, MONO, DIGITAL and a sampler, plus five banks of ready sounds
  (Memphis, Drift Phonk, Jazz Phonk, Chill/Lo-Fi, Wave).
- **Generators:** LUCKY DRUMS, ACE HATS, LOW ROLLER, GOLDEN CHORDS, CHORD GEN, CHORD VAR, QUICK FILL and EUCLID, each
  with MAKE and UNDO.
- **Piano Roll:** edit notes, chords and lengths on the device; OCT+ + K1 chooses whole chords or single notes.
- **Sequencer:** K2 PLAY repeats a bar live, K3 COPY TO copies a bar onto another one.
- **Performance page:** mute tracks on the black keys, jump between bars, eight effect throws on the white keys.
- **Sampler:** up to 32 samples, SLICE with up to 16 slices on a grid or tapped in by hand.
- **Master:** delay, reverb, comb, DJ filter, and TAPE, VHS and VINYL textures.
- **Installer:** install, back up and restore, load samples, reset to the factory state.

## Help and documentation

| | |
|---|---|
| Installation, going back, troubleshooting | [INSTALL.md](INSTALL.md) ([Deutsch](INSTALL.de.md)) |
| User guide (PDF, with the cheat sheet) | [English](docs/manual/en/Trapstation-Guide-EN.pdf) · [Deutsch](docs/manual/de/Trapstation-Guide-DE.pdf) |
| User guide (web pages) | [English](docs/manual/en/USER_GUIDE.md) · [Deutsch](docs/manual/de/USER_GUIDE.md) |
| Cheat sheet | [PDF](docs/cheatsheet/Trapstation-Cheat-Sheet.pdf) |
| Design reference: every screen | [docs/design](docs/design/) |
| What changed | [CHANGELOG.md](CHANGELOG.md) |
| A problem or a bug | [Issues](https://github.com/sog-maker/trapstation-os/issues) |

Building the firmware yourself: [docs/developer](docs/developer/).

## Licence and credits

Free software under **GPL-3.0-only**. TRAPSTATION OS is based on [BBL JAM BOX](https://github.com/UnkleBlairy/BBL-JAM-BOX)
by Blair and [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita (Hügelton Instruments); see
[CREDITS.md](CREDITS.md) and [LICENSING.md](LICENSING.md). The TRAPSTATION name, logo and artwork are not covered by the
GPL. All sounds are synthesised or made by the author.

Not made or supported by M-VAVE. M-VAVE and FM-1 are trademarks of their respective owners. Installing custom firmware
is at your own risk.

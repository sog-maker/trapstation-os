# Changelog

## TRAPSTATION OS 1.1.0.2 (2026-11-16) – kits with FM6 tracks

### Fixed
- **Kits and sound slots with FM6 tracks:** in 1.1.0.0 and 1.1.0.1 a single FM6 track blocked saving and loading kits
  and user sounds (`FM6: USE PROJECT SAVE`), and choosing a kit on the start menu kept the last jam while the screen
  said `KIT LOADED`. Now they save and load: a kit or sound keeps the FM6 preset of a track (by the voice's name) and
  the track's settings. Changes on the FM6 pages stay in the last jam; Trap Station Desktop's Read FM-1 tracks keeps
  them on the computer.
- `KIT LOADED` on the start menu only when the kit was loaded.

Identity `FM-1_511`. Kits and sounds keep their format (an FM6 track's preset rides in a byte no value uses); the
last jam is the same as in 1.1.0.1.

## TRAPSTATION OS 1.1.0.1 (2026-11-16) – the last jam fits again

### Fixed
- **`LAST JAM FULL`:** in 1.1.0.0 the last jam ran out of room with little more than one bar of steps on every track, and from then
  on nothing was saved over power-off. The jam is stored packed now: six tracks of 64 steps each, five of them FM6,
  take a little over half of its room. The FM6 sound is stored for the FM6 tracks only, value for value as before: no
  sound changes.

### Changed
- The last jam has a new format (`JAM7`). A jam of 1.1.0.0 is loaded and saved in the new format at the next save.
  Going back to 1.1.0.0 starts with a fresh jam.
- Trap Station Desktop reads the new jam from 0.2.1 on; 0.2.0 says it does not know it.

Identity `FM-1_511`. Companion: Trap Station Desktop 0.2.1.

## TRAPSTATION OS 1.1.0.0 (2026-11-16) – the FM6 engine

### New
- **FM6:** FM SYN is a six-operator FM engine now (32 algorithms, an envelope of four rates and four levels per
  operator, LFO, pitch envelope), with DRIVE, CUTOFF and OUTPUT per track. Sixteen FM voices across the device, up to
  six on a track. A held note follows the knobs.
- **Presets:** the 22 keys, bells, organs, pads and leads of banks A–E rebuilt on FM6. New names: GHOST VOICES (was
  MELLOW EP), DARK FLUTE (was JAZZ CLAVI), DUSTY KOTO (was MUTED LOFI PLUCK). RACE LEAD and DREAM LEAD are FM6 sounds too.
- **Twelve FM6 pages** in the Workstation design: ROUTING, OP TONE, OP LEVEL (FX); OP RATES, OP LEVELS, KEY SCALE (SCL);
  LFO MOTION, LFO DEPTH, PITCH ENV (ENV); COLOR, DELAY, REVERB (LFO). OCT+ + K1 chooses the operator on every page;
  on PITCH ENV, OCT+ + K2 switches between the rates and the levels.
- **Undo:** OCT- tapped twice takes back the last knob move, or the last sound or pattern load.

### Changed
- The start takes about half a second. The tempo shows whole BPM.
- A loaded scene changes at once, no longer on the next bar.
- The four-operator FM SYN is gone; the FM drum machines, the 808 machines, MONO and the sampler are unchanged and
  sound the same.
- The last jam has a new format that carries the FM6 sounds: a jam of 1.0.0.0 is not loaded. An FM6 sound is kept with
  the jam; while a track plays one, user sound slots and kits do not save or load (`FM6: USE PROJECT SAVE`).

Identity `FM-1_511`. Companion: Trap Station Desktop 0.2.0.

## TRAPSTATION OS 1.0.0.0 (2026-11-15) – first public release

### New
- **Sounds:** five banks (Memphis, Drift Phonk, Jazz Phonk, Chill/Lo-Fi, Wave) with 808s, FM keys, bells, organs, pads and
  leads; 808 glide and sub, accents and rolls.
- **Generators:** LUCKY DRUMS, ACE HATS, LOW ROLLER, GOLDEN CHORDS, CHORD GEN, CHORD VAR, QUICK FILL and EUCLID, each
  with MAKE and UNDO.
- **Piano Roll:** edit notes, chords and lengths on the device; OCT+ + K1 chooses whole chords or single notes, K1
  picks among them (QUICK FILL the same). The keys of the notes that are sounding light up.
- **Sequencer:** K2 PLAY repeats a bar live; K3 COPY TO copies a bar onto another one on every track.
- **Performance page:** all tracks, mutes on the black keys (fully out of the mix), bar jumps on keys 8–11, eight effect
  throws on the white keys.
- **Sampler:** SLICE (up to 16 slices, on a grid or tapped in), knob sets on OCT+ + K1, play modes on OCT+ + K2.
- **Master:** comb, DJ filter, AMP, TAPE, VHS and VINYL; scenes that change on the bar, with no loading screen.
- **The Workstation design**, a start-up with the pad logo, and an optional second core (CLOCK K4).
- **Installer:** backup and restore over USB, sample upload, and a reset to the factory state.
- **On the computer** the FM-1 shows up as TRAPSTATION FM-1.

Identity `FM-1_510`. The newer FM-1 flash chip (`0xB36014`) is supported.

TRAPSTATION OS is a fork of BBL JAM BOX 0.6.1 beta. Its earlier history is in the
[BBL JAM BOX repository](https://github.com/UnkleBlairy/BBL-JAM-BOX) and in [Felucca](https://github.com/hugelton/Felucca).

# TRAPSTATION OS

**Memphis DNA. Modern Phonk Power.** · [English](README.md)

TRAPSTATION OS ist eine freie Firmware, die den **M-VAVE FM-1** zur Trap- und Phonk-Groovebox macht: druckvolle 808s,
Cowbells, dunkle Bells, FM-Keys und Pads, Generatoren für Drums, Hi-Hats, Bass und Akkorde, ein Sampler mit Slices und
eine Live-Performance-Seite.

![TRAPSTATION](assets/logo/fmhub-card.png)

**Aktueller Release: [1.0.0.0](https://github.com/sog-maker/trapstation-os/releases/latest)** · läuft auf jedem FM-1, auch auf den
neueren Geräten mit dem Flash-Chip UC25HQ80.

## Firmware installieren

Du brauchst einen Computer mit **Chrome oder Edge**, ein USB-**Daten**kabel und etwa 15 Minuten.

1. Den **Online-Installer öffnen: <https://sog-maker.github.io/trapstation-os/>**
2. Den FM-1 anschließen (siehe unten) und **1. Firmware installieren** wählen.
3. Zuerst Sounds, Kits und Samples sichern (der Installer bietet es an), dann **Installieren** klicken.
4. USB verbunden lassen, bis **Fertig** erscheint. Der FM-1 startet mit dem Pad-Logo und der Studio Console neu.

Zur offiziellen Firmware (V15) oder zu einer anderen Firmware kommst du jederzeit mit demselben Installer zurück.
Schritt für Schritt, mit Hilfe bei Problemen: **[INSTALL.de.md](INSTALL.de.md)**.

Auch über [The Firmware Hub](https://thefirmwarehub.com/) erhältlich und als Datei (`trapstation-os-1.0.0.0.fwsc`) auf der
[Release-Seite](https://github.com/sog-maker/trapstation-os/releases/latest).

## Gerät verbinden

- Den FM-1 **direkt am Computer** anschließen, ohne USB-Hub, Dock oder Monitor-Anschluss dazwischen.
- Ein USB-**Daten**kabel verwenden (manche billigen Kabel laden nur).
- Andere Musikprogramme, die den FM-1 nutzen (DAW, M-VAVE-Apps), beim Installieren schließen.
- Am Computer heißt der FM-1 **TRAPSTATION FM-1**. In der DAW ist er ein normales USB-MIDI-Gerät.

## Desktop-App (optional)

**[Trap Station Desktop](https://github.com/sog-maker/trapstation-desktop)** für Windows: Audiodateien in Samples umwandeln und
auf den FM-1 laden, die Werksbänke ansehen, den Bildschirm spiegeln und die Jam nach Ableton Live exportieren. Auf der
Release-Seite der App herunterladen und ihrem README folgen.

## Funktionen

- **Sechs Spuren plus FX-Spur**, 64 Steps, Parameter-Locks, Trig-Bedingungen, Szenen und RESUME der letzten Session.
- **Klänge:** 808-Kit-Maschinen mit Glide und Sub, FM SYN, MONO, DIGITAL und ein Sampler, dazu fünf Bänke mit fertigen
  Sounds (Memphis, Drift Phonk, Jazz Phonk, Chill/Lo-Fi, Wave).
- **Generatoren:** LUCKY DRUMS, ACE HATS, LOW ROLLER, GOLDEN CHORDS, CHORD GEN, CHORD VAR, QUICK FILL und EUCLID, jeweils
  mit MAKE und UNDO.
- **Piano Roll:** Noten, Akkorde und Längen am Gerät bearbeiten; OCT+ + K1 wählt ganze Akkorde oder einzelne Noten.
- **Sequencer:** K2 PLAY wiederholt eine Bar live, K3 COPY TO kopiert eine Bar auf eine andere.
- **Performance-Seite:** Spuren auf den schwarzen Tasten muten, zwischen Bars springen, acht Effekt-Würfe auf den weißen
  Tasten.
- **Sampler:** bis zu 32 Samples, SLICE mit bis zu 16 Slices im Raster oder von Hand getippt.
- **Master:** Delay, Reverb, Comb, DJ-Filter sowie die Texturen TAPE, VHS und VINYL.
- **Installer:** installieren, sichern und wiederherstellen, Samples laden, Werkszustand herstellen.

## Hilfe und Dokumentation

| | |
|---|---|
| Installation, Rückweg, Fehlerhilfe | [INSTALL.de.md](INSTALL.de.md) ([English](INSTALL.md)) |
| Anleitung (PDF, mit Spickzettel) | [Deutsch](docs/manual/de/Trapstation-Guide-DE.pdf) · [English](docs/manual/en/Trapstation-Guide-EN.pdf) |
| Anleitung (Webseiten) | [Deutsch](docs/manual/de/USER_GUIDE.md) · [English](docs/manual/en/USER_GUIDE.md) |
| Cheat-Sheet | [PDF](docs/cheatsheet/Trapstation-Cheat-Sheet.pdf) |
| Designreferenz: alle Bildschirme | [docs/design](docs/design/) |
| Was sich geändert hat | [CHANGELOG.md](CHANGELOG.md) |
| Ein Problem oder ein Fehler | [Issues](https://github.com/sog-maker/trapstation-os/issues) |

Die Firmware selbst bauen: [docs/developer](docs/developer/).

## Lizenz und Credits

Freie Software unter **GPL-3.0-only**. TRAPSTATION OS basiert auf [BBL JAM BOX](https://github.com/UnkleBlairy/BBL-JAM-BOX)
von Blair und [Felucca](https://github.com/hugelton/Felucca) von Leo Kuroshita (Hügelton Instruments); siehe
[CREDITS.md](CREDITS.md) und [LICENSING.md](LICENSING.md). Name, Logo und Artwork von TRAPSTATION stehen nicht unter der
GPL. Alle Sounds sind synthetisiert oder selbst gemacht.

Nicht von M-VAVE gemacht oder unterstützt. M-VAVE und FM-1 sind Marken ihrer jeweiligen Inhaber. Das Installieren
eigener Firmware geschieht auf eigenes Risiko.

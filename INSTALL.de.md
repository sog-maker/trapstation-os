# TRAPSTATION OS installieren

[English](INSTALL.md) · [Zurück zum README](README.de.md)

> **Eigene Firmware, auf eigenes Risiko.** TRAPSTATION OS ist nicht von M-VAVE und wird von M-VAVE nicht unterstützt.
> Lies diese Seite einmal vor dem Start. Zur offiziellen Firmware kommst du jederzeit zurück.

Du brauchst etwa 15 Minuten, einen Computer mit **Chrome oder Edge** und ein USB-**Daten**kabel (das mitgelieferte
Kabel des FM-1 geht; manche billigen Kabel laden nur).

Der Installer: **<https://sog-maker.github.io/trapstation-os/>**

---

## Vor dem Start

1. **Chrome oder Edge am Computer verwenden.** Safari, Firefox und Handys können nicht mit dem FM-1 sprechen.
2. **Die offizielle V15-Firmware herunterladen und aufheben**, für den Rückweg: `FM-1_015.fwsc` von
   <https://github.com/m-vave/fm1_open/blob/main/sdk/firmware/FM-1_015.fwsc> (Knopf **Download raw file**).
   Nicht umbenennen oder ändern: Der Installer prüft, dass es genau die offizielle Datei ist.
3. **Alle anderen Programme schließen, die den FM-1 nutzen:** DAW, M-VAVE-Apps, andere Installer-Tabs.
4. **Den FM-1 direkt am Computer anschließen: kein USB-Hub**, kein Dock, keine Tastatur- oder Monitor-Buchse. Ein Hub
   kann die Verbindung mitten im Schreiben abbrechen und den FM-1 hängen lassen.
5. **Den FM-1 laden und den Laptop am Netz lassen**, damit nichts mitten in der Installation einschläft oder leer wird.
6. **Beim Computer bleiben, bis „Fertig“ erscheint.** Die Seite nicht schließen oder neu laden. Wenn etwas schiefgeht,
   die Meldung abschreiben oder fotografieren, bevor du neu lädst.

---

## Firmware installieren

1. Den FM-1 **einschalten**, warten, bis er gestartet ist, und die Wiedergabe stoppen.
2. Per USB anschließen und den Installer öffnen.
3. **1. Firmware installieren · sichern** wählen.
4. Von einer früheren TRAPSTATION-Version? Zuerst **Jetzt sichern** klicken: Das speichert Sounds, Kits, die letzte Jam
   und die Samples in eine Datei auf dem Computer.
5. **Installieren** klicken. Fragt der Browser, ob die Seite **MIDI-Geräte steuern** darf: **Zulassen**.
6. Die Statuszeile zeigt **Start → Prüfen → Wechsel in den Update-Modus → Schreiben → Neustart → Fertig**. Das dauert ein paar Minuten.
   **Das USB-Kabel nicht abziehen, solange geschrieben wird.**
7. Nach **Fertig** startet der FM-1 neu: erst das Pad-Logo, dann die Studio Console mit **NEW**, **LOAD** und **RESUME**.

**Für den besten Klang:** Auf der Seite **CLOCK** den 4. Regler (CPU) nach rechts drehen und den FM-1 aus- und wieder
einschalten. Die Anzeige heißt dann **2CPU**: Der zweite Kern ist an, mit mehr Reserve für Effekte. Die Einstellung
bleibt gespeichert.

### Wenn etwas schiefgeht

| Meldung | Was tun |
|---|---|
| **FM-1 nicht gefunden** | Ist der FM-1 an? Nutzt ein anderes Programm ihn? Ein anderes Kabel versuchen. Dann noch einmal **Installieren**. |
| Nach **Installieren** passiert nichts | Es wurde nichts geschrieben. Seite neu laden, USB abziehen, auf den Windows-Ton „Gerät entfernt“ warten, wieder einstecken, noch einmal **Installieren**. |
| **MIDI-(SysEx-)Zugriff nicht erlaubt** | Auf das Symbol links neben der Adresse klicken, **MIDI-Geräte** erlauben, noch einmal **Installieren**. |
| **Der Update-Loader ist nicht erschienen** | Den FM-1 eingeschaltet lassen. USB abziehen, auf den Ton „Gerät entfernt“ warten, wieder einstecken, noch einmal **Installieren**. Es geht dort weiter, wo es aufgehört hat. |
| **Angehalten** oder **USB-Verbindung verloren** | Kabel wieder einstecken, noch einmal **Installieren**. |
| **Der FM-1 ist nicht zurückgekommen** | Den FM-1 aus- und wieder einschalten. |

Scheitert eine Installation und startet der FM-1 gar nicht mehr, braucht die Rettung zusätzliche Hardware, den
[FM-1-transporter](https://github.com/kurogedelic/FM-1-transporter). Deshalb nie abziehen, solange geschrieben wird.

---

## Sichern, Wiederherstellen, Werkszustand

Alles im Installer unter **Sichern & Wiederherstellen**:

- **Jetzt sichern** speichert Sounds, Kits, die letzte Jam, die Einstellungen und die Samples in eine Datei.
- **Wiederherstellen** spielt eine Sicherungsdatei zurück auf den FM-1 (Häkchen setzen, um die Samples mitzunehmen).
  Der FM-1 startet danach neu.
- **Sichern und Werkszustand herstellen** sichert zuerst und löscht dann deine eigenen Sounds, Kits, Samples und die
  letzte Jam. Danach hat der FM-1 nur die Werks-Presets, die zwei Werks-Kits (MEMPHIS 808, DR660 KIT) und die
  Werkssounds. Deine Einstellungen (2CPU, LEDs, MIDI) bleiben.

Eine neue Version zu installieren löscht nie deine Sounds, Kits oder Samples.

## Eigene Samples laden

1. Im Installer **2. Samples laden** wählen.
2. Eine Audiodatei wählen (WAV, MP3, OGG oder FLAC), den gewünschten Teil mit **Start** und **Ende** festlegen, mit
   **Schreiben** zur Liste hinzufügen. Bis zu 32 Samples, zusammen etwa 11 Sekunden.
3. **Aufs FM-1 hochladen** klicken. Der FM-1 zeigt **SAMPLES LOADED**, wenn er fertig ist.
4. Am FM-1 einen Sampler-Sound wählen; sein Regler **SLOT** wählt S01 bis S32.

Jedes Hochladen ersetzt den ganzen Sample-Speicher durch die Liste der Seite; die Seite merkt sich die Liste im Browser.
Die App [Trap Station Desktop](https://github.com/sog-maker/trapstation-desktop) kann dasselbe mit mehr Audioformaten.

---

## Zurück zur offiziellen Firmware (V15)

1. Den FM-1 einschalten, Wiedergabe stoppen, anschließen und den Installer öffnen.
2. **Zurück zur offiziellen V15** öffnen.
3. Unter **Offizielle FM-1.fwsc** die heruntergeladene `FM-1_015.fwsc` wählen. Es erscheint **Offizielle V15 geprüft**.
4. **Sichern und zurück zur offiziellen V15** klicken. Deine TRAPSTATION-Sounds, -Kits und -Samples werden vorher in
   eine Datei gesichert (V15 kann sie nicht nutzen; bei der Rückkehr stellst du sie wieder her).
5. Auf **Fertig** warten. Der FM-1 startet mit V15 neu.

## Zu einer anderen Firmware wechseln und zurück

TRAPSTATION bringt seinen Update-Loader mit, wie Felucca und die meisten Firmwares auf
[The Firmware Hub](https://thefirmwarehub.com/). Du kannst eine andere Firmware mit deren Installer (oder über den Hub)
aufspielen und mit diesem Installer zu TRAPSTATION zurückkehren, in jeder Reihenfolge. Vorher sichern: Jede Firmware
hat ihre eigenen Daten.

---

## Hilfe

Ein [Issue](https://github.com/sog-maker/trapstation-os/issues) eröffnen. Bitte angeben, was die Statuszeile des Installers zeigte
und was auf dem Bildschirm des FM-1 steht.

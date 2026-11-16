/* SPDX-License-Identifier: GPL-3.0-only
 * render_scenes.c - renders one track of a kit through the firmware's own sequencer (emulator core,
 * web/emu/felucca_web.c). Loads a spec file (see kit_to_wav.py): globals, the track sounds (engine, params),
 * the pattern's steps, the scene masks. Starts the transport, switches the scenes (P_MUTE = not the mask bit,
 * as SCENE APPLY does), and writes the audible track as a WAV (16 bit, mono, 44.1 kHz).
 *
 * Since 1.1: the engine name FM6 in a "track" line, and "f TRACK INDEX VALUE" lines for the 158 values of a
 * track's FM6 voice (with -DTRAP_FM6_TEST=1 -DTRAP_UX_TEST=1).
 *
 *   cc -O1 -ffp-contract=off -fwrapv -w -DFELUCCA_FM4=1 -DFELUCCA_SIMPLE=1 -DTRAP_FM6_TEST=1 -DTRAP_UX_TEST=1 \
 *      -Ibuild/gen -Ifirmware/src -Iweb/emu -Iweb/emu/shim -o build/render_scenes tools/render_scenes.c -lm
 *   render_scenes SPEC.txt OUT.wav
 */
#include <time.h>
#include "felucca_web.c"

static FILE *wf;
static uint32_t wn;
static int audible = 1;
static int masks[4];
static double scene_ms = 1000.0;
static int samples_loaded;

static void wav_head(uint32_t n)
{
    uint32_t b = n * 2u, v;
    fseek(wf, 0, SEEK_SET);
    fwrite("RIFF", 1, 4, wf); v = 36u + b; fwrite(&v, 4, 1, wf);
    fwrite("WAVEfmt ", 1, 8, wf); v = 16; fwrite(&v, 4, 1, wf);
    { uint16_t f = 1, ch = 1, ba = 2, bits = 16; uint32_t sr = 44100, br = 88200;
      fwrite(&f, 2, 1, wf); fwrite(&ch, 2, 1, wf); fwrite(&sr, 4, 1, wf); fwrite(&br, 4, 1, wf);
      fwrite(&ba, 2, 1, wf); fwrite(&bits, 2, 1, wf); }
    fwrite("data", 1, 4, wf); fwrite(&b, 4, 1, wf);
}

static void run_ms(double ms, int rec)
{
    uint32_t n = (uint32_t)(ms * 44.1 + 0.5), k, i;
    while (n) {
        k = n > 256u ? 256u : n;
        web_render(k);
        if (rec)
            for (i = 0; i < k; i++) {
                float x = out_l[i] * 32767.0f;
                int16_t s = (int16_t)(x > 32767.0f ? 32767 : x < -32768.0f ? -32768 : (int)x);
                fwrite(&s, 2, 1, wf);
                wn++;
            }
        n -= k;
    }
}

static void apply_scene(int s)
{
    int p;
    ph_scene = (int16_t)s;
    for (p = 0; p < NTRK; p++)
        trk[p].p[P_MUTE] = (int16_t)(p == audible - 1 ? !((masks[s] >> p) & 1) : 1);
    fxl_mute = 1;
}

static int hexnib(int c)
{
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : 0;
}

int main(int argc, char **argv)
{
    FILE *sp;
    char key[32];
    int scene = 0;
    double total_ms = 0, elapsed = 0;

    if (argc < 3) { fprintf(stderr, "render_scenes SPEC OUT.wav\n"); return 2; }
    web_nor_erase();
    web_master(1023);
    web_boot();
    run_ms(3000, 0);

    sp = fopen(argv[1], "r");
    if (!sp) { fprintf(stderr, "no spec %s\n", argv[1]); return 2; }
    while (fscanf(sp, "%31s", key) == 1) {
        if (!strcmp(key, "g")) {
            int i, v;
            if (fscanf(sp, "%d %d", &i, &v) != 2) return 2;
            song.g[i] = (int16_t)v;
        } else if (!strcmp(key, "track")) {
            int t; char e[16];
            if (fscanf(sp, "%d %15s", &t, e) != 2) return 2;
            if (t < 1 || t > NTRK) return 2;
            trk[t - 1].eng_req = (uint8_t)(!strcmp(e, "ANALOG") ? ENGI_ANALOG : !strcmp(e, "SAMPLER") ? ENGI_SAMPLER
                                         : !strcmp(e, "MONO") ? ENGI_MONO : ENGI_DIGITAL);
#if TRAP_FM6_TEST
            if (!strcmp(e, "FM6")) trk[t - 1].eng_req = ENGI_FM6;
        } else if (!strcmp(key, "f")) {
            int t, i, v;
            if (fscanf(sp, "%d %d %d", &t, &i, &v) != 3 || t < 1 || t > NTRK || i < 0 || i >= 158) return 2;
            tf6_edit[t - 1][i] = (int16_t)v;
            tf6_ready[t - 1] = 1;
            tf6[t - 1].valid = 0;
#endif
        } else if (!strcmp(key, "p")) {
            int t, i, v;
            if (fscanf(sp, "%d %d %d", &t, &i, &v) != 3) return 2;
            trk[t - 1].p[i] = (int16_t)v;
        } else if (!strcmp(key, "step")) {
            int t, i, k; char hex[32];
            uint8_t *b;
            if (fscanf(sp, "%d %d %31s", &t, &i, hex) != 3) return 2;
            b = (uint8_t *)&trk[t - 1].step[i];
            for (k = 0; k < (int)sizeof(step_t); k++)
                b[k] = (uint8_t)(hexnib(hex[2 * k]) << 4 | hexnib(hex[2 * k + 1]));
        } else if (!strcmp(key, "masks")) {
            if (fscanf(sp, "%d %d %d %d", &masks[0], &masks[1], &masks[2], &masks[3]) != 4) return 2;
        } else if (!strcmp(key, "scene_ms")) {
            if (fscanf(sp, "%lf", &scene_ms) != 1) return 2;
        } else if (!strcmp(key, "samples")) {
            char path[512];
            FILE *sf;
            size_t n;
            if (fscanf(sp, "%511s", path) != 1) return 2;
            sf = fopen(path, "rb");
            if (!sf) { fprintf(stderr, "no samples %s\n", path); return 2; }
            n = fread(nor + SMP_BASE, 1, sizeof nor - SMP_BASE, sf);
            fclose(sf);
            (void)n;
            samples_loaded = 1;
        } else if (!strcmp(key, "audible")) {
            if (fscanf(sp, "%d", &audible) != 1) return 2;
        } else if (!strcmp(key, "total_ms")) {
            if (fscanf(sp, "%lf", &total_ms) != 1) return 2;
        } else {
            fprintf(stderr, "bad spec key %s\n", key);
            return 2;
        }
    }
    fclose(sp);
    if (samples_loaded)
        smp_scan();

    run_ms(50, 0);
    wf = fopen(argv[2], "wb");
    if (!wf) return 1;
    wav_head(0);
    apply_scene(0);
    transport_req = 1;
    while (elapsed < total_ms) {
        int s;
        run_ms(10.0, 1);
        elapsed += 10.0;
        s = (int)(elapsed / scene_ms);
        if (s > 3) s = 3;
        if (s != scene) { scene = s; apply_scene(s); }
    }
    wav_head(wn);
    fclose(wf);
    return 0;
}

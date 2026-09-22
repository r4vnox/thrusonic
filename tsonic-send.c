#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>

#define SAMPLE_RATE 44100
#define BIT_DURATION 0.05   // 50ms per bit (yavas ama güvenılir)
#define FREQ_0 6000
#define FREQ_1 8000

snd_pcm_t *handle;
snd_pcm_hw_params_t *params;
snd_pcm_uframes_t frames;
int dir;
short *buffer;

void play_tone(double frequency, double duration) {
    int total_samples = (int)(SAMPLE_RATE * duration);
    int loops = total_samples / frames;
    int remainder = total_samples % frames;

    for (int i = 0; i < loops; i++) {
        for (int j = 0; j < frames; j++) {
            double t = (double)(i * frames + j) / SAMPLE_RATE;
            buffer[j] = (short)(32767.0 * sin(2.0 * M_PI * frequency * t));
        }
        snd_pcm_writei(handle, buffer, frames);
    }
    if (remainder > 0) {
        for (int j = 0; j < remainder; j++) {
            double t = (double)(loops * frames + j) / SAMPLE_RATE;
            buffer[j] = (short)(32767.0 * sin(2.0 * M_PI * frequency * t));
        }
        snd_pcm_writei(handle, buffer, remainder);
    }
}

// SECDED (13,8) encode
int secded_encode(int data) {
    int d[9];
    for (int i = 1; i <= 8; i++) d[i] = (data >> (8 - i)) & 1;
    int p1 = d[1] ^ d[2] ^ d[4] ^ d[5] ^ d[7];
    int p2 = d[1] ^ d[3] ^ d[4] ^ d[6] ^ d[7];
    int p4 = d[2] ^ d[3] ^ d[4] ^ d[8];
    int p8 = d[5] ^ d[6] ^ d[7] ^ d[8];

    int cw = 0;
    cw |= (p1 << 0);
    cw |= (p2 << 1);
    cw |= (d[1] << 2);
    cw |= (p4 << 3);
    cw |= (d[2] << 4);
    cw |= (d[3] << 5);
    cw |= (d[4] << 6);
    cw |= (p8 << 7);
    cw |= (d[5] << 8);
    cw |= (d[6] << 9);
    cw |= (d[7] << 10);
    cw |= (d[8] << 11);

    int overall = 0;
    for (int i = 0; i < 12; i++) overall ^= (cw >> i) & 1;
    cw |= (overall << 12);
    return cw;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Kullanım: %s <dosya>\n", argv[0]);
        return 1;
    }

    int rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) { fprintf(stderr, "Ses aygıtı açılamadı.\n"); return 1; }

    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(handle, params, 1);
    unsigned int val = SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);
    snd_pcm_hw_params(handle, params);
    snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    buffer = (short *)malloc(frames * sizeof(short));

    FILE *file = fopen(argv[1], "rb");
    if (!file) { perror("Dosya açılamadı"); return 1; }

    // Preamble: 32 bit alternating + sync byte 0xD5
    printf("[Preamble] 32 bit alternating + 0xD5...\n");
    for (int i = 0; i < 32; i++) {
        int bit = i % 2;
        play_tone(bit ? FREQ_1 : FREQ_0, BIT_DURATION);
    }
    for (int i = 7; i >= 0; i--) {
        int bit = (0xD5 >> i) & 1;
        play_tone(bit ? FREQ_1 : FREQ_0, BIT_DURATION);
    }

    printf("[ThruSonic Send] SECDED(13,8) ile gönderiliyor...\n");
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        int cw = secded_encode(ch);
        for (int i = 12; i >= 0; i--) {
            int bit = (cw >> i) & 1;
            play_tone(bit ? FREQ_1 : FREQ_0, BIT_DURATION);
        }
    }

    printf("[Gönderim Tamamlandı]\n");
    fclose(file);
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);
    return 0;
}

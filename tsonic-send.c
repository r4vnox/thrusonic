#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>

#define SAMPLE_RATE 44100
#define BIT_DURATION 0.02 // Her bir bit 10ms sürecek (100 bit/saniye)
#define FREQ_0 6000       // 0 biti için frekans
#define FREQ_1 8000       // 1 biti için frekans (laptop zor duyduğu için suanlık düşürüldü!)
#define USE_HAMMING 0  // 0 = Normal (çalışan), 1 = Hamming (deneysel)

// ALSA için global değişkenler
snd_pcm_t *handle;
snd_pcm_hw_params_t *params;
snd_pcm_uframes_t frames;
int dir;
short *buffer;

// Belirli bir frekansta ve sürede ses üreten fonksiyon
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

// Hamming(7,4) encode: 4 data biti  -> 7 kod bıtı
int hamming_encode(int data) {
    int d1 = (data >> 3) & 1;
    int d2 = (data >> 2) & 1;
    int d3 = (data >> 1) & 1;
    int d4 = data & 1;
    int p1 = d1 ^ d2 ^ d4;
    int p2 = d1 ^ d3 ^ d4;
    int p3 = d2 ^ d3 ^ d4;
    return (p1 << 6) | (p2 << 5) | (d1 << 4) | (p3 << 3) | (d2 << 2) | (d3 << 1) | d4;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Kullanım: %s <dosya_yolu>\n", argv[0]);
        return 1;
    }

    // 1. ALSA ses aygıtını çalma modunda aç
    int rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) {
        fprintf(stderr, "Ses aygıtı açılamadı.\n");
        return 1;
    }

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

    // 2. Dosyayı binary read (rb) modunda aç
    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) {
        perror("Dosya açılamadı");
        return 1;
    }

    // 3. PREAMBLE GÖNDER (Senkronizasyon için)
    // 16 bit alternatif 10101010... deseni
    printf("[Preamble] Senkronizasyon sinyali gönderiliyor...\n");
    for (int i = 0; i < 16; i++) {
        int bit = (i % 2 == 0) ? 1 : 0;
        play_tone(bit ? FREQ_1 : FREQ_0, BIT_DURATION);
    }

    // Sync byte: 0xD5 (11010101) - Preamble'da görünmez, benzersiz
    for (int i = 7; i >= 0; i--) {
        int bit = (0xD5 >> i) & 1;
        play_tone(bit ? FREQ_1 : FREQ_0, BIT_DURATION);
    }

    printf("[ThruSonic Send] Dosya FSK modülasyonu ile gönderiliyor...\n");

    int ch;
    while ((ch = fgetc(file)) != EOF) {
#if USE_HAMMING
        // Hamming(7,4) Modu
        int high = (ch >> 4) & 0x0F;
        int low = ch & 0x0F;
        int cw1 = hamming_encode(high);
        int cw2 = hamming_encode(low);
        for (int i = 6; i >= 0; i--) play_tone(((cw1 >> i) & 1) ? FREQ_1 : FREQ_0, BIT_DURATION);
        for (int i = 6; i >= 0; i--) play_tone(((cw2 >> i) & 1) ? FREQ_1 : FREQ_0, BIT_DURATION);
#else
        // Normal mod (8 bit)
        for (int i = 7; i >= 0; i--) {
            int bit = (ch >> i) & 1;
            play_tone(bit ? FREQ_1 : FREQ_0, BIT_DURATION);
        }
#endif
    }

    printf("[Gönderim Tamamlandı]\n");

    // 5. Temizlik
    fclose(file);
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fftw3.h>

#define SAMPLE_RATE 44100
#define BIT_DURATION 0.05
#define FREQ_0 6000
#define FREQ_1 8000
#define N 2205          // 44100 * 0.05
#define SYNC_BYTE 0xD5
#define MAG_THRESHOLD 3.0

// SECDED decode
int secded_decode(int cw, int *corrected) {
    int p1 = (cw >> 0) & 1;
    int p2 = (cw >> 1) & 1;
    int d1 = (cw >> 2) & 1;
    int p4 = (cw >> 3) & 1;
    int d2 = (cw >> 4) & 1;
    int d3 = (cw >> 5) & 1;
    int d4 = (cw >> 6) & 1;
    int p8 = (cw >> 7) & 1;
    int d5 = (cw >> 8) & 1;
    int d6 = (cw >> 9) & 1;
    int d7 = (cw >> 10) & 1;
    int d8 = (cw >> 11) & 1;
    int overall = (cw >> 12) & 1;

    int s1 = p1 ^ d1 ^ d2 ^ d4 ^ d5 ^ d7;
    int s2 = p2 ^ d1 ^ d3 ^ d4 ^ d6 ^ d7;
    int s4 = p4 ^ d2 ^ d3 ^ d4 ^ d8;
    int s8 = p8 ^ d5 ^ d6 ^ d7 ^ d8;
    int syndrome = (s8 << 3) | (s4 << 2) | (s2 << 1) | s1;

    int overall_calc = p1 ^ p2 ^ d1 ^ p4 ^ d2 ^ d3 ^ d4 ^ p8 ^ d5 ^ d6 ^ d7 ^ d8 ^ overall;

    *corrected = 0;
    if (syndrome == 0 && overall_calc == 0) {
        // hata yok
    } else if (syndrome != 0 && overall_calc == 1) {
        *corrected = 1;
        if (syndrome <= 12) {
            cw ^= (1 << (syndrome - 1));
            d1 = (cw >> 2) & 1;
            d2 = (cw >> 4) & 1;
            d3 = (cw >> 5) & 1;
            d4 = (cw >> 6) & 1;
            d5 = (cw >> 8) & 1;
            d6 = (cw >> 9) & 1;
            d7 = (cw >> 10) & 1;
            d8 = (cw >> 11) & 1;
        }
    } else if (syndrome != 0 && overall_calc == 0) {
        *corrected = -1; //çift hata
    } else {

    }
    return (d1 << 7) | (d2 << 6) | (d3 << 5) | (d4 << 4) | (d5 << 3) | (d6 << 2) | (d7 << 1) | d8;
}

int main(int argc, char *argv[]) {
    if (argc < 3) { printf("Kullanım: %s <kayit.raw> <cikti>\n", argv[0]); return 1; }

    FILE *file = fopen(argv[1], "rb");
    if (!file) { perror("Kayıt açılamadı"); return 1; }
    FILE *out_file = fopen(argv[2], "wb");
    if (!out_file) { perror("Çıktı açılamadı"); fclose(file); return 1; }

    double *in = (double*) fftw_malloc(sizeof(double) * N);
    fftw_complex *out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (N/2 + 1));
    fftw_plan plan = fftw_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);
    short *buffer = (short*) malloc(sizeof(short) * N);

    int bin_6k = (int)(FREQ_0 * N / SAMPLE_RATE);
    int bin_8k = (int)(FREQ_1 * N / SAMPLE_RATE);

    int synced = 0;
    int silent_count = 0;
    int total_bits = 0;
    int sync_buffer[8] = {0};
    int codeword = 0;
    int bit_count = 0;
    int total_corrected = 0;
    int total_uncorrectable = 0;

    printf("[ThruSonic Recv] Preamble aranıyor...\n");

    while (fread(buffer, sizeof(short), N, file) == N) {
        for (int i = 0; i < N; i++) in[i] = (double)buffer[i] / 32768.0;
        fftw_execute(plan);

        double mag_6k = sqrt(out[bin_6k][0]*out[bin_6k][0] + out[bin_6k][1]*out[bin_6k][1]);
        double mag_8k = sqrt(out[bin_8k][0]*out[bin_8k][0] + out[bin_8k][1]*out[bin_8k][1]);

        if (!synced && mag_6k < MAG_THRESHOLD && mag_8k < MAG_THRESHOLD) continue;

        if (mag_6k < MAG_THRESHOLD && mag_8k < MAG_THRESHOLD) {
            silent_count++;
            if (silent_count > 25) break;
        } else {
            silent_count = 0;
        }

        int bit = (mag_8k > mag_6k) ? 1 : 0;

        if (!synced) {
            for (int k = 0; k < 7; k++) sync_buffer[k] = sync_buffer[k+1];
            sync_buffer[7] = bit;
            int match = 1;
            for (int k = 0; k < 8; k++) {
                int expected = (SYNC_BYTE >> (7 - k)) & 1;
                if (sync_buffer[k] != expected) { match = 0; break; }
            }
            if (match) {
                printf("[SYNC] Preamble bulundu! Veri okunuyor...\n");
                synced = 1;
                codeword = 0;
                bit_count = 0;
            }
            continue;
        }

        // SECDED veri okuma
        codeword = (codeword << 1) | bit;
        bit_count++;
        total_bits++;

        if (bit_count == 13) {
            int corrected;
            int data = secded_decode(codeword, &corrected);
            if (corrected == 1) {
                total_corrected++;
                fprintf(stderr, "[SECDED] 1-bit hata düzeltildi.\n");
            } else if (corrected == -1) {
                total_uncorrectable++;
                fprintf(stderr, "[SECDED] Düzeltilemez hata! Byte atlandı.\n");
            } else {
                fwrite(&data, 1, 1, out_file);
            }
            codeword = 0;
            bit_count = 0;
        }
    }

    if (!synced) fprintf(stderr, "[UYARI] Preamble bulunamadı!\n");

    printf("[Tamamlandı] Toplam %d bit, %d düzeltme, %d düzeltilemez hata.\n",
           total_bits, total_corrected, total_uncorrectable);

    fftw_destroy_plan(plan);
    fftw_free(in); fftw_free(out);
    free(buffer);
    fclose(file); fclose(out_file);
    return 0;
}

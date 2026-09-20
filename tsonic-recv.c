#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fftw3.h>

#define USE_HAMMING 0  // 0 = Normal (çalışan), 1 = Hamming (deneysel)
#define SAMPLE_RATE 44100
#define BIT_DURATION 0.01
#define FREQ_0 6000
#define FREQ_1 8000
#define N 882
#define SYNC_BYTE 0xD5
#define MAG_THRESHOLD 1.0

// Hamming(7,4) decode + tek bıt hata düzeltmee
int hamming_decode(int codeword, int *corrected) {
    int p1 = (codeword >> 6) & 1;
    int p2 = (codeword >> 5) & 1;
    int d1 = (codeword >> 4) & 1;
    int p3 = (codeword >> 3) & 1;
    int d2 = (codeword >> 2) & 1;
    int d3 = (codeword >> 1) & 1;
    int d4 = codeword & 1;

    int s1 = p1 ^ d1 ^ d2 ^ d4;
    int s2 = p2 ^ d1 ^ d3 ^ d4;
    int s3 = p3 ^ d2 ^ d3 ^ d4;
    int syndrome = (s3 << 2) | (s2 << 1) | s1;

    *corrected = 0;
    if (syndrome != 0) {
        *corrected = 1;
        codeword ^= (1 << (7 - syndrome));
        d1 = (codeword >> 4) & 1;
        d2 = (codeword >> 2) & 1;
        d3 = (codeword >> 1) & 1;
        d4 = codeword & 1;
    }
    return (d1 << 3) | (d2 << 2) | (d3 << 1) | d4;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Kullanım: %s <kayit.raw> <cikti.txt>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) { perror("Kayıt dosyası açılamadı"); return 1; }

    FILE *out_file = fopen(argv[2], "wb");
    if (out_file == NULL) { perror("Çıktı dosyası oluşturulamadı"); fclose(file); return 1; }

    double *in = (double*) fftw_malloc(sizeof(double) * N);
    fftw_complex *out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (N/2 + 1));
    fftw_plan plan = fftw_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);
    short *buffer = (short*) malloc(sizeof(short) * N);

    int bin_18k = (int)(FREQ_0 * N / SAMPLE_RATE);
    int bin_19k = (int)(FREQ_1 * N / SAMPLE_RATE);

    // ==================== ALICI DURUM DEĞİŞKENLERİ ====================
    int synced = 0;
    int silent_count = 0;
    int total_bits = 0;
    int sync_buffer[8] = {0};

    // Normal mod değişkenleri
    unsigned char current_byte = 0;
    int bit_count = 0;

    // Hamming modu değişkenleri
    int codeword = 0;
    int hamming_bit_count = 0;
    int high_nibble = 0;
    int nibble_count = 0;
    int total_corrected = 0;
    // ===================================================================

    printf("[ThruSonic Recv] Kayıt analiz ediliyor, preamble aranıyor...\n");

    while (fread(buffer, sizeof(short), N, file) == N) {
        for (int i = 0; i < N; i++) in[i] = (double)buffer[i] / 32768.0;
        fftw_execute(plan);

        double mag_18k = sqrt(out[bin_18k][0]*out[bin_18k][0] + out[bin_18k][1]*out[bin_18k][1]);
        double mag_19k = sqrt(out[bin_19k][0]*out[bin_19k][0] + out[bin_19k][1]*out[bin_19k][1]);

        if (!synced && mag_18k < MAG_THRESHOLD && mag_19k < MAG_THRESHOLD) continue;

        if (mag_18k < MAG_THRESHOLD && mag_19k < MAG_THRESHOLD) {
            silent_count++;
            if (silent_count > 25) break;
        } else {
            silent_count = 0;
        }

        int bit = (mag_19k > mag_18k) ? 1 : 0;

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
            }
            continue;
        }

        // ==================== VERİ OKUMA ====================
#if USE_HAMMING
        codeword = (codeword << 1) | bit;
        hamming_bit_count++;
        total_bits++;

        if (hamming_bit_count == 7) {
            int corrected;
            int nibble = hamming_decode(codeword, &corrected);
            if (corrected) {
                total_corrected++;
                fprintf(stderr, "[HAMMING] 1-bit hata düzeltildi!\n");
            }
            if (nibble_count == 0) {
                high_nibble = nibble;
                nibble_count = 1;
            } else {
                unsigned char byte = (high_nibble << 4) | nibble;
                fwrite(&byte, 1, 1, out_file);
                nibble_count = 0;
            }
            codeword = 0;
            hamming_bit_count = 0;
        }
#else
        current_byte = (current_byte << 1) | bit;
        bit_count++;
        total_bits++;

        if (bit_count == 8) {
            fwrite(&current_byte, 1, 1, out_file);
            current_byte = 0;
            bit_count = 0;
        }
#endif
        // ==================================================
    }

    if (!synced) fprintf(stderr, "[UYARI] Preamble bulunamadı!\n");

#if USE_HAMMING
    printf("[Tamamlandı] Toplam %d bit, %d hata düzeltildi. Dosya: %s\n",
           total_bits, total_corrected, argv[2]);
#else
    printf("[Tamamlandı] Toplam %d bit işlendi. Dosya: %s\n", total_bits, argv[2]);
#endif

    fftw_destroy_plan(plan);
    fftw_free(in); fftw_free(out);
    free(buffer);
    fclose(file); fclose(out_file);
    return 0;
}

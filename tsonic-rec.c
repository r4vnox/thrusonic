#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>

#define SAMPLE_RATE 44100 // Generator ile aynı örnekleme hızı
#define DURATION 300      // Kaç saniye kayıt yapılacağı (test için yükseltildi. değiştirebilirsiniz.)

int main() {
    int rc;
    int size;
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    unsigned int val;
    int dir;
    snd_pcm_uframes_t frames;

    // 1. ALSA ses aygıtını (varsayılan mikrofonu) KAYIT (CAPTURE) modunda açıyoruz
    rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        fprintf(stderr, "Mikrofon açılamadı: %s\n", snd_strerror(rc));
        return 1;
    }

    // Donanım parametreleri için yer ayırıyoruz
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);

    // Kesintisiz (Interleaved) okuma/yazma modunu seçiyoruz
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

    // Ses formatını 16-bıt Küçük Uçlu (Little Endian) yapıyoruz (Generator ile aynı)
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

    // Tek kanal (Mono) kayıt yapacağız
    snd_pcm_hw_params_set_channels(handle, params, 1);

    // Örnekleme hızını (44100 Hz) bağlıyoruz
    val = SAMPLE_RATE;
    snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);

    // Parametreleri ses kartına gönderiyoruz
    rc = snd_pcm_hw_params(handle, params);
    if (rc < 0) {
        fprintf(stderr, "Parametreler ayarlanamadı: %s\n", snd_strerror(rc));
        snd_pcm_close(handle);
        return 1;
    }

    // Buffer boyutunu alıyoruz
    snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    size = frames * 2; // 16-bit (2 byte) her örnek için
    char *buffer = (char *)malloc(size);

    // Kaç döngü boyunca kayıt yapılacağını hesaplıyoruz
    snd_pcm_hw_params_get_period_time(params, &val, &dir);
    long loops = (DURATION * 1000000) / val;

    // Kayıt yapılacak dosyayı binary write (wb) modunda açıyoruz
    FILE *file = fopen("recorded.raw", "wb");
    if (file == NULL) {
        perror("Kayıt dosyası oluşturulamadı");
        snd_pcm_close(handle);
        free(buffer);
        return 1;
    }

    printf("[ThruSonic Recorder] %d saniye boyunca mikrofon dinleniyor...\n", DURATION);
    printf("Kayıt başladı... (recorded.raw dosyasına yazılıyor)\n");

    for (int i = 0; i < loops; i++) {
        // Mikrofon dan ham ses verisini oku
        rc = snd_pcm_readi(handle, buffer, frames);
        if (rc == -EPIPE) {
            // Buffer taşması (Overrun) durumunda toparlan
            fprintf(stderr, "Buffer taşması!\n");
            snd_pcm_prepare(handle);
        } else if (rc < 0) {
            fprintf(stderr, "Mikrofon okuma hatası: %s\n", snd_strerror(rc));
        } else if (rc != (int)frames) {
            fprintf(stderr, "Kısa okuma, beklenen: %d, alınan: %d\n", (int)frames, rc);
        }

        // Okunan veriyi dosyaya yaz
        if (rc > 0) {
            fwrite(buffer, 1, rc * 2, file);
        }
    }

    // Temizlik
    printf("[Kayıt Tamamlandı] recorded.raw dosyası oluşturuldu.\n");
    fclose(file);
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);
    return 0;
}

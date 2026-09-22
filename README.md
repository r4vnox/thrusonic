# ThruSonic 🦈🔊

İnternet, Wi-Fi veya Bluetooth bağlantısı olmadan, iki Linux bilgisayar arasında
yüksek frekanslı (ultrasonik) ses dalgalarını kullanarak gizli ve güvenli veri
transferi sağlayan C tabanlı bir "Data-over-Sound" aracı.

## 🚀 Nasıl Çalışır?

- **Verici (`tsonic-send`):** Gönderilmek istenen dosyayı binary (0-1) formatına
  çevirir. Frekans Kaydırmalı Anahtarlama (FSK) yöntemiyle 0'ları düşük frekansa,
  1'leri yüksek frekansa dönüştürür ve Linux ALSA altyapısı ile hoparlörden yayınlar.
- **Alıcı (`tsonic-recv`):** Mikrofon dan ortam sesini dinler, Hızlı Fourier
  Dönüşümü (FFT) algoritmalarını kullanarak anlık frekans analizi yapar. Gelen
  frekansları tekrar binary koda ve ardından orijinal dosyaya dönüştürür.





## 🔬 Ultrasonik Mod Hakkında

Projenin hedefi **18.000 Hz (0 biti)** ve **19.000 Hz (1 biti)** ultrasonik
frekanslardir. Ancak testlerde kullanılan dizustü bilgisayarların **dahili
mikrofonları ve hoparlörleri** bu frekanslarda yeterli performansı
**GÖSTERMEMİŞTİR.** Bu bir **donanım sınırıdır**, yazılım hatasıı değildir.

### Ultrasonik İçin Ne Gerekli?
- **Harici USB mikrofon** (20kHz'e kadar düz frekans tepkisi)
- **Harici hoparlör** (tweeter'lı sistem)
- `FREQ_0 = 18000`, `FREQ_1 = 19000` yapmak yeterlidir.




## 📁 Proje Dosyaları

| Dosya | Açıklama |
|-------|----------|
| `tsonic-send.c` | FSK verici (dosyayı sesle gönderir) |
| `tsonic-recv.c` | FSK alıcı (sesi analiz edip dosyaya çevirir) |
| `tsonic-rec.c` | Mikrofon kayıt modülü |
| `tsonic-fft.c` | FFT analiz modülü (test) |
| `encoder.c` | Binary encoder |
| `generator.c` | 440 Hz test sinyali üreteci |





## 🛠️ Derleme ve Kullanım (Şuanlık)

```bash
# Derleme
gcc tsonic-send.c -o tsonic-send -lasound -lm
gcc tsonic-recv.c -o tsonic-recv -lfftw3 -lm
gcc tsonic-rec.c -o tsonic-rec -lasound

# Text Transferi Testi
echo "Merhaba ThruSonic!" > test_mesaj.txt
./tsonic-rec & sleep 2 && ./tsonic-send test_mesaj.txt && sleep 2 && ./tsonic-recv recorded.raw cikti.txt
cat cikti.txt

# Binary Transferi Testi
head -c 32 /dev/urandom > binary_test.bin
./tsonic-rec & sleep 2 && ./tsonic-send binary_test.bin && sleep 2 && ./tsonic-recv recorded.raw cikti.bin
cmp binary_test.bin cikti.bin

```




## 📌 Yapılacaklar (TODO)

### ✅ Tamamlananlar
- [x] Proje temeli, Git ve GitHub kurulumu
- [x] Binary encoder modülü (`encoder.c`)
- [x] ALSA ile FSK ses üretimi (`generator.c`)
- [x] Mikrofon kayıt modülü (`tsonic-rec.c`)
- [x] FFT frekans analizi (`tsonic-fft.c`)
- [x] Preamble senkronizasyonu (`0xD5` sync byte)
- [x] SECDED(13,8) hata düzeltme kodu
- [x] Text transferi başarılı ("Merhaba ThruSonic!")

### 🚧 Devam Edenler
- [ ] Adaptif threshold (ortam gürültüsüne göre otomatik ayar)
- [ ] Binary dosya transferi optimizasyonu

### ⏳ Planlananlar
- [ ] Harici USB mikrofon ile ultrasonik mod testi (18kHz/19kHz)
- [ ] Reed-Solomon hata düzeltme (SECDED'den daha güçlü , bakılacak)
- [ ] Bandpass filtre (sadece hedef frekansları geçirme)
- [ ] Gerçek zamanlı (real-time) transfer modu
- [ ] GUI (Grafik Arayüz) geliştirme
- [ ] Çoklu dosya transferi desteği
- [ ] Şifreleme (AES) desteği




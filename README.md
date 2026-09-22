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

## 🧠 Hata Düzeltme: Hamming → SECDED

İlk olarak **Hamming(7,4)** kodu denenmiştir. Ancak 2+ bit hatasında **yanlış
düzeltme** yaptığı için veriyi daha da bozuyordu. Bu yüzden **SECDED(13,8)**
(Single Error Correction, Double Error Detection) koduna geçilmiştir:

- **8 data biti + 5 parity biti = 13 bit**
- 1 bit hata → **düzeltilir** ✅
- 2 bit hata → **tespit edilir, byte atlanır** (yanlış veri yazılmaz) ⚠️
- 3+ bit hata → tespit edilemez (çok nadir)

## 📊 Test Sonuçları (Gerçek Veriler)

| Test | Frekanslar | Süre | Sonuç |
|------|-----------|------|-------|
| Text Transferi (sessiz) | 6kHz / 8kHz | 50ms/bit | ✅ "Merhaba ThruSonic!" |
| Text Transferi (gürültülü) | 6kHz / 8kHz | 50ms/bit | ⚠️ "erhaba ThruSoic!" (2 harf bozuk) |
| Binary Transferi (128 byte) | 6kHz / 8kHz | 50ms/bit | ❌ Yüksek hata oranı |
| Ultrasonik (18/19 kHz) | 18kHz / 19kHz | - | ❌ Donanım sınırı |

**Stabil Ayarlar:** 6.000 Hz (0) / 8.000 Hz (1), BIT_DURATION 0.05s, SECDED(13,8).

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




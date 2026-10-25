# POASRISER

Riser / sweep generator, VST3. FL Studio ve Studio One'da çalışır.

## Nasıl çalışır

POASRISER bir enstrüman plugin'i. Piyano roll'a nota çizdiğinde riser başlar.

- Uzunluk: LEN butonundan seçilir (1/2, 1, 2, 4, 8 bar). Süre projenin BPM'ine göre hesaplanır.
- Nota yüksekliği riser'ın perdesini değiştirir. C5 (MIDI 60) orijinal perdedir.
- Nota kısa olsa da riser sonuna kadar çalar. LOOP açıksa nota basılı kaldığı sürece tekrar eder, nota bitince durur.
- LISTEN: plugin içinden önizleme.
- Knob: INTENSITY. Filtre aralığını, pitch yükselişini ve gürültü miktarını artırır. Çift tık = 50.
- REVERSE / FALL: yükselen mi düşen mi. Yazıya tıklayınca değişir.
- PHONE, CHORUS, TREMOLO: efektler. RAW seçilirse efektler bypass olur.
- Preset okları ve kaydet butonu üstte.

## Derleme (bilgisayarında hiçbir şey kurmadan)

1. GitHub'da boş bir repo aç.
2. Bu klasörün içindekileri repoya yükle (`.github` klasörü dahil).
3. Repo sayfasında **Actions** sekmesine gir, **Build POASRISER** çalışmasının bitmesini bekle (10-15 dk).
4. Çalışmanın altındaki **Artifacts** bölümünden `POASRISER-Windows-VST3` dosyasını indir.

## Kurulum (Windows)

1. İndirdiğin zip'i aç, içindeki `POASRISER.vst3` klasörünü şuraya kopyala:
   `C:\Program Files\Common Files\VST3`
2. FL Studio: Options > Manage plugins > Find installed plugins.
   Studio One: Studio One > Options > Locations > VST Plug-ins > Reset Blacklist ve Rescan.
3. POASRISER'ı instrument olarak ekle.

## Yerelde derlemek istersen

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target POASRISER_VST3
```

Gereken: CMake 3.22+, Visual Studio 2022 (Windows) veya Xcode (Mac). JUCE otomatik indirilir.

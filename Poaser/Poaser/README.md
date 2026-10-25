# Poaser

Reverb + pitch shifter + reverse riser VST3 plugini (JUCE / C++).
Atmosriser'dan ilham alınmış, kodu tamamen sıfırdan yazılmıştır.

## Özellikler
- **Reverb** (Fruity Reverb tarzı kontroller): Predelay, Low Cut, High Cut, Size, Diffusion, Decay, Damping, Mod Speed, Mod Depth, Stereo
- **Pitch shifter**: Pitch (±24 st), Fine (±100 ct), Pitch Mix, Grain; mod: Pre Reverb / Post Reverb / **Shimmer**
- **Reverse**: gerçek zamanlı ters reverb (Reverse mix + Rise Time)
- **Riser Builder** (Atmosriser mantığı): REC ile sesi kaydet → RENDER → oluşan ters reverb riser'ı DAW'ına sürükle-bırak (veya SAVE WAV)
- **87 preset** (Rooms, Halls, Plates, Spaces, Risers, Shimmer, Pitch, Creative, Vocal)

## Derleme (kurulum gerektirmeyen yol)
1. Klasörü bir GitHub reposuna yükle.
2. **Actions** sekmesinde "Build Poaser" çalışır, bitince **Artifacts** altından `Poaser-Windows-VST3` indir.
3. `Poaser.vst3` klasörünü `C:\Program Files\Common Files\VST3` içine kopyala, DAW'da tara.

## Yerel derleme (Windows)
Visual Studio 2022 (C++ iş yükü) ve CMake gerekir:

    cmake -S . -B build
    cmake --build build --config Release --target Poaser_VST3

Çıktı: `build/Poaser_artefacts/Release/VST3/Poaser.vst3`

## Riser Builder kullanımı
1. Plugini vokal kanalına ekle, **REC**'e bas, DAW'da klibi çal, tekrar **REC**'e bas.
2. İstediğin reverb/pitch ayarını yap, **RENDER**'a bas.
3. Alttaki **DRAG RISER** alanından sesi DAW'ın arrangement'ına sürükle (riser vokalin başlangıcına doğru yükselir).

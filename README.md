# SensorViz - Araç Sensör Görselleştirme Sistemi

Gerçek zamanlı Lidar sensör verilerini simüle eden ve görselleştiren C++ OpenGL uygulaması.

## Özellikler

- 🚗 **Araç Simülasyonu**: Merkez kırmızı üçgen ile araç konumu
- 📡 **360° Lidar Sensör**: Gerçek zamanlı dinamik sensor verileri
- 🎮 **İnteraktif Kontroller**: Fare ve klavye ile kamera kontrolü
- ⚡ **Gerçek Zamanlı**: Sürekli güncellenen animasyon sistemi

## Görselleştirme

- **Kırmızı üçgen**: Araç simgesi (merkez)
- **Yeşil noktalar**: Lidar sensör verileri (360 nokta)
- **Gri çizgiler**: Koordinat sistemi referansı

## Kontroller

### Fare
- **Tekerlek**: Zoom in/out (0.1x - 5.0x)
- **Sol tık + sürükle**: Kamera kaydırma (pan)

### Klavye
- **WASD**: Hareket
- **Q/E**: Zoom in/out
- **R**: Kamera reset
- **ESC**: Çıkış

## Teknik Özellikler

- **OpenGL ES 2.0** uyumlu
- **GLFW 3.4** pencere yönetimi
- **GLM** matematik kütüphanesi
- **GLAD** OpenGL loader
- **Shader** tabanlı rendering

## Kurulum

### Gereksinimler
- MinGW-w64 (GCC 13.2.0+)
- CMake 3.10+
- Windows 10/11+

### Derleme

```bash
cd SensorViz
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
```

## Çalıştırma

```bash
cd build
./SensorViz_manual.exe
```

## Proje Yapısı

```
SensorViz/
├── src/
│   ├── main.cpp      # Ana uygulama
│   └── glad.c        # OpenGL loader
├── shaders/
│   ├── simple.vert   # Vertex shader
│   └── simple.frag   # Fragment shader
├── include/          # Header dosyaları
├── libs/            # Kütüphaneler
└── build/           # Derlenmiş dosyalar
```


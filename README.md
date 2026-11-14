# PPLFontMaker

## Übersicht
- Tool zum Erzeugen von Bitmap-basierten Fonts aus TrueType oder OpenType Fonts, die mit pplib verwendet werden können.
- Kann die Bitmaps in verschiedenen Bittiefen speichern:
  - Monochrom: 1-Bit oder 8-Bit
  - mit Antialiasing: 2-Bit, 4-Bit oder 8-Bit
- Unterstützt verschiedene Größen in einer Datei
- Unterstützt fette und kursive Fonts
- Die zu rendernden Zeichen können ausgewählt werden

## Dateiformat
Siehe auch: [Font-Format Version 6](font6.md), [Font-Format Version 5](font5.md) und [PFP-File-Format Version 3](pfpfile_v3.md)

## Build
Voraussetzungen: CMake ab 3.20, C++20-Compiler, pplib8 (core und grafix) sowie FreeType.

```sh
cmake -S . -B build
cmake --build build
cmake --install build
```

pplib8 wird über `CMAKE_PREFIX_PATH` gefunden; die Top-Level-Installation (`~/lib` bzw. `~/lib64`)
ist bereits voreingestellt. Alternativ explizit angeben:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/pfad/zu/pplib8-installation
```

Ein Makefile als Wrapper bildet CMake pro Build-Variante in einem eigenen
Build-Ordner ab, so dass release, debug und asan nebeneinander existieren:

```sh
make release                       # build/release
make debug                         # build/debug
make asan APP_ARGS="-q a.ttf -t test.fnt6 -6 --aa8 -s 12"   # build/asan + Sanitizer-Report
make test                          # Unittests (GoogleTest)
make coverage                      # Unittests mit Coverage-Auswertung (gcovr)
```

  

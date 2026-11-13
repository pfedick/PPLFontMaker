# DESIGN.md – Migration PPLFontMaker auf CMake und pplib8

Stand: 2026-11-13. Status: **umgesetzt** (Entscheidungen des Nutzers sind eingearbeitet, siehe letzter Abschnitt).

## Zweck

- Build-System von Autoconf/Makefile auf CMake umstellen.
- Bibliothek von ppl7 auf pplib8 umstellen (Namespace `pplib`, aufgeteilte Header).
- Verhalten, CLI und Dateiformate bleiben unverändert.

## Nicht-Ziele

- Keine neuen CLI-Optionen, keine Änderungen an Font-Format 5 oder 6.
- Kein Umbau der Generator-Klassen (Klassenschnitt, Dateiaufteilung bleibt).
- Font5-Unterstützung wird portiert und wieder aktiviert (`-5`, `-l` bei Font5-Dateien).
- Keine Modernisierung des Render-Pfads (`common.cpp`, FreeType-Blitting) über die
  Namespace-Umstellung hinaus. Bestehende C-Casts und `malloc/free` bleiben, sofern
  nicht unten im Abschnitt "Defekte" anders beschlossen.

## Vorgefundener Build (Ausgangslage)

- `configure.ac` prüft umfangreiche ppl7-Dependencies (zlib, bzip2, pcre2, png, jpeg,
  freetype, iconv), erzeugt aber nur `Makefile` und `pplib/Makefile`.
- `Makefile.in`: Ziel `pplfontmaker`, Objekte nur `main.o`, `common.o`, `font6maker.o`.
  **`src/font5maker.cpp` ist nicht im Build.** ppl7 wird als Submodul `pplib/`
  gebaut (`release/libppl7-core.a`, `release/libppl7-gfx.a`).
- `update_version` generiert `include/version.h` (FM_VERSION, FM_REVSION, FM_BUILDDATE)
  **ins Repo**. Zusätzlich stehen in `include/fontmaker.h:14` eigene `FM_VERSION`
  (1.4.0) und `FM_RELEASEDATE` – Dopplung/Widerspruch.
- C++-Standard laut `AX_CXX_COMPILE_STDCXX(23, noext, mandatory)`.

## Vorgeschlagener CMake-Aufbau

```
CMakeLists.txt
include/version.h.in      (wird nach build/include/version.h generiert)
```

- `project(pplfontmaker VERSION 1.6.0 LANGUAGES CXX)`
- `CMAKE_CXX_STANDARD 20`, `CXX_EXTENSIONS OFF` (pplib8 ist C++20).
- `find_package(pplib8 REQUIRED)` – Targets `pplib::core` und `pplib::grafix`;
  `$HOME/lib` und `$HOME/lib64` werden automatisch an `CMAKE_PREFIX_PATH` angehängt.
- `find_package(Freetype REQUIRED)` – Target `Freetype::Freetype`.
- zlib/bzip2/pcre2/png/jpeg werden nicht mehr direkt benötigt; sie sind
  Implementierungsdetail von pplib8 und müssen nur dann angegeben werden, wenn
  pplib8 sie als INTERFACE-Requirements nicht mitliefert.
- Ein Executable-Target `pplfontmaker` aus `src/main.cpp`, `src/common.cpp`,
  `src/font5maker.cpp` und `src/font6maker.cpp`.
- `install(TARGETS pplfontmaker RUNTIME DESTINATION bin)` ersetzt
  `cp $(PROGRAM) @prefix@/bin`.
- Versions-Header künftig per `configure_file` ins Build-Verzeichnis
  (`build/include/version.h`), Repo bleibt unverändert (`include/version.h` gelöscht).
  Revision aus `git rev-list --count`, BUILDDATE aus `string(TIMESTAMP …)`;
  beides nur zur Konfigurationszeit, Revision-Fallback `0` für Nicht-Git-Trees.
- Keine RPATH-Setzung, es gibt nur ein Executable-Target.

## Öffentliche Schnittstelle

- **CLI**: unverändert; `-5` ist wieder verfügbar (vorher: "wird zur Zeit nicht
  unterstützt"), ebenso `-l`/`-ll` für Font5-Dateien.
- **`Main`**, **`CFontGeneratorCommon`**, **`CFont6Generator`**, **`Font6Glyph`**:
  Signaturen bleiben, nur Typnamen `ppl7::` → `pplib::`.
  Ausnahmen: `LoadRequest()` entfällt (siehe Abschnitt "Fehlermodell") und
  `CFont5Generator::CharRanges` ist jetzt `std::vector<std::pair<wchar_t, wchar_t>>`.
- **Header-Splitt statt `ppl7.h` / `ppl7-grafix.h`** (geprüft gegen die installierten
  pplib8-Header in `~/include/pplib8`):

  | Verwendetes Symbol | pplib8-Header |
  |---|---|
  | `pplib::String`, `pplib::WideString` | `pplib/types/string.h`, `pplib/types/widestring.h` |
  | `pplib::Array` | `pplib/types/array.h` |
  | `pplib::File` | `pplib/core/file.h` |
  | `pplib::DirEntry` | `pplib/core/dir.h` |
  | `pplib::PFPFile`, `pplib::PFPChunk` | `pplib/core/pfpfile.h` |
  | `pplib::Compression` | `pplib/core/compression.h` |
  | `pplib::Exception`, `pplib::OutOfMemoryException` | `pplib/exceptions.h` |
  | `Peek8/16/32`, `Poke8/16/32` | `pplib/core/functions.h` |
  | `pplib::HaveArgv`, `pplib::GetArgv` | `pplib/core/args.h` |
  | `pplib::RegEx` | `pplib/core/regex.h` |
  | `pplib::grafix::Drawable`, `::Color` | `pplib/grafix/drawable.h`, `pplib/grafix/color.h` |

## Ownership und Speicherverwaltung

- Unverändert: `Main::font` ist roher `CFontGeneratorCommon*` mit `new`/`delete`;
  `Font6Glyph` hält `char* header`/`char* bitmap` aus `malloc`; die FACE-Chunks gehen
  per `addChunk(std::move(chunk))` in das Eigentum von `PFPFile` über (pplib8 hat keinen
  Zeiger-Overload mehr); `FONTRENDER::buffer` gehört dem Renderer, bis `AddGlyph()` sie
  übernimmt (Kopie in die Map, Original wird mit `g` freigegeben).
- **Defekt 1 (UB, in jedem Build):** `Font6Glyph::operator=` in `src/font6maker.cpp:41`
  befreite `malloc`-Speicher mit `delete` (`delete (header); delete (bitmap);`).
  Auf `free()` korrigiert; kein `std::vector` (Entscheidung des Nutzers).
- `CFont5Generator` hatte ein ungenutztes Mitglied `PFPChunk* fontchunk`, das im
  Konstruktor einen leeren Chunk namens "FONT" anlegte. Das Font5-Format kennt keinen
  solchen Chunk (ID steht im PFP-Header) – Member und Chunk entfernt.
- Kein neuer roher `new`/`delete`-Code in neu geschriebenem CMake-/Hilfscode.

## Fehlermodell

- Bleibt wie gehabt: pplib-Exceptions (`pplib::Exception`, `pplib::OutOfMemoryException`)
  werden in `main.cpp` an den bekannten Stellen gefangen und mit `e.print()` gemeldet;
  Generator-Methoden liefern weiterhin `int` 0/1. Keine einheitliche Exception-API,
  kein `std::error_code` – bewusst Nicht-Ziel.
- Ausnahmen aus `Generate()`/`AddGlyph()` bleiben uncaught (Tool bricht ab) – wie bisher.
- **`PFPFile::LoadRequest()` existiert in pplib8 nicht mehr.** Der bisherige Hook zur
  Prüfung von Datei-ID und Version ist durch eine explizite Prüfung in `Main::work()`
  ersetzt: vor `load()` wird `ident(target)` aufgerufen und danach
  `getID() == "FONT"` sowie `getMainVersion() == fontVersion` kontrolliert; bei
  Abweichung bricht das Programm mit Fehlermeldung ab.

## Thread-Safety

- Programm ist eindrahig; kein geteilter variabler Zustand außer FT-Handles pro
  Generator-Instanz (`ftlib`, `face`) und der Chunk-Liste der Basisklasse. Keine Mutexe.
- `FT_Library`-Handle bleibt pro Objekt erzeugt und im Destruktor geschlossen.

## Plattform-Grenzen

- Zielplattformen wie in pplib8: Linux, FreeBSD, Windows/MinGW-w64 (MSYS).
- FreeBSD-Pfade (`-I/usr/local/include`, `-L/usr/local/lib`) bleiben als
  `CMAKE_PREFIX_PATH`/Toolset-Sache erhalten, kein Autotest dafür.
- `setlocale(LC_ALL, ".UTF-8")` und `SetConsoleOutputCP(CP_UTF8)` unter `WIN32` bleiben.
- `wchar_t` ist auf Linux 4 Byte, auf Windows 2 Byte. `AddCharRange(wchar_t,wchar_t)`
  mit Bereichen > 0xFFFF ist unter Windows fehlerhaft (Schleifenüberlauf) – betrifft
  Frage 3.

## Tests

- Vorhanden sind nur interaktive Shell-Skripte (`makefonts.sh`, `test.sh`), kein
  Test-Framework. Gemäss `cpp.instructions.md` wären Google-Test-Tests vorgesehen;
  der Nutzer hat für diese Migration **keine Tests** gewünscht, auch wenn `-5` damit
  eine ungetestete Codefläche zurück ins Build bringt.
- Für eine spätere Testrunde: Round-Trip über die öffentliche API in
  `tests/test_font5.cpp` / `tests/test_font6.cpp` mit Fixture, das ein Face einer
  kleinen Zeichengruppe (ASCII-Bereich, `--aa4`, Mono1) rendert, in Temp-Datei
  speichert, neu lädt und FACE-Header/Glyph-Zahl prüft. Erwartet: FreeType + pplib8
  im Testlauf.

## Entscheidungen (ehemals offene Punkte)

1. **pplib8-Liespfad:** `~/lib/cmake/pplib8` ( nicht `lib64`); `CMakeLists.txt` hängt
   beide Präfixe an `CMAKE_PREFIX_PATH`. Header liegen in `~/include/pplib8`.
2. **`src/font5maker.cpp`:** migriert und wieder ins Build genommen; `-5` ist
   implementiert. Grund laut Nutzer: die Font5-Unterstützung war offenbar nie von ppl6
   auf ppl7 portiert worden.
3. **Zeichenwerte:** bleibt `wchar_t` (Font-Renderer und `pplib::WideString` nutzen
   `wchar_t`).
4. **Autoconf-Artefakte:** gelöscht (`configure`, `configure.ac`, `aclocal.m4`,
   `Makefile.in`, `Makefile`, `autoconf/`, `genConfigure`, `autom4te.cache`).
5. **`Font6Glyph`-Speicher:** nur `delete` → `free`.
6. **C++-Standard:** C++20 (wie pplib8).
7. **Tests:** keine.

Zusätzliche Entscheidungen gegenüber dem Entwurf, weil pplib8 es so vorgibt:

- `PFPFile::LoadRequest()` ist weg → Formatprüfung jetzt in `Main::work()`.
- `CFont5Generator::CharRanges`: `CAssocArray` (ppl6-API) gibt es nicht mehr,
  `pplib::AssocArray` ist variant-basiert und passt nicht auf numerische
  Startcodepunkte → `std::vector<std::pair<wchar_t, wchar_t>>`.
- `CFont5Generator::fontchunk` (leerer "FONT"-Chunk) entfernt; Font5-Dateien bekommen
  wie Font6 nur FACE-Chunks (ID und Version stehen im PFP-Header).
- `CFont5Generator::enableHints()` als No-op ergänzt, damit die Klasse nicht abstract
  bleibt; Font5 speichert keine Hints.
- Versionsnummer dadurch auf **1.6.0** (Font5 wieder nutzbar), `FM_VERSION`/
  `FM_RELEASEDATE` aus `fontmaker.h` zugunsten des generierten Headers entfernt.
- Gefundener Alt-Fehler in `-l`/`-ll` behoben: bisher wurde `LoadFont()` (FreeType-TTF)
  statt `load()` (PFP-Datei) aufgerufen; zusätzlich wird jetzt die Fontversion
  (5 oder 6) aus dem Dateinamens-Suffix abgeleitet und geprüft.

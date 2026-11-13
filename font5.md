# Format PFP Font, Version 5

Die Version 5 verwendet als Basisformat das [PFP-File-Format Version 2](#) mit seinem Header. Die Dateien tragen die Endung `*.fnt5`. Eine Datei enthält in der Regel immer nur einen Font (z.B. Liberation Sans), kann aber beliebig viele Faces enthalten, die sich durch Style (Fett, Kursiv, Antialiased) und Pixelgröße unterscheiden.

Das File trägt als ID "FONT", Hauptversion 5, Unterversion 0.

## PFP-Header

Jede Datei beginnt mit dem 40 Byte großen PFP-Header:

```text
Byte  0: "PFP-File"                                     (8 Byte)
Byte  8: PFP-File-Version (2)                           (1 Byte)
Byte  9: Laenge des PFP-Header (40)                     (1 Byte)
Byte 10: File-ID ("FONT")                               (4 Byte)
Byte 14: Unterversion (0)                               (1 Byte)
Byte 15: Hauptversion (5)                               (1 Byte)
Byte 16: Zeiger auf Name                                (2 Byte)
Byte 18: Zeiger auf Autor                               (2 Byte)
Byte 20: Erstelldatum                                   (4 Byte)
Byte 24: Erstellzeit in Sekunden                        (4 Byte)
Byte 28: Datum letzte Aenderung                         (4 Byte)
Byte 32: Zeit letzte Aenderung in Sekunden              (4 Byte)
Byte 36: Komprimierung                                  (2 Byte)
```

### Beschreibung:

- **PFP-File-Version**
  Versionsnummer des PFP-Basisformats. Für Font 5 ist das die Version 2.
- **Laenge des PFP-Header**
  Länge des Headers in Bytes, bei Version 2 konstant 40.
- **File-ID**
  Kennung des Dateityps, hier immer "FONT".
- **Unterversion / Hauptversion**
  Versionsnummer des Font-Formats, hier 5.0.
- **Zeiger auf Name / Zeiger auf Autor**
  Offset der zugehörigen 0-terminierten Zeichenketten.
- **Erstelldatum / Erstellzeit**
  Datum und Zeit (Sekunden seit Mitternacht) der Erstellung.
- **Datum letzte Aenderung / Zeit letzte Aenderung**
  Datum und Zeit der letzten Änderung.
- **Komprimierung**
  Verfahren der ggf. verwendeten Komprimierung, 0 bei unkomprimierten Files.

Alle weiteren Adressen und Pointer sind relativ zum Start des Font-Headers (also ab Byte 40) zu sehen!

## Font-Header

Direkt nach dem PFP-Header folgt der Font-Header:

```text
Byte 40 (0):  Anzahl Fonts                           (2 Byte)
Byte 42 (2):  Pointer auf Font-Index                 (2 Byte)
Byte 44 (4):  Name + 0-Byte
              dann Autor + 0-Byte
              dann Font-Index
```

### Beschreibung:

- **Anzahl Fonts**
  Anzahl der in der Datei enthaltenen Faces.
- **Pointer auf Font-Index**
  Offset zum Font-Index, der genau wie die Namen direkt an den Header anschließt.

## Font-Index

Der Font-Index enthält pro Face genau drei Einträge:

```text
Byte 0: Size                                         (1 Byte)
Byte 1: Flags                                        (1 Byte)
Byte 2: Chunk-Address                                (4 Byte)
```

### Beschreibung:

- **Size**
  Größe des Faces in Pixel.
- **Flags**
  Folgende Flags sind definiert:
  ```text
  Bit 0: Font ist fett (bold)
  Bit 1: Font ist kursiv (italic)
  Bit 2: Font benutzt Antialiasing
  Bit 3: Font hat Kontur (outline)
  Bit 4: Font hat Schatten (shadow)
  ```
- **Chunk-Address**
  Offset des Font-Chunks, in dem alle Daten dieses Faces liegen.

## Font-Chunk

Alle Pointer innerhalb des Chunks sind relativ zum Anfang des Font-Chunks zu sehen!

```text
Byte  0: Pointer auf Char-Tabelle                    (2 Byte)
Byte  2: Pointer auf Farbtabelle (0 bei Monochrom)   (2 Byte)
Byte  4: Hoehe der Fonts                             (2 Byte)
Byte  6: Grundlinie                                  (2 Byte)
Byte  8: Abstand zwischen Buchstaben                 (1 Byte)
Byte  9: Abstand fuer Space                           (1 Byte)
Byte 10: Maximale Breite der Buchstaben              (2 Byte)
Byte 12: Minimale Breite der Buchstaben              (2 Byte)
Byte 14: Durchschnittliche Breite der Buchstaben     (2 Byte)
Byte 16: Anzahl Farben                               (1 Byte)
Byte 17: Bits pro Pixel bei Anti Aliased Fonts       (1 Byte)
```

### Beschreibung:

- **Pointer auf Char-Tabelle**
  Offset der Zeichentabelle. Diese bildet ISO-8859-15 ab und besteht aus 16 Byte pro Zeichen. Sie beginnt an einer durch 4 teilbaren Adresse.
- **Pointer auf Farbtabelle**
  Offset der Farbtabelle, 0 bei monochromen Fonts.
- **Hoehe der Fonts**
  Höhe aller Glyphs des Faces in Pixel.
- **Grundlinie**
  Position der Grundlinie innerhalb der Glyph-Height.
- **Abstand zwischen Buchstaben**
  Standardabstand, der beim Darstellen zwischen zwei Zeichen zusätzlich berücksichtigt wird.
- **Abstand fuer Space**
  Breite des Leerzeichens.
- **Maximale / Minimale / Durchschnittliche Breite der Buchstaben**
  Statistische Werte, aus denen die Applikation zum Beispiel die Gesamtbreite von Text abschätzen kann, ohne jedes Glyph einzeln zu lesen.
- **Anzahl Farben**
  Anzahl der im Font verwendeten Farben, 1 bei monochromen Fonts.
- **Bits pro Pixel bei Anti Aliased Fonts**
  Bittiefe der Graustufen bei antialiasten Fonts (Standard: 2).

### Char-Tabelle

Die Char-Tabelle hat 16 Byte pro Zeichen und ist über den Zeichenwert direkt indexierbar:

```text
Byte  0: Pointer auf Bitmap                           (4 Byte)
Byte  4: Pointer auf Rahmen-Map                       (4 Byte)
Byte  8: Pointer auf Shadow-Map                       (4 Byte)
Byte 12: Breite                                       (2 Byte)
Byte 14: Hoehe                                        (2 Byte)
```

#### Beschreibung:

- **Pointer auf Bitmap**
  Offset der Bitmap dieses Zeichens (0, wenn das Zeichen nicht vorkommt). Bei mehrfarbigen Fonts zeigt der Pointer auf die Bitmap der ersten Farbe.
- **Pointer auf Rahmen-Map**
  Offset der Bitmap der Kontur, nur bei Font-Flag "has outline" ausgewertet.
- **Pointer auf Shadow-Map**
  Offset der Bitmap des Schattens, nur bei Font-Flag "has shadow" ausgewertet.
- **Breite / Hoehe**
  Ausdehnung des Zeichens in Pixel.

### Bitmaps

Die Bitmaps folgen hintereinander, pro Farbe eine Bitmap. Der Anfang ist immer auf 4 Byte aligned:

```text
Byte 0: Bytes der Bitmap inkl. Header                 (2 Byte)
Byte 2: Offset-X (signed)                             (2 Byte)
Byte 4: Offset-Y (signed)                             (2 Byte)
Byte 6: reserved                                      (2 Byte)
Byte 8: Start der Bitmap
```

#### Beschreibung:

- **Bytes der Bitmap inkl. Header**
  Größe dieser Bitmap einschließlich des 8 Byte großen Headers.
- **Offset-X (signed)**
  Wert, der beim Zeichnen dazuaddiert werden muss, bevor die erste Spalte des Zeichens gezeichnet werden kann.
- **Offset-Y (signed)**
  Wert, der beim Zeichnen dazuaddiert werden muss, bevor die Zeile des Zeichens gezeichnet werden kann.
- **reserved**
  Nicht verwendet, jederzeit 0.
- **Start der Bitmap**
  Die Größe der Bitmap ist immer ein Vielfaches von 4. Bei mehrfarbigen Fonts addiert man *Bytes der Bitmap inkl. Header* zur Adresse der aktuellen Bitmap, um an den Anfang der nächsten Farbe zu gelangen.

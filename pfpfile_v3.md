# Format PFP-Files Version 3

Ein PFP-File in der Version 3 ist in mehrere aufeinanderfolgende Abschnitte aufgeteilt:

- 24 Byte langer Header
- optionaler 8 Byte langer Komprimierungsheader
- Chunks

Alle 4 Byte großen Größenangaben sind im LittleEndian-Format!

## Header

Der Header einer Version 3 Datei sieht so aus:

```text
Byte  0: String "PFP-File"                             (8 Byte)
Byte  8: PFP-File-Version (3)                          (1 Byte)
Byte  9: Länge des PFP-Header (24)                     (1 Byte)
Byte 10: File-ID, 4 Byte-String                        (4 Byte)
Byte 14: Unterversion                                  (1 Byte)
Byte 15: Hauptversion                                  (1 Byte)
Byte 16: Komprimierung                                 (1 Byte)
Byte 17: reserviert                                    (3 Byte)
Byte 20: Timestamp der Erstellung (UTC)                (4 Byte)
```

### Beschreibung:

- **PFP-File-Version**
  Versionsnummer des PFP-Basisformats, hier 3.
- **Länge des PFP-Header**
  Länge des Headers in Bytes, hier konstant 24. Alle nachfolgenden Offset-Werte sind relativ zum Anfang des Headers zu sehen.
- **File-ID**
  4 Byte langer String, der den Inhalt der Datei kennzeichnet, zum Beispiel "FONT".
- **Unterversion / Hauptversion**
  Versionsnummer des in der Datei enthaltenen Formats. Diese Werte gehören zum jeweiligen Anwendungsformat, nicht zum PFP-Basisformat.
- **Komprimierung**
  Folgende Werte sind definiert:
  ```text
  0 = unkomprimiert
  1 = Zlib
  2 = Bzip2
  ```
- **Timestamp der Erstellung (UTC)**
  Erstellungszeitpunkt in Sekunden seit dem 01.01.1970 (UTC). Anmerkung: Dieses Feld ist derzeit nur 4 Byte groß und sollte für 64 Bit Zeitstempel auf 8 Byte erweitert werden.

Im Anschluss an den Header folgen die Nutzdaten. Sofern keine Komprimierung verwendet wurde, geht es sofort mit dem ersten Chunk los. Ist die Datei komprimiert, folgt erst der Komprimierungsheader.

## Komprimierungsheader

```text
Byte 0: Größe der Nutzdaten unkomprimiert in Byte      (4 Byte)
Byte 4: Größe der Nutzdaten komprimiert in Byte        (4 Byte)
```

### Beschreibung:

- **Größe der Nutzdaten unkomprimiert**
  Größe aller Chunks nach dem decomprimierten Zustand.
- **Größe der Nutzdaten komprimiert**
  Größe der anschließend im File gespeicherten komprimierten Daten.

## Chunks

In einem PFP-File können beliebig viele Chunks vorkommen. Ein Chunk besteht immer aus einem 4 Byte langen Namen, gefolgt von einem 4 Byte Integer, der die Größe des Chunks einschließlich des Headers angibt, gefolgt von den Nutzdaten. Abgesehen von den unten aufgeführten vordefinierten Chunks können beliebig viele Chunks mit gleichem Namen vorhanden sein.

Ein Chunk muss nicht zwingend Nutzdaten enthalten.

```text
Byte 0: Chunkname, 4 Byte-String in Grossbuchstaben    (4 Byte)
Byte 4: Größe des Chunks einschließlich 8 Byte Header  (4 Byte)
Byte 8: Nutzdaten des Chunks
```

### Beschreibung:

- **Chunkname**
  4 Byte langer Name in Großbuchstaben, zum Beispiel "AUTH" oder "NAME".
- **Größe des Chunks**
  Größe des gesamten Chunks inklusive des 8 Byte großen Chunk-Headers, also einschließlich des Namens und dieses Werts selbst. Der nächste Chunk beginnt an der Adresse dieses Chunks plus dieser Größe.

## Vordefinierte Chunks

Die nachfolgenden Chunks sind vordefiniert, aber optional. Alle vier haben denselben Aufbau:

```text
Byte 0: Chunkname                                      (4 Byte)
Byte 4: Länge des nachfolgenden Strings                (4 Byte)
Byte 8: String mit schließendem 0-Byte
```

- **AUTH**
  Name des Authors. Der Name des Authors kann mit der Funktion `PFPFile::SetAuthor` gesetzt werden.
- **NAME**
  Name der Datei oder Beschreibung des Inhalts. Der Name des Files kann mit der Funktion `PFPFile::SetName` gesetzt werden.
- **DESC**
  Nähere Beschreibung. Die Description kann mit der Funktion `PFPFile::SetDescription` gesetzt werden.
- **COPY**
  Copyright-String. Der Copyright-String kann mit der Funktion `PFPFile::SetCopyright` gesetzt werden.

## End of File

Dieser Chunk ist immer der letzte in der Datei und kennzeichnet das Ende der Nutzdaten.

```text
Byte 0: ENDF                                           (4 Byte)
Byte 4: 0                                              (4 Byte)
```

## Nutzung

Um das Lesen und Schreiben solcher Dateien zu vereinfachen, kann die Klasse `pplib::PFPFile` in Kombination mit `pplib::PFPChunk` verwendet werden.

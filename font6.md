# Format PFP Font, Version 6

Die Version 6 ist angelehnt an Version 5 und verwendet als Basisformat das [PFP-File-Format Version 3](#) mit seinen Chunks. Die Wesentlichen Unterschiede zu Version 5 sind:

- Es gibt keine Sprungtabellen mehr
- Die Glyphs für die vorhandenen Zeichen liegen hintereinander in Chunks, die Applikation muss sich selbst merken, wo ein Glyph zu finden ist (z.B. innerhalb eines binären Baumes)
- Die Glyphs unterstützen Hints, also die Information, ob eine bestimmte Zeichenkombination näher zueinander dargestellt werden können oder mehr Platz brauchen. Beispiel: "To" - das "o" kann in diesem Fall nach links unter den T-Strich gerückt werden. Bei "Tö" geht das aber ja nach Font nicht.

Das File trägt als ID "FONT", Hauptversion 6, Unterversion 0. Eine Datei enthält in der Regel immer nur einen Font (z.B. Liberation Sans), kann aber beliebig viele Faces enthalten, die sich durch Style (Fett, Kursiv, Antialiased) und Pixelgröße unterscheiden. Jedes Face ist in einem eigenen Chunk untergebracht, der den Namen "FACE" trägt.

## Header

Jedes FACE beginnt mit einem 12 Byte großen Header:

```text
Byte 0:  Flags                                        (1 Byte)
Byte 1:  Pixelformat                                  (1 Byte)
Byte 2:  Size (Fontgröße in Pixel)                    (2 Byte)
Byte 4:  MaxBearingY                                  (2 Byte)
Byte 6:  MaxHeight                                    (2 Byte)
Byte 8:  Position Underscore                          (2 Byte)
Byte 10: Anzahl Glyphs                                (2 Byte)
```

### Beschreibung:

- **Flags**
  Folgende Flags sind definiert:
  ```text
  Bit 0: Font benutzt Antialiasing
  Bit 1: Font ist fett (bold)
  Bit 2: Font ist kursiv (italic)
  Bit 3: Font unterstützt Hints
  ```
- **Pixelformat**
  ```text
  0 = undefiniert
  1 = Monochrom, 8 Bit Pro Pixel (1 Byte)
  2 = Monochrom, 1 Bit pro Pixel
  3 = Antialiased, 8 Bit pro Pixel (1 Byte) = 256 Graustufen
  4 = Antialiased, 2 Bit pro Pixel = 4 Graustufen
  5 = Antialiased, 4 Bit pro Pixel = 16 Graustufen
  ```
- **Size**
  Größe der Fonts
- **MaxBearingY**
  Dieser positive Wert gibt an, wieviele Pixel nach oben des größte Glyph abweicht. Da Font6-Fonts immer von einer Grundlinie aus gezeichnet werden, kann man mit diesem Wert den höchsten Punkt im Font-Face ermitteln, um z.B. statt von der Grundline aus von oben aus die Fonts abzubilden (Align=Top)
- **MaxHeight**
  Dieser Wert gibt die maximale Höhe der Glyphs an und gleichzeitig die Anzahl Pixel, die bei einem Zeilenumbruch auf der Y-Achse dazuaddiert werden müssen. Aus der Differenz "MaxHeight-MaxBearingY" kann man ausserdem den tiefsten Punkt im Fontface errechnen, für den Fall dass man die Fonts von unten aus abbilden möchte (Align=Bottom).
- **Position Underscore**
- **Anzahl Glyphs**
  Anzahl Glyphs/Zeichen in diesem Face.

Der Aufbau ist somit fast identisch zu Version 5, nur Byte 10 enthält statt der Anzahl Spruntabellen die Anzahl Glyphs.

## Glyphs

Ein wesentlicher Unterschied zu Version 5 (und allen vorhergehenden) besteht darin, dass es keine Sprungtabellen mehr gibt. Stattdessen folgen nun direkt die Chunks mit den Glyphs. Der Aufbau der Chunks ist folgendermassen:

```text
Byte 0:    Größe des Chunks in Bytes                     (4 Byte)
Byte 4:    Unicode-Wert des Zeichens                     (2 Byte)
Byte 6:    Breite                                        (2 Byte)
Byte 8:    Höhe                                          (2 Byte)
Byte 10:   BearingX (signed)                             (2 Byte)
Byte 12:   BearingY (signed)                             (2 Byte)
Byte 14:   Advance                                       (2 Byte)
Byte 16:   Hints-Tabelle                                 (n Byte)
Byte 16+n: Bitmap                                        (x Byte)
```

### Beschreibung:

- **Größe des Chunks**
  Größe des Chunks in Byte einschließlich dieses Werts
- **Unicode-Wert des Zeichens**
  Dieser Wert ist 2 Byte gross, das heisst es lassen such nur Zeichen abbilden, deren Unicode-Werte zwischen 0 und 65535 liegen.
- **Breite**
  Breite des Zeichens in Pixel
- **Höhe**
  Höhe des Zeichens in Pixel
- **BearingX**
  Wert, der beim Zeichnen dazuaddiert werden muss, bevor die erste Spalte des Zeichens gezeichnet werden kann.
- **BearingY**
  Wert, der beim Zeichnen dazuaddiert werden muss, bevor die Zeile des Zeichens gezeichnet werden kann.
- **Advance**
  Wert, der dazuaddiert werden muss, um an die Position des nächsten darzustellenden Zeichens zu kommen. Falls es zu der Zeichenkombination Hints gibt, muss dessen Wert zu diesem noch dazuaddiert werden.
- **Hints-Tabelle**
  Unterstützt der Font Hints (Bit 3 im Flags-Feld des FACE-Headers ist gesetzt), schließt sich nun die Hints-Tabele an. Ist dies nicht der Fall, folgt sofort die Bitmap. Die Tabelle ist folgendermassen aufgebaut:
  ```text
  Byte 0: Unicode                   (2 Byte)
  Byte 2: Offset (signed)           (2 Byte)
  ```
  Unicode gibt den Unicode-Wert des nachfolgenden Zeichens an, Offset die Verschiebung nach links oder rechts. Das Ende der Tabelle wird dadurch gekennzeichnet, dass der Unicode-Wert 0 ist. Um Platz zu sparen werden in der Tabelle nur Zeichenkombinationen gespeichert, bei denen es auch tatsächlich eine Verschiebung gibt.
- **Bitmap**
  Das Format der Bitmap ergibt sich aus dem Pixelformat im FACE-Header. Die Größe errechnet sich aus *BitsProPixel * Breite * Höhe*, wobei auf volle Byte aufgerundet wird.

Aufgrund dieses Aufbaus läßt es sich nicht errechnen, an welcher Stelle im File sich ein bestimmtes Glyph befinden. Die Applikation muss daher alle Glyphs lesen und sich diese Information selbst erstellen, zum Beispiel innerhalb eines binären Baumes.
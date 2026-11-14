# Format PFP Font, Version 5

Die Version 5 verwendet als Basisformat das [PFP-File-Format Version 3](pfpfile_v3.md) mit seinen Chunks. Die Dateien tragen die Endung `*.fnt5`. Das File trägt als ID "FONT", Hauptversion 5, Unterversion 0. Eine Datei enthält in der Regel immer nur einen Font (z.B. Arial), kann aber beliebig viele Faces enthalten, die sich durch Style (Fett, Kursiv, Antialiased) und Pixelgröße unterscheiden. Jedes Face ist in einem eigenen Chunk untergebracht, der den Namen "FACE" trägt.

Alle Integer-Werte sind im Little-Endian-Format gespeichert.

## Header

Jedes FACE beginnt mit einem 12 Byte großen Header:

```text
Byte 0:  Flags                                        (1 Byte)
Byte 1:  Pixelformat                                  (1 Byte)
Byte 2:  Size (Fontgröße in Pixel)                    (2 Byte)
Byte 4:  MaxBearingY                                  (2 Byte)
Byte 6:  MaxHeight                                    (2 Byte)
Byte 8:  Position Underscore                          (2 Byte)
Byte 10: Anzahl Sprungtabellen                        (2 Byte)
```

### Beschreibung:

- **Flags**
  Folgende Flags sind definiert:
  ```text
  Bit 0: Font benutzt Antialiasing
  Bit 1: Font ist fett (bold)
  Bit 2: Font ist kursiv (italic)
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
  Dieser positive Wert gibt an, wieviele Pixel nach oben der größte Glyph abweicht. Da Font5-Fonts immer von einer Grundlinie aus gezeichnet werden, kann man mit diesem Wert den höchsten Punkt im Font-Face ermitteln, um z.B. statt von der Grundlinie aus von oben aus die Fonts abzubilden (Align=Top)
- **MaxHeight**
  Dieser Wert gibt die maximale Höhe der Glyphs an und gleichzeitig die Anzahl Pixel, die bei einem Zeilenumbruch auf der Y-Achse dazuaddiert werden müssen. Aus der Differenz "MaxHeight-MaxBearingY" kann man außerdem den tiefsten Punkt im Fontface errechnen, für den Fall dass man die Fonts von unten aus abbilden möchte (Align=Bottom).
- **Position Underscore**
- **Anzahl Sprungtabellen**
  Dieser Wert gibt an, wieviele Sprungtabellen im Face enthalten sind.

## Sprungtabellen

Das Font5-Format sieht vor, dass es mehrere Sprungtabellen geben kann. Dadurch ist es möglich, selektiv nur die Fonts in das File einzubinden, die man tatsächlich benötigt.

```text
Byte 0:  Start Character in Tabelle (UNICODE)         (2 Byte)
Byte 2:  End Character in Tabelle (UNICODE)           (2 Byte)
Byte 4:  Pointer auf Beginn der Sprungtabelle         (4 Byte)
```

### Beschreibung:

- **Start und Ende in Character Tabelle**
  Diese beiden Werte geben an, mit welchem Zeichen die nachfolgende Sprungtabelle beginnt und endet. Da UNICODE verwendet wird, sind die Werte 2 Byte groß und im Little-Endian-Format angegeben.
- **Pointer auf Beginn der Sprungtabelle**
  Ein 4 Byte Pointer im Little-Endian-Format, der den Beginn der Sprungtabelle innerhalb des FACE angibt.

Die Sprungtabellen selbst bestehen aus 4-Byte-Werten im Little-Endian-Format, die den Beginn des jeweiligen Glyphs innerhalb des Font-FACE angeben. Dabei wird der 12 Byte große Header und die Sprungtabelle mitgerechnet.

Die Sprungtabelle enthält nur Pointer für existierende Glyphs. Ist ein Glyph nicht vorhanden, enthält der Wert in der Sprungtabelle 0.

Die Position der ersten Sprungtabelle errechnet sich folgendermaßen:

```text
Position = 12 + Anzahl Sprungtabellen * 8
```

## Glyphs

Ein Glyph besteht aus einem Header und einer Bitmap. Der Header ist folgendermaßen aufgebaut:

```text
Byte 0:  Breite                                       (2 Byte)
Byte 2:  Höhe                                         (2 Byte)
Byte 4:  BearingX (signed)                            (2 Byte)
Byte 6:  BearingY (signed)                            (2 Byte)
Byte 8:  Advance                                      (2 Byte)
Byte 10: Bitmap                                       (x Byte)
```

### Beschreibung:

- **Breite**
  Breite des Zeichens in Pixel
- **Höhe**
  Höhe des Zeichens in Pixel
- **BearingX**
  Wert, der beim Zeichnen dazuaddiert werden muss, bevor die erste Spalte des Zeichens gezeichnet werden kann.
- **BearingY**
  Wert, der beim Zeichnen dazuaddiert werden muss, bevor die Zeile des Zeichens gezeichnet werden kann.
- **Advance**
  Wert, der dazuaddiert werden muss, um an die Position des nächsten darzustellenden Zeichens zu kommen.
- **Bitmap**
  Das Format der Bitmap ergibt sich aus dem Pixelformat im FACE-Header. Die Größe errechnet sich aus *BitsProPixel \* Breite \* Höhe*, wobei auf volle Byte aufgerundet wird. Bei den Pixelformaten mit 1, 2 und 4 Bit pro Pixel werden die Bits mit dem höchstwertigen Bit (MSB) beginnend fortlaufend über das gesamte Glyph gepackt, ohne Auffüllung auf Zeilen-, Byte- oder Wortgrenzen. Wie viele Byte tatsächlich im Glyph stehen, liest ein Parser sicher nur aus dem Abstand zum nächsten Glyph (siehe [Anmerkungen zur Implementierung](#anmerkungen-zur-implementierung)).

Die Position des ersten Glyphs errechnet sich folgendermaßen:

```text
Position = 12 + Anzahl Sprungtabellen * 8 + Anzahl Chars in jeder Sprungtabelle * 4
```

## Anmerkungen zur Implementierung

Die folgenden Punkte sind keine Formatvorgaben, sondern beschreiben, wie PPLFontMaker die Dateien schreibt bzw. wie pplib sie liest:

- Alle Zeiger (auf Sprungtabellen und auf Glyphs) sind relativ zum Beginn der FACE-Chunk-Daten zu sehen, nicht zum Dateianfang.
- Da Unicode-Werte in den Sprungtabellen und Zeigertabellen 2 Byte groß sind, können nur Zeichen im Bereich 0 bis 65535 abgebildet werden.
- Der undokumentierte Wert *Position Underscore* wird von PPLFontMaker aus der TrueType `underline_position` berechnet, ganzzahlig in Pixel, positiv unterhalb der Grundlinie.
- PPLFontMaker polstert Glyphs nicht auf eine bestimmte Ausrichtung; die Glyphen folgen direkt hintereinander. Der Bitmap-Puffer wird als `(Breite * Höhe) >> 3` (1 Bit), `>> 2` (2 Bit) bzw. `>> 1` (4 Bit) plus je 1 Byte berechnet, also ein Byte größer als nötig, wenn die Bitzahl glatt in ein Byte aufgeht. Darauf darf sich ein Leser nicht verlassen, denn der nächste Offset steht in der Sprungtabelle.
- *MaxHeight* wird als `größte Ausdehnung unter der Grundlinie + MaxBearingY + 1` ermittelt, beschreibt also die Zeilenhöhe und nicht allein die Höhe des höchsten Glyphs.
- PPLFontMaker rendert die Konturen mit `Größe + 2` Pixeln (`SetPixelSizes`), damit Überlängen Ober- und Unterlinie nicht abschneiden. Im Feld *Size* des FACE-Headers steht die angeforderte Größe.
- Der Font5-Renderer in pplib wählt das Face über exakte Übereinstimmung von Größe und Flag-Byte aus. Ein Face mit gesetztem Bit 1 (bold) wird nur gefunden, wenn ein fetter Font angefordert ist. Gibt es kein passendes Face, zeichnet der Renderer jedes Glyph des vorhandenen Faces zweimal, das zweite Mal mit um 1 Pixel erhöhter X-Position.
- Das Flag-Feld des FACE-Headers enthält kein Bit für Hints. Kerning-Informationen gibt es erst ab [Version 6](font6.md).

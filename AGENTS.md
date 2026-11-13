#  PPLFontMaker
- Tool zum Erzeugen von Bitmap-basierten Fonts aus TrueType oder OpenType Fonts, die mit pplib verwendet werden können.
- Kann die Bitmaps in verschiedenen Bittiefen speichern:
  - Monochrom: 1-Bit oder 8-Bit
  - mit Antialiasing: 2-Bit, 4-Bit oder 8-Bit
- Unterstützt verschiedene Größen in einer Datei
- Unterstützt fette und kursive Fonts
- Die zu rendernden Zeichen können ausgewählt werden

## Dateiformat
Font 6: [Font-Format Version 6](font6.md)
Font 5: [Font-Format Version 5](font5.md)
PFP-File-Format Version 3: [PFP-File-Format Version 3](pfpfile_v3.md)

  



## Migration
- Umstellung von ppl7 auf pplib8.
- Umstellung von Autoconf auf CMake



## Migration auf pplib8
Das Projekt "pplib8", ist eine plattformübergreifende C++-Bibliothek, die auf verschiedenen Plattformen (Windows MingW64/MSys, Linux, FreeBSD) getestet wird und sowohl auf Desktop- als auch in Teilen auf Mikrocontroller-Umgebungen läuft. Die Bibliothek bietet eine Vielzahl von Funktionen und Abstraktionen, um gängige Programmieraufgaben zu vereinfachen. Sie deckt Datei- und Verzeichnisoperationen, Strings und Arrays, Zeit, Mathematik, Threads, Grafik, Sound, Datenbankzugriff und Internetkommunikation ab.

pplib8 ist die neuere Version von ppl7. Sie verwendet CMake als Build-System und ist im Homeverzeichnis des Benutzers installiert.
Siehe `~/lib64/cmake/pplib8/` für die CMake-Konfigurationsdateien von pplib8. Die Library ist weitestgehend API-kompatibel zu ppl7, weist aber einige Änderungen und Erweiterungen auf.

Gravierenste Ännderungen:
- Namespace: `pplib` statt `ppl7`
- Statt weniger zentraler Header-Dateien gibt es jetzt viele spezialisierte Header-Dateien, zum Beispiel `pplib/types/string.h`. Die zentralen Header-Dateien wie `pplib.h` sind noch vorhanden, sollten aber möglichst nicht mehr direkt inkludiert werden.
- Dir-Klasse wurde stark überarbeitet, insbesondere die Itteratoren sind jetzt standardkonforme C++-Iteratoren.
- File-Klasse: map()-Methoden wurden überarbeitet (tryMap) 
- Audio-Decoder wurde leicht angepasst, getSamples wirft nicht mehr, sondern gibt auch im Fehlerfall 0 zurück.
- Encoder ist größtenteils übernommen worden, ist aber noch ungetestet und könnte Fehler enthalten.




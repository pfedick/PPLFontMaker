# Testdaten

## TestSans.ttf

Ausschnitt aus **Liberation Sans Regular** (Lizenz: SIL Open Font License 1.1,
siehe [LICENSE-TestSans.txt](LICENSE-TestSans.txt)). Die Original-Datei wurde auf
wenige Codepoints reduziert und in "TestSans" umbenannt, damit die Tests
unabhaengig von installierten Systemschriften sind und keine Lizenzprobleme
entstehen.

Enthaltene Codepoints: `U+0020` bis `U+007E`, `U+00A4`, `U+00DF`, `U+20AC`.

Erzeugen (mit Python fontTools):

```bash
pyftsubset /usr/share/fonts/liberation/LiberationSans-Regular.ttf \
  --output-file=tests/testdata/TestSans.ttf \
  --unicodes="U+0020-007E,U+00A4,U+00DF,U+20AC" \
  --layout-features="" --no-hinting --desubroutinize
```

Anschliessend den Font-Namen mit fontTools (oder z.B. FontForge) auf "TestSans"
aendern (Name-IDs 1, 2, 4 und 6).

`tests/tmp/` wird zur Laufzeit der Tests angelegt und ist nicht Teil der
Testdaten.

#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#endif
#include "fontmaker.h"
#include <locale.h>

int main(int argc, char** argv)
{
    Main m;
#ifdef WIN32
    // Windows: Konsole auf UTF-8 Output schalten
    SetConsoleOutputCP(CP_UTF8);
#endif
    setlocale(LC_ALL, ".UTF-8");
    return m.start(argc, argv);
}

Main::Main()
{
    fontVersion = 6;
    font = NULL;
    flags = 0;
}

Main::~Main()
{
}

void Main::version()
{
    ppl7::String s;
    s.setf("PPL FONTMAKER Version %s vom %s", FM_VERSION, FM_RELEASEDATE);
    s.print(true);
    s.repeat("=", s.len());
    s.print(true);
    printf("Author: %s\n", FM_AUTHOR);
    printf("Copyright: %s\n", FM_COPYRIGHT);
}

void Main::help()
{
    version();
    printf("\n"
           "Syntax:\n"
           "    font5maker -t TARGET -q TTF -s # [FLAGS] [-c #-#]\n"
           "\n"
           "Parameter:\n"
           "  -t TARGET   Name der Zieldatei. Ist diese bereits vorhanden\n"
           "              werden neue Faces hinzugefügt.\n"
           "  -q TTF      Quelldatei mit den TrueType-Fonts\n"
           "  -s #        Pixelgröße, die generiert werden soll\n"
           "              Hier kann auch eine Range angegeben werden (z.B. 8-20)\n"
           "              oder eine kommagetrennte Liste (z.B. 8,10,12,14,20)\n"
           "  -c #-#      Unicode Bereich, der in das Fontface integriert werden\n"
           "              soll. Es können mehrere kommagetrennte Bereiche\n"
           "              angegeben werden. Default = 32-255,8364 (€-Symbol),\n"
           "              7838 (ẞ, grosses SZ)\n"
           //           "  -5          PPL-Font Version 5 generieren\n"
           "  -6          PPL-Font Version 6 generieren (=Default)\n"
           "Flags:\n"
           "  --aa        Antialiased Fonts erstellen\n"
           "  --aa2       Antialiased Fonts mit 2 Bit pro Pixel\n"
           "  --aa4       Antialiased Fonts mit 4 Bit pro Pixel\n"
           "  --aa8       Antialiased Fonts mit 8 Bit pro Pixel (default)\n"
           "  --mono1     Monochrome Fonts mit 1 Bit pro Pixel erstellen (default)\n"
           "  --mono8     Monochrome Fonts mit 8 Bit pro Pixel erstellen\n"
           "  --isbold    Quelldatei enthält fette Fonts\n"
           "  --genbold   Es sollen fette Fonts generiert werden\n"
           "              Dazu wird jeder Buchstabe zweimal gezeichnet und\n"
           "              dabei um einen Pixel nach rechts versetzt\n"
           "  --isitalic  Quelldatei enthält kursive Fonts\n"
           "  --name \"\"   Name der Datei\n"
           "  --author \"\" Author der Datei\n"
           "  --copy \"\"   Copyright der Datei\n"
           "  --zlib      Zlib-Kompression verwenden (Default=unkomprimiert)\n"
           "  --bzip2     Bzip2-Kompression verwenden (Default=unkomprimiert)\n"
           "  --idn       Fügt alle für .de-Domains erlaubten IDN-Zeichen hinzu\n"
           "  --disable-hints  Hints deaktivieren (nur in Font6)\n"
           "\n"
           "Sonstiges:\n"
           "  -h oder --help Zeigt diese Hilfe an\n"
           "  -l FILE     Zeigt den Inhalt der Font-Datei an\n"
           "  -ll FILE    Zeigt den Inhalt der Font-Datei inklusive Glyphs an\n"
           "  -e FILE     File editieren. Mit dieser Option kann Name, Author, etc.\n"
           "              und die Komprimierung geändert werden, sowie in Kombination\n"
           "              mit -r Fonts gelöscht werden.\n"
           "  -r          Fonts mit der angegebenen Größe und Flags werden gelöscht\n"
           "\n"
           "Enthält die Zieldatei bereits identische FACEs, werden diese überschrieben."
           "\n");
}

int Main::list(const ppl7::String& filename, bool withglyphs)
{
    ppl7::PFPFile ff;
    if (!ff.ident(filename)) {
        printf("ERROR: Unbekanntes Dateiformat\n");
        return 1;
    }
    ppl7::String Id = ff.getID();
    if (Id != "FONT") {
        printf("ERROR: Kein FONT-Format, sondern %s, Version %i.%i\n", (const char*)ff.getID(), ff.getMainVersion(), ff.getSubVersion());
        return 1;
    }
    if (ff.getMainVersion() == 5) {
        /* TODO
        CFont5Generator font5;
        if (!font5.Load(quelle)) {
            ppl6::PrintError();
            return 1;
        }
        font5.List(withglyphs);
        */
        printf("Fint5-Format wird zur Zeit nicht unterstützt.\n");
        return 0;
    } else if (ff.getMainVersion() == 6) {
        CFont6Generator font6;
        if (!font6.LoadFont(quelle)) {
            printf("ERROR: Fehler beim Laden der Font-Datei\n");
            return 1;
        }
        font6.List(withglyphs);
        return 0;
    }
    printf("ERROR: Unbekanntes Font-Format: Version %i.%i", ff.getMainVersion(), ff.getSubVersion());
    return 1;
}

int Main::checkFlags(int argc, char** argv)
{
    flags = 0;
    // Die Flags --aa, --mono1 und --mono8 können nicht gemeinsam verwendet werden
    if (ppl7::HaveArgv(argc, argv, "--aa")) flags++;
    if (ppl7::HaveArgv(argc, argv, "--mono1")) flags++;
    if (ppl7::HaveArgv(argc, argv, "--mono8")) flags++;
    if (flags > 1) {
        printf("Die Flags \"--aa\", \"--mono1\" und \"--mono8\" können nicht gemeinsam verwendet werden\n");
        return 0;
    }

    flags = 0;
    if (ppl7::HaveArgv(argc, argv, "--aa2"))
        flags |= FONTFLAGS::ANTIALIAS | FONTFLAGS::AA2;
    else if (ppl7::HaveArgv(argc, argv, "--aa4"))
        flags |= FONTFLAGS::ANTIALIAS | FONTFLAGS::AA4;
    else if (ppl7::HaveArgv(argc, argv, "--aa"))
        flags |= FONTFLAGS::ANTIALIAS;
    else if (ppl7::HaveArgv(argc, argv, "--mono8"))
        flags |= FONTFLAGS::MONO8;
    else
        flags |= FONTFLAGS::MONO1;
    if (ppl7::HaveArgv(argc, argv, "--isbold")) flags |= FONTFLAGS::ISBOLD;
    if (ppl7::HaveArgv(argc, argv, "--isitalic")) flags |= FONTFLAGS::ISITALIC;
    if (ppl7::HaveArgv(argc, argv, "--genbold")) {
        if ((flags & FONTFLAGS::AA2) == 0 && (flags & FONTFLAGS::AA4) == 0 && (flags & FONTFLAGS::MONO1) == 0) {
            flags |= FONTFLAGS::GENERATEBOLD;
        } else {
            printf("ERROR: --genbold kann zur Zeit nicht zusammen mit --aa2, --aa4 oder --mono\nverwendet werden.\n");
            return 0;
        }
    }
    return 1;
}

int Main::getSizes(int argc, char** argv)
{
    if ((!edit) && (!(ppl7::HaveArgv(argc, argv, "-s")))) {
        printf("Fontsize fehlt\n");
        return 0;
    }
    int start, end;
    ppl7::String Size = ppl7::GetArgv(argc, argv, "-s");
    ppl7::String Tmp;
    std::vector<String> Matches;

    if (ppl7::RegEx::capture("/^([0-9]+)\\-([0-9]+)$", Size, Matches)) { // Range
        start = Matches[1].toInt();
        end = Matches[2].toInt();
        if (end < start) {
            start = Matches[2].toInt();
            end = Matches[1].toInt();
        }
        for (int i = start; i <= end; i++) {
            FontSizesTodo.insert(i);
        }
    } else if (Size.instr(",", 0) >= 0) { // Liste
        ppl7::Array a(Size, ",");
        for (auto it = a.begin(); it != a.end(); ++it) {
            FontSizesTodo.insert(it->toInt());
        }
    } else { // Einzeln
        FontSizesTodo.insert(Size.toInt());
    }
    if (FontSizesTodo.size() == 0) {
        printf("Keine Fontgroessen angegeben\n");
        return 0;
    }
    return 1;
}

int Main::getFiles(int argc, char** argv)
{
    edit = ppl7::GetArgv(argc, argv, "-e");
    quelle = ppl7::GetArgv(argc, argv, "-q");
    target = ppl7::GetArgv(argc, argv, "-t");
    if (ppl7::HaveArgv(argc, argv, "-e")) {
        target = edit;
    } else {
        if (target.isEmpty()) {
            printf("Ziel fehlt\n");
            return 0;
        }
    }
    if (!ppl7::HaveArgv(argc, argv, "-e")) {
        if (quelle.isEmpty()) {
            printf("Quelle fehlt\n");
            return 0;
        }
    }
    if (quelle.notEmpty()) {
        // Läßt sich die Quelle öffnen?
        ppl7::File ff;
        try {
            ff.open(quelle, ppl7::File::READ);
        }
        catch (const ppl7::Exception& e) {
            printf("ERROR: Fehler beim Öffnen der Quelldatei: %s\n", e.what());
            e.print();
            return 0;
        }
    }
    return 1;
}

int Main::getChars(int argc, char** argv)
{
    // Unicode Bereich

    if (ppl7::HaveArgv(argc, argv, "-c")) {
        Array CharsTodo(ppl7::GetArgv(argc, argv, "-c"), ",");
        int start, end;
        for (auto it = CharsTodo.begin(); it != CharsTodo.end(); ++it) {
            ppl7::String Size = *it;
            std::vector<String> Matches;
            if (ppl7::RegEx::capture("/^([0-9]+)\\-([0-9]+)$/", Size, Matches)) { // Range
                start = Matches[1].toInt();
                end = Matches[2].toInt();
                if (end < start) {
                    start = Matches[2].toInt();
                    end = Matches[1].toInt();
                }
                font->AddCharRange(start, end);
            } else if (ppl7::RegEx::capture("/^([0-9]+)$/", Size, Matches)) { // Einzelnes Zeichen
                start = Matches[1].toInt();
                font->AddChar(start);
            } else {
                printf("Unicode-Bereich wurde falsch angegeben\n");
                return 0;
            }
        }
    } else {
        font->AddCharRange(32, 255);
        font->AddChar(8364);   // €
        font->AddChar(0x1E9E); // Grosses ß: ẞ
    }
    if (ppl7::HaveArgv(argc, argv, "--idn")) {
        // Zusätzliche IDN-Zeichen:
        font->AddChar(263); // 263: ć
        font->AddChar(265); // 265: ĉ
        font->AddChar(267); // 267: ċ
        font->AddChar(269); // 269: č
        font->AddChar(271); // 271: ď
        font->AddChar(273); // đ  	273
        font->AddChar(275); // ē  	275
        font->AddChar(277); // ĕ  	277
        font->AddChar(279); // ė  	279
        font->AddChar(281); // ę  	281
        font->AddChar(283); // ě  	283
        font->AddChar(285); // ĝ  	285
        font->AddChar(287); // ğ  	287
        font->AddChar(289); // ġ  	289
        font->AddChar(291); // ģ  	291
        font->AddChar(293); // ĥ  	293
        font->AddChar(297); // ĩ  	297
        font->AddChar(299); // ī  	299
        font->AddChar(301); // ĭ  	301
        font->AddChar(303); // į  	303
        font->AddChar(305); // ı  	305
        font->AddChar(309); // ĵ  	309
        font->AddChar(311); // ķ  	311
        font->AddChar(312); // ĸ  	312
        font->AddChar(314); // ĺ  	314
        font->AddChar(316); // ļ  	316
        font->AddChar(318); // ľ  	318
        font->AddChar(322); // ł  	322
        font->AddChar(324); // ń  	324
        font->AddChar(326); // ņ  	326
        font->AddChar(328); // ň  	328
        font->AddChar(331); // ŋ  	331
        font->AddChar(333); // ō  	333
        font->AddChar(335); // ŏ  	335
        font->AddChar(337); // ő  	337
        font->AddChar(339); // œ  	339
        font->AddChar(341); // ŕ  	341
        font->AddChar(343); // ŗ  	343
        font->AddChar(345); // ř  	345
        font->AddChar(347); // ś  	347
        font->AddChar(349); // ŝ  	349
        font->AddChar(351); // ş  	351
        font->AddChar(353); // š  	353
        font->AddChar(355); // ţ  	355
        font->AddChar(357); // ť  	357
        font->AddChar(359); // ŧ  	359
        font->AddChar(361); // ũ  	361
        font->AddChar(363); // ū  	363
        font->AddChar(365); // ŭ  	365
        font->AddChar(367); // ů  	367
        font->AddChar(369); // ű  	369
        font->AddChar(371); // ų  	371
        font->AddChar(373); // ŵ  	373
        font->AddChar(375); // ŷ  	375
        font->AddChar(378); // ź  	378
        font->AddChar(380); // ż  	380
        font->AddChar(382); // ž  	382
    }
    return 1;
}

int Main::start(int argc, char** argv)
{
    if (argc < 2 || ppl7::HaveArgv(argc, argv, "-h") || ppl7::HaveArgv(argc, argv, "--help")) {
        help();
        return 0;
    }
    if (ppl7::HaveArgv(argc, argv, "-v") || ppl7::HaveArgv(argc, argv, "--version")) {
        version();
        return 0;
    }
    // Fontversion
    if (ppl7::HaveArgv(argc, argv, "-5")) fontVersion = 5;
    if (ppl7::HaveArgv(argc, argv, "-6")) fontVersion = 6;

    // Dateiinhalt Listen?
    if (ppl7::HaveArgv(argc, argv, "-ll") || ppl7::HaveArgv(argc, argv, "-l")) {
        if (ppl7::HaveArgv(argc, argv, "-ll"))
            quelle = ppl7::GetArgv(argc, argv, "-ll");
        else
            quelle = ppl7::GetArgv(argc, argv, "-l");
        return list(quelle, ppl7::HaveArgv(argc, argv, "-ll"));
    }

    if (!checkFlags(argc, argv)) return 1;
    if (!getFiles(argc, argv)) return 1;
    if (!getSizes(argc, argv)) return 1;

    if (fontVersion == 5) {
        // font = new CFont5Generator;
        printf("Font5-Format wird zur Zeit nicht unterstützt.\n");
        return 0;
    } else {
        font = new CFont6Generator;
    }

    if (!getChars(argc, argv)) return 1;

    if (!work(argc, argv)) {
        delete font;
        return 1;
    }
    delete font;
    return 0;
}

int Main::work(int argc, char** argv)
{
    // Das Quellfile wird geladen, wenn es existiert
    if (ppl7::File::exists(target)) {
        try {
            font->load(target);
        }
        catch (const ppl7::Exception& e) {
            printf("ERRROR: Zieldatei existiert, scheint aber keine Font-Datei zu sein\n");
            e.print();
            return 0;
        }
    }
    try {
        if (ppl7::HaveArgv(argc, argv, "--name")) {
            font->setName(ppl7::GetArgv(argc, argv, "--name"));
        }
        if (ppl7::HaveArgv(argc, argv, "--author")) {
            font->setAuthor(ppl7::GetArgv(argc, argv, "--author"));
        }
        if (ppl7::HaveArgv(argc, argv, "--copy")) {
            font->setCopyright(ppl7::GetArgv(argc, argv, "--copy"));
        }
    }
    catch (const ppl7::Exception& e) {
        printf("ERROR: Fehler beim Setzen von Name, Author oder Copyright\n");
        e.print();
        return 0;
    }
    if (ppl7::HaveArgv(argc, argv, "--zlib"))
        font->setCompression(ppl7::Compression::Algo_ZLIB);
    else if (ppl7::HaveArgv(argc, argv, "--bzip2"))
        font->setCompression(ppl7::Compression::Algo_BZIP2);
    else
        font->setCompression(ppl7::Compression::Algo_NONE);

    if (ppl7::HaveArgv(argc, argv, "--disable-hints")) {
        font->enableHints(false);
    } else {
        font->enableHints(true);
    }
    if (quelle.notEmpty()) {
        if (!font->LoadFont(quelle)) {
            printf("ERROR: Fehler beim Laden der Quelldatei\n");
            return 0;
        }
        // Hat unser Font einen Namen?
        ppl7::String name = font->getName();
        if (name.isEmpty() || name.size() < 2) {
            font->CopyFreeTypeName();
        }
    }

    if (edit.notEmpty()) {
        // FACEs löschen?
        if (ppl7::HaveArgv(argc, argv, "-r")) {
            for (auto it = FontSizesTodo.begin(); it != FontSizesTodo.end(); ++it) {
                printf("Lösche Fontgröße %i\n", *it);
                font->DeleteFace(*it, flags);
            }
        }
    } else {
        for (auto it = FontSizesTodo.begin(); it != FontSizesTodo.end(); ++it) {
            if (!font->Generate(*it, flags)) {
                printf("ERROR: Fehler beim Generieren der Fonts\n");
                return 0;
            }
        }
    }
    try {
        font->save(target);
    }
    catch (const ppl7::Exception& e) {
        printf("ERROR: Fehler beim Speichern der Font-Datei\n");
        e.print();
        return 0;
    }
    ppl7::DirEntry stat = ppl7::File::statFile(target);
    printf("Font-Datei \"%s\" erfolgreich gespeichert, Größe: %llu Bystes\n", (const char*)target, stat.Size);
    font->List();
    return 1;
}

/*
UINT GetWindowsDirectory(
  LPTSTR lpBuffer,
  UINT uSize
);


*/

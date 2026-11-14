#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#endif
#include <pplib/core/args.h>
#include <pplib/core/compression.h>
#include <pplib/core/dir.h>
#include <pplib/core/file.h>
#include <pplib/core/regex.h>
#include <pplib/exceptions.h>
#include <pplib/types/array.h>
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
    pplib::String s;
    s.setf("PPL FONTMAKER Version %s (Revision %s) vom %s", FM_VERSION, FM_REVSION, FM_BUILDDATE_STRING);
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
           "  -5          PPL-Font Version 5 generieren\n"
           "  -6          PPL-Font Version 6 generieren (=Default)\n"
           "Flags:\n"
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
           "  -r          Fonts mit der angegebenen Größe, dem Pixelformat und\n"
           "              Stil der gesetzten Flags werden gelöscht\n"
           "\n"
           "Enthält die Zieldatei bereits identische FACEs, werden diese überschrieben."
           "\n");
}

int Main::list(const pplib::String& filename, bool withglyphs)
{
    pplib::PFPFile ff;
    if (!ff.ident(filename)) {
        printf("ERROR: Unbekanntes Dateiformat\n");
        return 1;
    }
    pplib::String Id = ff.getID();
    if (Id != "FONT" && Id != "UNKN") {
        printf("ERROR: Kein FONT-Format, sondern %s, Version %i.%i\n", (const char*)ff.getID(), ff.getMainVersion(), ff.getSubVersion());
        return 1;
    }
    if (ff.getMainVersion() == 5) {
        CFont5Generator font5;
        try {
            font5.load(filename);
        }
        catch (const pplib::Exception& e) {
            printf("ERROR: Fehler beim Laden der Font-Datei\n");
            e.print();
            return 1;
        }
        font5.List(withglyphs);
        return 0;
    } else if (ff.getMainVersion() == 6) {
        CFont6Generator font6;
        try {
            font6.load(filename);
        }
        catch (const pplib::Exception& e) {
            printf("ERROR: Fehler beim Laden der Font-Datei\n");
            e.print();
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
    // Die Format-Flags schliessen sich gegenseitig aus
    int formats = 0;
    if (pplib::HaveArgv(argc, argv, "--aa2")) formats++;
    if (pplib::HaveArgv(argc, argv, "--aa4")) formats++;
    if (pplib::HaveArgv(argc, argv, "--aa8")) formats++;
    if (pplib::HaveArgv(argc, argv, "--mono1")) formats++;
    if (pplib::HaveArgv(argc, argv, "--mono8")) formats++;
    if (formats > 1) {
        printf("Die Flags \"--aa2\", \"--aa4\", \"--aa8\", \"--mono1\" und \"--mono8\" können nicht gemeinsam verwendet werden\n");
        return 0;
    }

    flags = 0;
    if (pplib::HaveArgv(argc, argv, "--aa2"))
        flags |= FONTFLAGS::ANTIALIAS | FONTFLAGS::AA2;
    else if (pplib::HaveArgv(argc, argv, "--aa4"))
        flags |= FONTFLAGS::ANTIALIAS | FONTFLAGS::AA4;
    else if (pplib::HaveArgv(argc, argv, "--aa8"))
        flags |= FONTFLAGS::ANTIALIAS;
    else if (pplib::HaveArgv(argc, argv, "--mono8"))
        flags |= FONTFLAGS::MONO8;
    else
        flags |= FONTFLAGS::MONO1;
    if (pplib::HaveArgv(argc, argv, "--isbold")) flags |= FONTFLAGS::ISBOLD;
    if (pplib::HaveArgv(argc, argv, "--isitalic")) flags |= FONTFLAGS::ISITALIC;
    if (pplib::HaveArgv(argc, argv, "--genbold")) flags |= FONTFLAGS::GENERATEBOLD;
    return 1;
}

int Main::getSizes(int argc, char** argv)
{
    if (edit.isEmpty() && (!pplib::HaveArgv(argc, argv, "-s"))) {
        printf("Fontsize fehlt\n");
        return 0;
    }
    int start, end;
    pplib::String Size = pplib::GetArgv(argc, argv, "-s");

    // "-s" kann eine Einzelgroesse, eine Liste ("12,14") oder Bereiche ("12-14")
    // enthalten, beides auch gemischt ("8,10-13")
    if (Size.notEmpty()) {
        pplib::Array a(Size, ",");
        for (auto it = a.begin(); it != a.end(); ++it) {
            std::vector<String> Matches;
            if (pplib::RegEx::capture("/^([0-9]+)\\-([0-9]+)$/", *it, Matches)) { // Bereich
                start = Matches[1].toInt();
                end = Matches[2].toInt();
                if (end < start) {
                    start = Matches[2].toInt();
                    end = Matches[1].toInt();
                }
                for (int fontsize = start; fontsize <= end; fontsize++) {
                    FontSizesTodo.insert(fontsize);
                }
            } else if (pplib::RegEx::capture("/^[0-9]+$/", *it, Matches)) { // Einzelgroesse
                FontSizesTodo.insert(it->toInt());
            } else {
                printf("Fontgroesse wurde falsch angegeben: %s\n", (const char*) *it);
                return 0;
            }
        }
    }
    if (FontSizesTodo.size() == 0) {
        printf("Keine Fontgroessen angegeben\n");
        return 0;
    }
    return 1;
}

int Main::getFiles(int argc, char** argv)
{
    edit = pplib::GetArgv(argc, argv, "-e");
    quelle = pplib::GetArgv(argc, argv, "-q");
    target = pplib::GetArgv(argc, argv, "-t");
    if (pplib::HaveArgv(argc, argv, "-e")) {
        target = edit;
    } else {
        if (target.isEmpty()) {
            printf("Ziel fehlt\n");
            return 0;
        }
    }
    if (!pplib::HaveArgv(argc, argv, "-e")) {
        if (quelle.isEmpty()) {
            printf("Quelle fehlt\n");
            return 0;
        }
    }
    if (quelle.notEmpty()) {
        // Läßt sich die Quelle öffnen?
        pplib::File ff;
        try {
            ff.open(quelle, pplib::File::FileMode::READ);
        }
        catch (const pplib::Exception& e) {
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

    if (pplib::HaveArgv(argc, argv, "-c")) {
        Array CharsTodo(pplib::GetArgv(argc, argv, "-c"), ",");
        int start, end;
        for (auto it = CharsTodo.begin(); it != CharsTodo.end(); ++it) {
            pplib::String Size = *it;
            std::vector<String> Matches;
            if (pplib::RegEx::capture("/^([0-9]+)\\-([0-9]+)$/", Size, Matches)) { // Range
                start = Matches[1].toInt();
                end = Matches[2].toInt();
                if (end < start) {
                    start = Matches[2].toInt();
                    end = Matches[1].toInt();
                }
                if (end > FM_MAX_UNICODE) {
                    printf("ERROR: Zeichencodes über U+FFFF (%i) unterstützt das Fontformat nicht\n", (int) FM_MAX_UNICODE);
                    return 0;
                }
                font->AddCharRange(start, end);
            } else if (pplib::RegEx::capture("/^([0-9]+)$/", Size, Matches)) { // Einzelnes Zeichen
                start = Matches[1].toInt();
                if (start > FM_MAX_UNICODE) {
                    printf("ERROR: Zeichencodes über U+FFFF (%i) unterstützt das Fontformat nicht\n", (int) FM_MAX_UNICODE);
                    return 0;
                }
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
    if (pplib::HaveArgv(argc, argv, "--idn")) {
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
    if (argc < 2 || pplib::HaveArgv(argc, argv, "-h") || pplib::HaveArgv(argc, argv, "--help")) {
        help();
        return 0;
    }
    if (pplib::HaveArgv(argc, argv, "-v") || pplib::HaveArgv(argc, argv, "--version")) {
        version();
        return 0;
    }
    // Fontversion
    if (pplib::HaveArgv(argc, argv, "-5")) fontVersion = 5;
    if (pplib::HaveArgv(argc, argv, "-6")) fontVersion = 6;

    // Dateiinhalt Listen?
    if (pplib::HaveArgv(argc, argv, "-ll") || pplib::HaveArgv(argc, argv, "-l")) {
        if (pplib::HaveArgv(argc, argv, "-ll"))
            quelle = pplib::GetArgv(argc, argv, "-ll");
        else
            quelle = pplib::GetArgv(argc, argv, "-l");
        return list(quelle, pplib::HaveArgv(argc, argv, "-ll"));
    }

    if (!checkFlags(argc, argv)) return 1;
    if (!getFiles(argc, argv)) return 1;
    if (!getSizes(argc, argv)) return 1;

    if (fontVersion == 5) {
        font = new CFont5Generator;
    } else {
        font = new CFont6Generator;
    }

    if (!getChars(argc, argv)) {
        delete font;
        return 1;
    }

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
    if (pplib::File::exists(target)) {
        // Handelt es sich um das erwartete Font-Format?
        if (!font->ident(target) || font->getMainVersion() != fontVersion) {
            printf("ERROR: Zieldatei existiert, ist aber keine Font%d-Datei\n", fontVersion);
            return 0;
        }
        // Aeltere Dateien dieses Tools haben die Datei-ID "UNKN" statt "FONT".
        if (font->getID() != "FONT" && font->getID() != "UNKN") {
            printf("ERROR: Zieldatei hat die unbekannte Datei-ID \"%s\"\n", (const char*)font->getID());
            return 0;
        }
        try {
            font->load(target);
        }
        catch (const pplib::Exception& e) {
            printf("ERRROR: Zieldatei existiert, scheint aber keine Font-Datei zu sein\n");
            e.print();
            return 0;
        }
    }
    try {
        if (pplib::HaveArgv(argc, argv, "--name")) {
            font->setName(pplib::GetArgv(argc, argv, "--name"));
        }
        if (pplib::HaveArgv(argc, argv, "--author")) {
            font->setAuthor(pplib::GetArgv(argc, argv, "--author"));
        }
        if (pplib::HaveArgv(argc, argv, "--copy")) {
            font->setCopyright(pplib::GetArgv(argc, argv, "--copy"));
        }
    }
    catch (const pplib::Exception& e) {
        printf("ERROR: Fehler beim Setzen von Name, Author oder Copyright\n");
        e.print();
        return 0;
    }
    if (pplib::HaveArgv(argc, argv, "--zlib"))
        font->setCompression(pplib::Compression::Algorithm::ZLIB);
    else if (pplib::HaveArgv(argc, argv, "--bzip2"))
        font->setCompression(pplib::Compression::Algorithm::BZIP2);
    else
        font->setCompression(pplib::Compression::Algorithm::NONE);

    if (pplib::HaveArgv(argc, argv, "--disable-hints")) {
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
        pplib::String name = font->getName();
        if (name.isEmpty() || name.size() < 2) {
            font->CopyFreeTypeName();
        }
    }

    if (edit.notEmpty()) {
        // FACEs löschen?
        if (pplib::HaveArgv(argc, argv, "-r")) {
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
    catch (const pplib::Exception& e) {
        printf("ERROR: Fehler beim Speichern der Font-Datei\n");
        e.print();
        return 0;
    }
    pplib::DirEntry stat = pplib::File::statFile(target);
    printf("Font-Datei \"%s\" erfolgreich gespeichert, Größe: %llu Bytes\n", (const char*)target, (unsigned long long)stat.Size);
    font->List();
    return 1;
}

/*
UINT GetWindowsDirectory(
  LPTSTR lpBuffer,
  UINT uSize
);


*/

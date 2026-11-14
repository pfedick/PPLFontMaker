// PPLFontMaker
// Copyright by Patrick Fedick, in 2026
//
// Unittests: Pruefen der von diesem Tool erzeugeten Font-Dateien

#include <string>
#include <vector>

namespace fmtest {

// Ein Chunk einer (unkomprimierten) PFP-Datei
struct PfpChunk {
    std::string name;
    std::vector<unsigned char> data;
};

// Ein Face (FONT6) bzw. FACE-Chunk (FONT5) mit den wichtigsten Headerwerten
struct FaceInfo {
    int style = 0;       // Stil-Flagbyte ohne Hints-Bit
    int hints = 0;       // 1, wenn das Hints-Bit gesetzt ist (nur Font6)
    int pixelformat = 0; // 1 Mono8, 2 Mono1, 3 AA8, 4 AA2, 5 AA4
    int size = 0;        // Groesse des Face in Pixeln
    int maxbearingy = 0;
    int maxheight = 0;
    int underline = 0;
    int jumptables = 0; // Font5: Anzahl Sprungtabellen, Font6: Flagbyte 15
    int glyphcount = 0;  // Font6: Anzahl Glyphs laut Header
};

// Vollstaendiger Pfad zu einer Datei in tests/testdata
std::string testDataPath(const std::string& name);

// Vollstaendiger Pfad zu einer Datei in tests/tmp
std::string tmpFilePath(const std::string& name);

// Loescht eine Datei, falls es sie gibt
void deleteFile(const std::string& path);

// Liest eine komplette Datei. Wirft std::runtime_error, wenn sie nicht
// lesbar ist.
std::vector<unsigned char> readFile(const std::string& path);

// Ruft das fertige Programm mit args auf (ohne Programmnamen) und liefert
// den Exit-Code. output erhaelt, wenn nicht nullptr, die Ausgabe.
int runTool(const std::vector<std::string>& args, std::string* output = nullptr);

// 16-Bit- bzw. 32-Bit-Wert little-endian aus data an Position pos
unsigned int peek16(const std::vector<unsigned char>& data, size_t pos);
unsigned int peek32(const std::vector<unsigned char>& data, size_t pos);

// Der Rahmen einer (unkomprimierten) PFP-Datei Version 3
struct PfpHeader {
    std::string fileID;  // "FONT"
    int formatVersion = 0;
    int majorVersion = 0;  // 5 oder 6
    int minorVersion = 0;
    int compression = 0;
};

// Zerlegt den Rahmen einer unkomprimierten PFP-Datei (Version 3) in ihre
// Chunks. Bei einem fehlerhaften Rahmen kommt false zurueck, reason
// beschreibt den Fehler.
bool parsePfpFile(const std::vector<unsigned char>& raw, PfpHeader& header, std::vector<PfpChunk>& chunks, std::string& reason);

// Sucht einen Chunk anhand des Namens (erster Treffer), sonst nullptr
const PfpChunk* findChunk(const std::vector<PfpChunk>& chunks, const std::string& name);

// Liest die Header aller FACE-Chunks und prueft sie. majorVersion ist 5 oder
// 6. Bei Font6 werden zusaetzlich die Glyph-Blöcke durchlaufen. Gefundene
// Probleme werden als Zeilen in problems gesammelt. Rueckgabe ist die Anzahl
// der gelesenen Faces.
int checkFaces(const std::vector<PfpChunk>& chunks, int majorVersion, std::vector<FaceInfo>& faces, std::vector<std::string>& problems);

} // namespace fmtest

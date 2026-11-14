// PPLFontMaker
// Copyright by Patrick Fedick, in 2026
//
// Helfer fuer die Unittests

#include "testhelper.h"

#include <cstdio>
#include <cstring>
#include <set>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <sys/wait.h>

#include <filesystem>

namespace fmtest {

std::string testDataPath(const std::string& name)
{
    return std::string(FM_TESTDATA_DIR) + "/" + name;
}

std::string tmpFilePath(const std::string& name)
{
    return std::string(FM_TMP_DIR) + "/" + name;
}

void deleteFile(const std::string& path)
{
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

std::vector<unsigned char> readFile(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Datei nicht lesbar: " + path);
    }
    return std::vector<unsigned char>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

int runTool(const std::vector<std::string>& args, std::string* output)
{
    std::string command = std::string("\"") + FM_TOOL + "\"";
    for (const std::string& arg : args) {
        command += " \"" + arg + "\"";
    }
    command += " 2>&1";

    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr) {
        throw std::runtime_error("Programm kann nicht gestartet werden: " + std::string(FM_TOOL));
    }

    std::string text;
    char buffer[4096];
    size_t count;
    while ((count = fread(buffer, 1, sizeof(buffer), pipe)) > 0) {
        if (output != nullptr) {
            text.append(buffer, count);
        }
    }
    int status = pclose(pipe);
    if (output != nullptr) {
        *output = text;
    }
#ifdef _WIN32
    return status;
#else
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
#endif
}

unsigned int peek16(const std::vector<unsigned char>& data, size_t pos)
{
    if (pos + 2 > data.size()) {
        throw std::runtime_error("Leseposition ausserhalb des Chunks");
    }
    return data[pos] | (data[pos + 1] << 8);
}

unsigned int peek32(const std::vector<unsigned char>& data, size_t pos)
{
    if (pos + 4 > data.size()) {
        throw std::runtime_error("Leseposition ausserhalb des Chunks");
    }
    return data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | ((unsigned int) data[pos + 3] << 24);
}

bool parsePfpFile(const std::vector<unsigned char>& raw, PfpHeader& header, std::vector<PfpChunk>& chunks, std::string& reason)
{
    chunks.clear();
    reason.clear();
    header = PfpHeader();

    if (raw.size() < 24) {
        reason = "Datei zu kurz fuer einen PFP-Header";
        return false;
    }
    if (memcmp(raw.data(), "PFP-File", 8) != 0) {
        reason = "Kein PFP-File-Header";
        return false;
    }
    if (raw[8] != 3) {
        reason = "Unerwartete PFP-Formatversion";
        return false;
    }
    if (raw[9] != 24) {
        reason = "Unerwartete Headerlaenge";
        return false;
    }
    if (raw[16] != 0 || raw[17] != 0) {
        reason = "Test erwartet eine unkomprimierte Datei";
        return false;
    }

    header.formatVersion = raw[8];
    header.fileID.assign((const char*) &raw[10], 4);
    header.minorVersion = raw[14];
    header.majorVersion = raw[15];
    header.compression = raw[16];

    size_t pos = 24;
    bool endfound = false;
    while (pos + 8 <= raw.size()) {
        PfpChunk chunk;
        chunk.name.assign((const char*) &raw[pos], 4);
        unsigned int size = peek32(raw, pos + 4);
        if (chunk.name == "ENDF") {
            if (size != 0) {
                reason = "Der ENDF-Chunk muss die Groesse 0 haben";
                return false;
            }
            endfound = true;
            break;
        }
        if (size < 8 || pos + size > raw.size()) {
            reason = "Chunk " + chunk.name + " hat eine ungueltige Groesse";
            return false;
        }
        chunk.data.assign(raw.begin() + pos + 8, raw.begin() + pos + size);
        chunks.push_back(chunk);
        pos += size;
    }

    if (!endfound) {
        reason = "Der ENDF-Chunk fehlt";
        return false;
    }
    return true;
}


namespace {

// Anzahl Bits pro Pixel laut Pixelformat des FACE-Headers
int bitsPerPixel(int pixelformat)
{
    switch (pixelformat) {
        case 1:  // Monochrom, 8 Bit
        case 3:  // Antialiasing, 8 Bit
            return 8;
        case 2:  // Monochrom, 1 Bit
            return 1;
        case 4:  // Antialiasing, 2 Bit
            return 2;
        case 5:  // Antialiasing, 4 Bit
            return 4;
    }
    return 0;
}

size_t bitmapBytes(int width, int height, int pixelformat)
{
    int bits = bitsPerPixel(pixelformat);
    if (bits == 0 || width <= 0 || height <= 0) {
        return 0;
    }
    return ((size_t) width * (size_t) height * (size_t) bits + 7) / 8;
}

std::string faceTag(const FaceInfo& info)
{
    char buffer[80];
    snprintf(buffer, sizeof(buffer), "Face %ipx/Format %i/Stil %i", info.size, info.pixelformat, info.style);
    return std::string(buffer);
}

// Laeuft die Glyph-Blöcke eines Font6-Face durch (font6.md)
void checkFace6(const std::vector<unsigned char>& data, FaceInfo& info, std::vector<std::string>& problems)
{
    std::string tag = faceTag(info);
    size_t pos = 12;
    int previous = -1;
    int found = 0;

    while (pos < data.size()) {
        if (pos + 16 > data.size()) {
            problems.push_back(tag + ": Glyph-Header am Dateiende ist abgeschnitten");
            return;
        }
        unsigned int blocksize = peek32(data, pos);
        if (blocksize < 16 || pos + blocksize > data.size()) {
            problems.push_back(tag + ": Glyph-Block an Position " + std::to_string(pos) + " hat eine ungueltige Groesse");
            return;
        }
        int code = (int) peek16(data, pos + 4);
        int width = (int) peek16(data, pos + 6);
        int height = (int) peek16(data, pos + 8);

        size_t hintbytes = 0;
        if (info.hints) {
            size_t p = pos + 16;
            while (p + 4 <= pos + blocksize) {
                unsigned int hintcode = peek16(data, p);
                hintbytes += 4;
                p += 4;
                if (hintcode == 0) {
                    break;
                }
            }
        }

        size_t needed = bitmapBytes(width, height, info.pixelformat);
        size_t available = blocksize - 16 - hintbytes;
        if (width > 0 && height > 0 && (available < needed || available > needed + 1)) {
            problems.push_back(tag + ": Bitmap von Zeichen " + std::to_string(code) + " ist " + std::to_string(available) + " Byte, erwartet " + std::to_string(needed));
        }
        if (code <= previous) {
            problems.push_back(tag + ": Zeichen " + std::to_string(code) + " steht nicht in aufsteigender Reihenfolge");
        }
        previous = code;
        found++;
        pos += blocksize;
    }

    if (found == 0) {
        problems.push_back(tag + ": Face enthaelt keine Glyphen");
    }
    if (found != info.glyphcount) {
        problems.push_back(tag + ": Header nennt " + std::to_string(info.glyphcount) + " Glyphen, gefunden " + std::to_string(found));
    }
}

// Prueft Sprungtabellen, Zeigerfeld und Glyph-Blöcke eines Font5-Face
void checkFace5(const std::vector<unsigned char>& data, FaceInfo& info, std::vector<std::string>& problems)
{
    std::string tag = faceTag(info);
    size_t numTables = (size_t) info.jumptables;
    if (numTables == 0) {
        problems.push_back(tag + ": Face enthaelt keine Sprungtabelle");
        return;
    }
    size_t tableStart = 12;
    if (tableStart + numTables * 8 > data.size()) {
        problems.push_back(tag + ": Sprungtabelle ragt ueber den Face-Anfang hinaus");
        return;
    }

    size_t codes = 0;
    size_t expectedTable = tableStart + numTables * 8;
    for (size_t i = 0; i < numTables; i++) {
        size_t entry = tableStart + i * 8;
        int start = (int) peek16(data, entry);
        int end = (int) peek16(data, entry + 2);
        unsigned int offset = peek32(data, entry + 4);
        if (end < start) {
            problems.push_back(tag + ": Sprungtabelle " + std::to_string(i) + " endet vor ihrem Anfang");
            return;
        }
        if (offset != expectedTable) {
            problems.push_back(tag + ": Sprungtabelle " + std::to_string(i) + " zeigt auf " + std::to_string(offset) + ", erwartet " + std::to_string(expectedTable));
            return;
        }
        expectedTable += (size_t)(end - start + 1) * 4;
        codes += (size_t)(end - start + 1);
    }

    size_t glyphsStart = tableStart + numTables * 8 + codes * 4;
    if (glyphsStart > data.size()) {
        problems.push_back(tag + ": Zeigerfeld ragt ueber den Chunk hinaus");
        return;
    }

    // Die Zeigerfeld-Eingaenge nennen den Anfang jedes Glyphs. Die Laenge eines
    // Glyphs steht nicht im Format, sie ergibt sich aus dem Abstand zum
    // naechsten Glyph bzw. zum Chunk-Ende.
    std::vector<size_t> anfange;
    for (size_t i = 0; i < codes; i++) {
        unsigned int pointer = peek32(data, tableStart + numTables * 8 + i * 4);
        if (pointer != 0) {
            anfange.push_back((size_t) pointer);
        }
    }

    if (anfange.empty()) {
        problems.push_back(tag + ": Face enthaelt keine Glyphen");
        return;
    }
    if (anfange.front() != glyphsStart) {
        problems.push_back(tag + ": erstes Glyph beginnt auf " + std::to_string(anfange.front()) + ", erwartet " + std::to_string(glyphsStart));
    }

    for (size_t i = 0; i < anfange.size(); i++) {
        size_t start = anfange[i];
        size_t ende = (i + 1 < anfange.size()) ? anfange[i + 1] : data.size();
        if (start + 10 > ende) {
            problems.push_back(tag + ": Glyph an Position " + std::to_string(start) + " ist juenger als sein 10 Byte grosser Header");
            return;
        }
        if (i > 0 && start <= anfange[i - 1]) {
            problems.push_back(tag + ": Glyph-Zeiger an Position " + std::to_string(start) + " ist nicht aufsteigend");
            return;
        }
        int width = (int) peek16(data, start);
        int height = (int) peek16(data, start + 2);
        size_t needed = bitmapBytes(width, height, info.pixelformat);
        // Das Programm polstert die Bitmap um ein Byte auf, wenn die Bitanzahl
        // glatt in ein Byte aufgeht (font5.md, Hinweise zur Implementierung).
        size_t available = ende - start - 10;
        if (width > 0 && height > 0 && (available < needed || available > needed + 1)) {
            problems.push_back(tag + ": Bitmap an Position " + std::to_string(start) + " ist " + std::to_string(available) + " Byte, erwartet " + std::to_string(needed));
        }
    }
}

}  // namespace

int checkFaces(const std::vector<PfpChunk>& chunks, int majorVersion, std::vector<FaceInfo>& faces, std::vector<std::string>& problems)
{
    faces.clear();
    std::set<std::string> seen;

    for (const PfpChunk& chunk : chunks) {
        if (chunk.name != "FACE") {
            continue;
        }
        const std::vector<unsigned char>& data = chunk.data;
        if (data.size() < 12) {
            problems.push_back("FACE-Chunk ist juenger als der 12 Byte grosse Header");
            continue;
        }

        FaceInfo info;
        int flags = data[0];
        info.style = flags & 7;
        info.hints = (majorVersion == 6 && (flags & 8) != 0) ? 1 : 0;
        info.pixelformat = data[1];
        info.size = (int) peek16(data, 2);
        info.maxbearingy = (int) peek16(data, 4);
        info.maxheight = (signed short) peek16(data, 6);
        info.underline = (signed short) peek16(data, 8);
        info.jumptables = (majorVersion == 5) ? (int) peek16(data, 10) : 0;
        info.glyphcount = (majorVersion == 6) ? (int) peek16(data, 10) : -1;

        std::string tag = faceTag(info);
        if (bitsPerPixel(info.pixelformat) == 0) {
            problems.push_back(tag + ": unbekanntes Pixelformat");
        }
        if (info.size <= 0 || info.size > 500) {
            problems.push_back(tag + ": unsinnige Groesse");
        }
        if (info.maxheight <= 0 || info.maxheight > 6 * info.size) {
            problems.push_back(tag + ": MaxHeight " + std::to_string(info.maxheight) + " passt nicht zur Groesse");
        }
        if (info.maxbearingy < 0 || info.maxbearingy > 4 * info.size) {
            problems.push_back(tag + ": MaxBearingY " + std::to_string(info.maxbearingy) + " passt nicht zur Groesse");
        }

        if (majorVersion == 6) {
            checkFace6(data, info, problems);
        } else {
            checkFace5(data, info, problems);
        }

        char key[48];
        snprintf(key, sizeof(key), "%i/%i/%i", info.size, info.pixelformat, info.style);
        if (!seen.insert(std::string(key)).second) {
            problems.push_back(std::string("Mehrere Faces mit derselben Kombination Groesse/Pixelformat/Stil: ") + key);
        }
        faces.push_back(info);
    }

    return (int) faces.size();
}

const PfpChunk* findChunk(const std::vector<PfpChunk>& chunks, const std::string& name)
{
    for (const PfpChunk& chunk : chunks) {
        if (chunk.name == name) {
            return &chunk;
        }
    }
    return nullptr;
}

} // namespace fmtest

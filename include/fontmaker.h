#ifndef FONTMAKER_H
#define FONTMAKER_H
#include <ft2build.h>
#include FT_FREETYPE_H

#include <pplib/core/functions.h>
#include <pplib/core/pfpfile.h>
#include <pplib/exceptions.h>
#include <pplib/grafix/color.h>
#include <pplib/grafix/drawable.h>
#include <pplib/types/string.h>
#include <pplib/types/widestring.h>
#include "version.h"
#include <map>
#include <set>
#include <utility>
#include <vector>

using namespace pplib;

#define FM_AUTHOR "Patrick Fedick"
#define FM_COPYRIGHT "(c) Copyright by Patrick Fedick in 2026"

namespace FONTFLAGS
{
enum
{
    ANTIALIAS = 0x00000001,
    ISBOLD = 0x00000002,
    ISITALIC = 0x00000004,
    GENERATEBOLD = 0x00000008,
    GENERATESHADOW = 0x00000010,
    GENERATEOUTLINE = 0x00000020,
    MONO1 = 0x00000040,
    MONO8 = 0x00000080,
    AA2 = 0x00000100,
    AA4 = 0x00000200
};
} // namespace FONTFLAGS

// Das Font5- und Font6-Format kennt nur Zeichencodes bis U+FFFF
constexpr wchar_t FM_MAX_UNICODE = 0xFFFF;

// Umrechnung der Commandline-Flags in die Werte, die im FACE-Header stehen.

// Stil-Flagbyte: Bit 0 Antialiasing, Bit 1 fett, Bit 2 kursiv. Das Hints-Bit
// (Bit 3), das Font6 beim Schreiben setzt, wird hier nicht beruecksichtigt.
inline int FaceStyleFlag(int flags)
{
    int f = 0;
    if (flags & FONTFLAGS::ANTIALIAS) f |= 1;
    if (flags & FONTFLAGS::ISBOLD) f |= 2;
    if (flags & FONTFLAGS::GENERATEBOLD) f |= 2;
    if (flags & FONTFLAGS::ISITALIC) f |= 4;
    return f;
}

// Pixelformat laut FACE-Header: 1 Mono8, 2 Mono1, 3 AA8, 4 AA2, 5 AA4
inline int FacePixelFormat(int flags)
{
    if (flags & FONTFLAGS::AA2) return 4;
    if (flags & FONTFLAGS::AA4) return 5;
    if (flags & FONTFLAGS::ANTIALIAS) return 3;
    if (flags & FONTFLAGS::MONO1) return 2;
    return 1;
}

class CFontGeneratorCommon;

class Main
{
private:
    pplib::String edit;
    pplib::String quelle;
    pplib::String target;
    int flags;
    std::set<int> FontSizesTodo;
    int fontVersion;

    CFontGeneratorCommon* font;

    int list(const pplib::String& filename, bool withglyphs = false);
    int checkFlags(int argc, char** argv);
    int getSizes(int argc, char** argv);
    int getFiles(int argc, char** argv);
    int work(int argc, char** argv);
    int getChars(int argc, char** argv);

public:
    Main();
    ~Main();
    void help();
    void version();
    int start(int argc, char** argv);
};

typedef struct
{
    // RECT	outline;
    int flags;
    int width, height;
    int bearingy;
    int bearingx;
    int advance;
    char* buffer;
    int buffersize;
} FONTRENDER;

class CFont5Generator;

class Font5Glyph
{
    friend class CFont5Generator;

private:
    wchar_t unicode;
    char header[10];
    void* bitmap;
    int size;

public:
    Font5Glyph();
    Font5Glyph(const Font5Glyph& other);
    ~Font5Glyph();
    Font5Glyph& operator=(const Font5Glyph& other);
};

class CFontGeneratorCommon : public pplib::PFPFile
{
    friend class CFont5Generator;

protected:
    FT_Library ftlib;
    FT_Face face;
    bool bfaceloaded;

public:
    CFontGeneratorCommon();
    virtual ~CFontGeneratorCommon();

    int LoadFont(const char* file);

    int Render(int code, FONTRENDER* render);
    int BltGlyph(pplib::grafix::Drawable& surface, int x, int y, FONTRENDER* glyph, pplib::grafix::Color& c);

    void DeleteFace(int size, int flags);
    void CopyFreeTypeName();

    virtual int AddCharRange(wchar_t start, wchar_t end) = 0;
    virtual int AddChar(wchar_t unicode) = 0;
    virtual int Generate(int size, int flags) = 0;
    virtual void List() = 0;
    virtual void List(bool withGlyphs) = 0;
    virtual void ListGlyphs(PFPChunk* c) = 0;
    virtual void enableHints(bool enable) = 0;
};

class CFont5Generator : public CFontGeneratorCommon
{
private:
    int debugx, debugy;
    std::vector<std::pair<wchar_t, wchar_t>> CharRanges;
    std::map<wchar_t, Font5Glyph> Glyphs;

    int AddGlyph(wchar_t code, FONTRENDER* render);

public:
    CFont5Generator();
    virtual ~CFont5Generator();
    virtual int AddCharRange(wchar_t start, wchar_t end);
    virtual int AddChar(wchar_t unicode);
    virtual int Generate(int size, int flags);

    virtual void List();
    virtual void List(bool withGlyphs);
    virtual void ListGlyphs(PFPChunk* c);
    void enableHints(bool enable) override;
};

class Font6Glyph
{
public:
    wchar_t unicode;
    char* header;
    size_t headersize;
    char* bitmap;
    size_t bitmapsize;

    Font6Glyph();
    Font6Glyph(const Font6Glyph& other);
    ~Font6Glyph();

    Font6Glyph& operator=(const Font6Glyph& other);
};

class CFont6Generator : public CFontGeneratorCommon
{
private:
    std::set<wchar_t> CharList;
    std::map<wchar_t, Font6Glyph> Glyphs;
    size_t totalGlyphSize;
    size_t totalHintsSize;
    bool hintsEnabled;

    int AddGlyph(wchar_t code, FONTRENDER* render);

public:
    CFont6Generator();
    virtual ~CFont6Generator();

    virtual int AddCharRange(wchar_t start, wchar_t end);
    virtual int AddChar(wchar_t unicode);
    virtual int Generate(int size, int flags);
    virtual void List();
    virtual void List(bool withGlyphs);
    virtual void ListGlyphs(PFPChunk* c);
    void enableHints(bool enable) override;
};

#endif

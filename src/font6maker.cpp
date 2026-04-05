#include <stdlib.h>
#include "fontmaker.h"

Font6Glyph::Font6Glyph()
{
    unicode = 0;
    header = NULL;
    headersize = 0;
    bitmap = NULL;
    bitmapsize = 0;
}

Font6Glyph::~Font6Glyph()
{
    free(header);
    free(bitmap);
}

Font6Glyph::Font6Glyph(const Font6Glyph& other)
{
    unicode = other.unicode;
    headersize = other.headersize;
    bitmapsize = other.bitmapsize;
    header = NULL;
    bitmap = NULL;
    if (other.headersize > 0) {
        header = (char*)malloc(other.headersize);
        if (!header) throw ppl7::OutOfMemoryException();
        memcpy(header, other.header, other.headersize);
    }
    if (other.bitmapsize > 0) {
        bitmap = (char*)malloc(other.bitmapsize);
        if (!bitmap) throw ppl7::OutOfMemoryException();
        memcpy(bitmap, other.bitmap, other.bitmapsize);
    }
}

Font6Glyph& Font6Glyph::operator=(const Font6Glyph& other)
{
    delete (header);
    delete (bitmap);
    unicode = other.unicode;
    headersize = other.headersize;
    bitmapsize = other.bitmapsize;
    header = NULL;
    bitmap = NULL;
    if (other.headersize > 0) {
        header = (char*)malloc(other.headersize);
        if (!header) throw ppl7::OutOfMemoryException();
        memcpy(header, other.header, other.headersize);
    }
    if (other.bitmapsize > 0) {
        bitmap = (char*)malloc(other.bitmapsize);
        if (!bitmap) throw ppl7::OutOfMemoryException();
        memcpy(bitmap, other.bitmap, other.bitmapsize);
    }
    return *this;
}

CFont6Generator::CFont6Generator()
{
    setVersion(6, 0);
}

CFont6Generator::~CFont6Generator()
{
}

int CFont6Generator::AddCharRange(wchar_t start, wchar_t end)
{
    for (wchar_t i = start; i < end; i++) {
        CharList.insert(i);
    }
    return 1;
}

int CFont6Generator::AddChar(wchar_t unicode)
{
    CharList.insert(unicode);
    return 1;
}

int CFont6Generator::AddGlyph(wchar_t code, FONTRENDER* render)
{
    size_t headersize = 16;
    FT_UInt glyph_index = FT_Get_Char_Index(face, code);

    std::map<wchar_t, int> Hints;

    // Wieviele Hints gibt es ?
    size_t numhints = 0;
    if (FT_HAS_KERNING(face)) {
        FT_Vector kerning;
        std::set<wchar_t>::const_iterator it;
        for (it = CharList.begin(); it != CharList.end(); it++) {
            FT_UInt next_glyph = FT_Get_Char_Index(face, *it);
            FT_Get_Kerning(face, glyph_index, next_glyph, FT_KERNING_DEFAULT, &kerning);
            if (kerning.x != 0) {
                // printf ("Kerning %i => %i: %i\n",code,*it,kerning.x/64);
                numhints++;
                Hints.insert(std::pair<wchar_t, int>(*it, kerning.x / 64));
            }
            // x+=(kerning.x>>6);
        }
        headersize += numhints * 4;
        headersize += 4;
        totalHintsSize += numhints * 4 + 4;
    }
    Font6Glyph g;
    g.header = (char*)malloc(headersize);
    if (!g.header) throw ppl7::OutOfMemoryException();
    g.headersize = headersize;
    g.unicode = code;
    Poke32(g.header + 0, headersize + render->buffersize);
    Poke16(g.header + 4, code);
    Poke16(g.header + 6, render->width);
    Poke16(g.header + 8, render->height);
    Poke16(g.header + 10, render->bearingx);
    Poke16(g.header + 12, render->bearingy);
    Poke16(g.header + 14, render->advance);
    if (FT_HAS_KERNING(face)) {
        numhints = 0;
        std::map<wchar_t, int>::const_iterator it;
        for (it = Hints.begin(); it != Hints.end(); it++) {
            Poke16(g.header + 16 + numhints * 4, it->first);
            Poke16(g.header + 18 + numhints * 4, it->second);
            numhints++;
        }
        Poke16(g.header + 16 + numhints * 4, 0);
        Poke16(g.header + 18 + numhints * 4, 0);
    }
    g.bitmap = render->buffer;
    g.bitmapsize = render->buffersize;

    // printf ("addglyph: %lc = %i, Hints: %zi, Headersize: %zi, Bitmapsize:
    // %zi\n",code,code,numhints,headersize,g.bitmapsize);
    Glyphs.insert(std::pair<wchar_t, Font6Glyph>(code, g));
    totalGlyphSize += g.headersize + g.bitmapsize;
    return 1;
}

int CFont6Generator::Generate(int fontsize, int flags)
{
    if (!bfaceloaded) return 0;
    // Fontgroesse einstellen
    // FT_Set_Char_Size(face,0,fontsize*64,72,72);
    FT_Set_Pixel_Sizes(face, 0, fontsize + 2);
    FONTRENDER glyph;
    DeleteFace(fontsize, flags & 7);
    glyph.buffer = NULL;
    glyph.flags = flags;
    Glyphs.clear();

    totalGlyphSize = 0;
    totalHintsSize = 0;
    int maxheight = 0;
    int maxunderlength = -9999999;
    int maxbearingy = 0;
    int numGlyphs = 0;

    std::set<wchar_t>::const_iterator it;

    for (it = CharList.begin(); it != CharList.end(); it++) {
        numGlyphs++;
        if (Render(*it, &glyph)) {
            if (glyph.bearingy > maxbearingy) maxbearingy = glyph.bearingy;
            if (0 - glyph.bearingy + glyph.height > maxunderlength) maxunderlength = 0 - glyph.bearingy + glyph.height;
            if (!AddGlyph(*it, &glyph)) {
                return 0;
            }
        }
    }
    maxheight = maxunderlength + maxbearingy + 1;
    // Speicherbedarf berechnen:
    size_t bytes = 12;       // Header des FACE
    bytes += totalGlyphSize; // Größe der Glyphs
    printf("Fontsize: %i, %zi Glyphs gerendert, Gesamtgröße: %zi Bytes, davon %zi Bytes für Hints.\n", fontsize, Glyphs.size(),
           totalGlyphSize, totalHintsSize);

    char* buffer = (char*)malloc(bytes);
    if (!buffer) {
        printf("ERROR: Fehler beim Reservieren von %zi Bytes Speicher\n", bytes);
        return 0;
    }
    memset(buffer, 0, bytes);
    // Zuerst schreiben wir den Header
    int f = 0;
    if (flags & FONTFLAGS::ANTIALIAS) f |= 1;
    if (flags & FONTFLAGS::ISBOLD) f |= 2;
    if (flags & FONTFLAGS::GENERATEBOLD) f |= 2;
    if (flags & FONTFLAGS::ISITALIC) f |= 4;
    if (FT_HAS_KERNING(face)) {
        f |= 8;
    }
    Poke8(buffer + 0, f);
    if (flags & FONTFLAGS::AA2)
        Poke8(buffer + 1, 4);
    else if (flags & FONTFLAGS::AA4)
        Poke8(buffer + 1, 5);
    else if (flags & FONTFLAGS::ANTIALIAS)
        Poke8(buffer + 1, 3);
    else if (flags & FONTFLAGS::MONO1)
        Poke8(buffer + 1, 2);
    else
        Poke8(buffer + 1, 1);
    Poke16(buffer + 2, fontsize);
    Poke16(buffer + 4, maxbearingy);
    Poke16(buffer + 6, maxheight);
    Poke16(buffer + 8, (0 - face->underline_position) >> 6);
    Poke16(buffer + 10, Glyphs.size());

    // Nun kopieren wir die Glyphs hinzu
    size_t p = 12;
    std::map<wchar_t, Font6Glyph>::const_iterator g;
    for (g = Glyphs.begin(); g != Glyphs.end(); g++) {
        memcpy(buffer + p, g->second.header, g->second.headersize);
        p += g->second.headersize;
        // printf ("Bitmapsize: %zi\n",g->second.bitmapsize);
        // memset(buffer+p,7,g->second.bitmapsize);
        memcpy(buffer + p, g->second.bitmap, g->second.bitmapsize);
        p += g->second.bitmapsize;
    }
    // printf ("p=%zi, bytes=%zi\n",p,bytes);
    PFPChunk* facechunk = new PFPChunk;
    facechunk->setName("FACE");
    facechunk->setData(buffer, bytes);
    addChunk(facechunk);
    free(buffer);
    return 1;
}

int CFont6Generator::LoadRequest(const char* id, int mainversion, int subversion)
{
    // Handelt es sich auch um eine Font5-Datei?
    if (strcmp(id, "FONT") != 0 || mainversion != 6 || subversion != 0) return 0;
    return 1;
}

void CFont6Generator::List()
{
    return List(false);
}

void CFont6Generator::List(bool withGlyphs)
{
    const char* comp = "unkomprimiert";
    if (this->getCompression() == 1) comp = "Zlib-Komprimierung";
    if (this->getCompression() == 2) comp = "Bzip2-Komprimierung";
    printf("PFP-File Version 3, %s Version %i.%i, %s\n", (const char*)getID(), getMainVersion(), getSubVersion(), comp);
    String tmp;
    if ((tmp = getName()).notEmpty()) printf("Name:        %s\n", (const char*)tmp);
    if ((tmp = getAuthor()).notEmpty()) printf("Author:      %s\n", (const char*)tmp);
    if ((tmp = getCopyright()).notEmpty()) printf("Copyright:   %s\n", (const char*)tmp);
    if ((tmp = getDescription()).notEmpty()) printf("Description: %s\n", (const char*)tmp);

    ppl7::PFPFile::Iterator it;
    reset(it);
    PFPChunk* c;
    char* b;
    int flags;
    int pixelformat;
    ppl7::String s;
    while ((c = (PFPChunk*)getNext(it))) {
        if (c->name() == "FACE") {
            b = (char*)c->data();
            printf("FACE Fontsize: %i, Size: %zd Byte, Glyphs: %i", Peek16(b + 2), c->size(), Peek16(b + 10));
            printf(", MaxBY: %i, MaxHeight: %i, Underscore: %i", Peek16(b + 4), Peek16(b + 6), Peek16(b + 8));
            flags = Peek8(b);
            pixelformat = Peek8(b + 1);
            s.clear();
            if (flags & 1) {
                s += "Antialiased, ";
                if (pixelformat == 3) s += "8 Bit/Pixel, ";
                if (pixelformat == 4) s += "2 Bit/Pixel, ";
                if (pixelformat == 5) s += "4 Bit/Pixel, ";
            } else {
                s += "Monochrom, ";
                if (pixelformat == 1) s += "8 Bit/Pixel, ";
                if (pixelformat == 2) s += "1 Bit/Pixel, ";
            }
            if (flags & 2) s += "Bold, ";
            if (flags & 4) s += "Italic, ";
            if (flags & 8) s += "with Hints, ";
            s.chop(2);
            if (s.len())
                printf(", Flags: %s\n", (const char*)s);
            else
                printf(", Flags: keine\n");
            if (withGlyphs) ListGlyphs(c);
        }
    }
}

void CFont6Generator::ListGlyphs(PFPChunk* c)
{
    char* b = (char*)c->data() + 12;
    uint32_t num = 0;
    while (1) {
        uint32_t chunksize = ppl7::Peek32(b);
        uint16_t unicode = ppl7::Peek16(b + 4);
        if (chunksize == 0 || unicode == 0) break;
        printf("%4d: Unicode: %5d, Chunksize: %5d\n", num, unicode, chunksize);
        num++;
        b += chunksize;
    }

    /* TODO
    char *header=(char*)c->Data();
    char *jumpindex=header+12;
    char *glyph;
    CWString s;
    for (int j=0;j<(int)peek16(header+10);j++) {
        int start=peek16(jumpindex);
        int end=peek16(jumpindex+2);
        char *jump=header+peek32(jumpindex+4);
        jumpindex+=8;
        ppluint32 p;
        for (int i=start;i<=end;i++) {
            s.SetChar(i);

            p=peek32(jump+(i-start)*4);
            if (p) {
                glyph=header+p;
                pplint16 w=peek16(glyph+0);
                pplint16 h=peek16(glyph+2);
                pplint16 bx=peek16(glyph+4);
                pplint16 by=peek16(glyph+6);
                pplint16 ad=peek16(glyph+8);
                printf ("Char %5i: Size: %3i x %3i, Bearing x: %3i, y: %3i, Advance: %i   - ", i,
                        w,
                        h,
                        bx,
                        by,
                        ad);
                s.Print(true);
            } else {
                printf ("Char %5i: nicht vorhanden\n",i);
            }
        }
    }
    */
}

#include <stdlib.h>
#include "fontmaker.h"

Font5Glyph::Font5Glyph()
{
    bitmap = NULL;
    memset(header, 0, 8);
    size = 0;
    unicode = 0;
}

Font5Glyph::Font5Glyph(const Font5Glyph& other)
{
    unicode = other.unicode;
    memcpy(header, other.header, 10);
    bitmap = NULL;
    size = other.size;
    if (other.bitmap) {
        bitmap = (char*)malloc(size);
        if (!bitmap) throw ppl7::OutOfMemoryException();
        memcpy(bitmap, other.bitmap, size);
    }
}

Font5Glyph::~Font5Glyph()
{
    if (bitmap) free(bitmap);
}

Font5Glyph& Font5Glyph::operator=(const Font5Glyph& other)
{
    if (bitmap) free(bitmap);
    unicode = other.unicode;
    memcpy(header, other.header, 10);
    bitmap = NULL;
    size = other.size;
    if (other.bitmap) {
        bitmap = (char*)malloc(size);
        if (!bitmap) throw ppl7::OutOfMemoryException();
        memcpy(bitmap, other.bitmap, size);
    }
    return *this;
}

CFont5Generator::CFont5Generator()
{
    setVersion(5, 0);
    fontchunk = new PFPChunk;
    fontchunk->setName("FONT");
    addChunk(fontchunk);
}

CFont5Generator::~CFont5Generator()
{
    Glyphs.clear();
}

int CFont5Generator::AddCharRange(wchar_t start, wchar_t end)
{
    // Zuerst prüfen, ob es die Range schon gibt
    ppl7::AssocArray::Iterator it;
    CharRanges.reset(it);
    ppl6::CAssocArray* r;
    while ((r = CharRanges.getNextArray())) {
        wchar_t s = (wchar_t)ppl6::atoi(r->Get("start"));
        wchar_t e = (wchar_t)ppl6::atoi(r->Get("end"));
        if (start >= s && start <= e) return 0;
        if (end >= s && end <= e) return 0;
        if (start <= s && end >= e) return 0;
    }
    // Ok, es gibt keine Überschneidungen
    ppl6::CAssocArray a;
    a.Setf("start", "%i", start);
    a.Setf("end", "%i", end);
    CharRanges.Set("[]", &a);
    return 1;
}

int CFont5Generator::AddChar(wchar_t unicode)
{
    return AddCharRange(unicode, unicode);
}

int CFont5Generator::AddGlyph(wchar_t code, FONTRENDER* render)
{
    Font5Glyph g;
    g.unicode = code;
    Poke16(g.header + 0, render->width);
    Poke16(g.header + 2, render->height);
    Poke16(g.header + 4, render->bearingx);
    Poke16(g.header + 6, render->bearingy);
    Poke16(g.header + 8, render->advance);
    g.bitmap = render->buffer;
    g.size = render->buffersize + 10;
    Glyphs.insert(std::pair<wchar_t, Font5Glyph>(code, g));
    return 1;
}

int CFont5Generator::Generate(int fontsize, int flags)
{
    if (!bfaceloaded) return 0;
    // Fontgroesse einstellen
    // FT_Set_Char_Size(face,0,fontsize*64,72,72);
    FT_Set_Pixel_Sizes(face, 0, fontsize + 2);
    FONTRENDER glyph;
    DeleteFace(fontsize, flags & 7);
    glyph.buffer = NULL;
    glyph.flags = flags;

    int size = 0;
    int maxheight = 0;
    int maxunderlength = -9999999;
    int maxbearingy = 0;
    wchar_t start, end;
    int numJumpTables = 0;
    int numGlyphs = 0;
    CharRanges.Reset();
    ppl6::CAssocArray* r;
    while ((r = CharRanges.GetNextArray())) {
        numJumpTables++;
        start = ppl6::atoi(r->Get("start"));
        end = ppl6::atoi(r->Get("end"));
        for (int i = start; i <= end; i++) {
            numGlyphs++;
            if (Render(i, &glyph)) {
                if (glyph.bearingy > maxbearingy) maxbearingy = glyph.bearingy;
                if (0 - glyph.bearingy + glyph.height > maxunderlength) maxunderlength = 0 - glyph.bearingy + glyph.height;
                if (!AddGlyph(i, &glyph)) {
                    return 0;
                }
                size += glyph.buffersize + 10;
            }
        }
    }
    maxheight = maxunderlength + maxbearingy + 1;
    // Speicherbedarf berechnen:
    int bytes = 12;             // Header des FACE
    bytes += numJumpTables * 8; // Anzahl Sprungtabellen
    bytes += (numGlyphs * 4);   // Anzahl Glyphs
    bytes += size;              // Größe der Glyphs

    char* buffer = (char*)malloc(bytes);
    if (!buffer) {
        printf("ERROR: Fehler beim Reservieren von %d Bytes Speicher\n", bytes);
        return 0;
    }
    memset(buffer, 0, bytes);
    // Zuerst schreiben wir den Header
    int f = 0;
    if (flags & FONTFLAGS::ANTIALIAS) f |= 1;
    if (flags & FONTFLAGS::ISBOLD) f |= 2;
    if (flags & FONTFLAGS::GENERATEBOLD) f |= 2;
    if (flags & FONTFLAGS::ISITALIC) f |= 4;
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
    Poke16(buffer + 10, numJumpTables);
    // Nun bauen wir die Sprungtabelle und die Glyphs zusammen
    char* jumpindex = buffer + 12;
    char* jump = buffer + 12 + numJumpTables * 8;
    int p = 12 + numJumpTables * 8 + numGlyphs * 4;
    char* b = buffer + p;
    Font5Glyph* g;
    CharRanges.Reset();
    while ((r = CharRanges.GetNextArray())) {
        start = ppl6::atoi(r->Get("start"));
        end = ppl6::atoi(r->Get("end"));
        Poke16(jumpindex + 0, start);
        Poke16(jumpindex + 2, end);
        Poke32(jumpindex + 4, jump - buffer);
        jumpindex += 8;
        for (int i = start; i <= end; i++) {
            g = (Font5Glyph*)Glyphs.find(&i);
            if (g) {
                Poke32(jump, p);
                memcpy(b, g->header, 10);
                memcpy(b + 10, g->bitmap, g->size - 10);
                b += g->size;
                p += g->size;
            }
            jump += 4;
        }
    }

    PFPChunk* facechunk = new PFPChunk;
    facechunk->setName("FACE");
    facechunk->setData(buffer, bytes);
    addChunk(facechunk);
    Glyphs.clear();
    free(buffer);

    return 1;
}

int CFont5Generator::LoadRequest(const char* id, int mainversion, int subversion)
{
    // Handelt es sich auch um eine Font5-Datei?
    if (strcmp(id, "FONT") != 0 || mainversion != 5 || subversion != 0) return 0;
    return 1;
}

void CFont5Generator::List()
{
    return List(false);
}

void CFont5Generator::List(bool withGlyphs)
{
    const char* comp = "unkomprimiert";
    if (this->getCompression() == 1) comp = "Zlib-Komprimierung";
    if (this->getCompression() == 2) comp = "Bzip2-Komprimierung";
    printf("PFP-File Version 3, %s Version %i.%i, %s\n", (const char*)getID(), getMainVersion(), getSubVersion(), comp);
    ppl7::String tmp;
    if ((tmp = getName()).notEmpty()) printf("Name:        %s\n", (const char*)tmp);
    if ((tmp = getAuthor()).notEmpty()) printf("Author:      %s\n", (const char*)tmp);
    if ((tmp = getCopyright()).notEmpty()) printf("Copyright:   %s\n", (const char*)tmp);
    if ((tmp = getDescription()).notEmpty()) printf("Description: %s\n", (const char*)tmp);

    ppl7::PFPFile::Iterator it;
    reset(it);
    PFPChunk* c;
    char* b;
    char* jumpindex;
    int flags;
    int pixelformat;
    ppl7::String s;
    while ((c = (PFPChunk*)getNext(it))) {
        if (c->name() == "FACE") {
            b = (char*)c->data();
            jumpindex = b + 12;
            printf("FACE Fontsize: %i, Size: %i Byte, Chartable: ", Peek16(b + 2), c->size());
            for (int i = 0; i < (int)Peek16(b + 10); i++) {
                if (i) printf(", ");
                if (Peek16(jumpindex) != Peek16(jumpindex + 2))
                    printf("%i-%i", Peek16(jumpindex), Peek16(jumpindex + 2));
                else
                    printf("%i", Peek16(jumpindex));
                jumpindex += 8;
            }
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
            s.chop(2);
            if (s.len())
                printf(", Flags: %s\n", (const char*)s);
            else
                printf(", Flags: keine\n");
            if (withGlyphs) ListGlyphs(c);
        }
    }
}

void CFont5Generator::ListGlyphs(PFPChunk* c)
{
    char* header = (char*)c->data();
    char* jumpindex = header + 12;
    char* glyph;
    WideString s;
    for (int j = 0; j < (int)Peek16(header + 10); j++) {
        wchar_t start = Peek16(jumpindex);
        wchar_t end = Peek16(jumpindex + 2);
        char* jump = header + Peek32(jumpindex + 4);
        jumpindex += 8;
        uint32_t p;
        for (wchar_t i = start; i <= end; i++) {
            s.set(i);
            p = Peek32(jump + (i - start) * 4);
            if (p) {
                glyph = header + p;
                int16_t w = Peek16(glyph + 0);
                int16_t h = Peek16(glyph + 2);
                int16_t bx = Peek16(glyph + 4);
                int16_t by = Peek16(glyph + 6);
                int16_t ad = Peek16(glyph + 8);
                printf("Char %5i: Size: %3i x %3i, Bearing x: %3i, y: %3i, Advance: %i   - ", i, w, h, bx, by, ad);
                s.print(true);
            } else {
                printf("Char %5i: nicht vorhanden\n", i);
            }
        }
    }
}

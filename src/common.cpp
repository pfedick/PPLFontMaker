#include <stdlib.h>
#include "fontmaker.h"

CFontGeneratorCommon::CFontGeneratorCommon()
{
    setId("FONT");
    // Freetype initialisieren
    FT_Init_FreeType(&ftlib);
    face = NULL;
    bfaceloaded = false;
}

CFontGeneratorCommon::~CFontGeneratorCommon()
{
    // FreeType freigeben
    if (bfaceloaded) FT_Done_Face(face);
    FT_Done_FreeType(ftlib);
}

int CFontGeneratorCommon::LoadFont(const char* file)
{
    if (bfaceloaded) FT_Done_Face(face);
    bfaceloaded = false;
    int error = FT_New_Face(ftlib, file, 0, &face); /* create face object */
    if (error != 0) return 0;
    bfaceloaded = true;
    return 1;
}

// Erzeugt eine um ein Pixel nach rechts versetzte Kopie der Glyph-Bitmap.
// Zusammen mit der unveraenderten Bitmap ergibt das den Fake-Fett-Effekt,
// jeder Buchstabe wird zweimal mit einem Pixel Versatz gezeichnet.
static char* CreateBoldSource(const uint8_t* source, int pitch, int width, int rows, bool mono, int* outPitch)
{
    int dilatedWidth = width + 1;
    int outRowBytes = mono ? ((dilatedWidth + 7) / 8) : dilatedWidth;
    if (outRowBytes <= 0 || rows <= 0) return NULL;
    char* out = (char*)calloc((size_t)outRowBytes * rows, 1);
    if (!out) return NULL;

    for (int y = 0; y < rows; y++) {
        const uint8_t* src = source + (size_t)y * pitch;
        uint8_t* dst = (uint8_t*)out + (size_t)y * outRowBytes;
        if (mono) {
            // Die Bits einer Zeile stehen MSB-zuerst; ein Pixel nach rechts
            // verschieben bedeutet also ein Bit nach rechts plus Uebertrag.
            for (int b = 0; b < outRowBytes; b++) {
                uint8_t cur = (b < pitch) ? src[b] : 0;
                uint8_t carry = (b > 0) ? (uint8_t)(src[b - 1] << 7) : 0;
                dst[b] = (uint8_t)(cur | (cur >> 1) | carry);
            }
            int padBits = outRowBytes * 8 - dilatedWidth;
            if (padBits > 0) dst[outRowBytes - 1] &= (uint8_t)(0xff << padBits);
        } else {
            for (int x = 0; x < dilatedWidth; x++) {
                uint8_t cur = (x < width) ? src[x] : 0;
                uint8_t prev = (x > 0 && (x - 1) < width) ? src[x - 1] : 0;
                dst[x] = (cur > prev) ? cur : prev;
            }
        }
    }
    *outPitch = outRowBytes;
    return out;
}

int CFontGeneratorCommon::Render(int code, FONTRENDER* render)
{
    FT_GlyphSlot slot = face->glyph;
    FT_UInt glyph_index;
    int error;

    glyph_index = FT_Get_Char_Index(face, code);
    if (!glyph_index) return 0;

    // Anti Aliasing oder nicht?
    if (render->flags & FONTFLAGS::ANTIALIAS)
        error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_RENDER);
    else
        error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_TARGET_MONO | FT_LOAD_RENDER);
    if (error != 0) return 0;

    /*
    error=FT_Load_Glyph(face,glyph_index,FT_LOAD_DEFAULT);
    if (error!=0) return 0;

    // Anti Aliasing oder nicht?
    if (render->flags&FONTFLAGS::ANTIALIAS)
        FT_Render_Glyph(face->glyph,FT_RENDER_MODE_NORMAL);
    else
        FT_Render_Glyph(face->glyph,FT_RENDER_MODE_MONO);
    */

    // In eine einheitliche Bitmap umwandeln, 1 Byte pro Pixel
    render->width = slot->bitmap.width;
    render->height = slot->bitmap.rows;
    render->advance = (slot->advance.x) >> 6;
    render->bearingy = slot->bitmap_top;
    render->bearingx = slot->bitmap_left;
    /*
    HexDump(render,10);
    printf ("render->width: %i, ",render->width);
    printf ("render->height: %i\n",render->height);
    */
    // Fake-Fett: Die Glyph-Bitmap vor der Umrechnung um ein Pixel nach rechts
    // verbreitern. Dadurch laufen alle Pixelformate über denselben Code.
    bool isMono = (slot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO);
    int srcWidth = slot->bitmap.width;
    int srcPitch = slot->bitmap.pitch;
    char* boldSource = NULL;
    if (render->flags & FONTFLAGS::GENERATEBOLD) {
        boldSource = CreateBoldSource((const uint8_t*)slot->bitmap.buffer, srcPitch, srcWidth, (int)slot->bitmap.rows, isMono, &srcPitch);
        if (!boldSource) {
            printf("ERROR: Fehler beim Reservieren der Fett-Vorlage\n");
            return 0;
        }
        srcWidth++;
        render->advance++;
    }
    render->width = srcWidth;

    render->buffersize = render->width * render->height;
    if (render->flags & FONTFLAGS::MONO1)
        render->buffersize = (render->buffersize >> 3) + 1;
    else if (render->flags & FONTFLAGS::AA2)
        render->buffersize = (render->buffersize >> 2) + 1;
    else if (render->flags & FONTFLAGS::AA4)
        render->buffersize = (render->buffersize >> 1) + 1;

    render->buffer = (char*)malloc(render->buffersize);
    if (!render->buffer) {
        printf("ERROR: Fehler beim Reservieren von %d Bytes Speicher\n", render->buffersize);
        free(boldSource);
        return 0;
    }
    memset(render->buffer, 0, render->buffersize);

    uint8_t* glyph = boldSource ? (uint8_t*)boldSource : (uint8_t*)slot->bitmap.buffer;
    char* zz = render->buffer;

    uint8_t pixel;
    uint8_t value;

    if (render->flags & FONTFLAGS::AA2) {
        uint8_t bitcount = 0;
        uint8_t packed = 0;
        for (int y = 0; y < (int)slot->bitmap.rows; y++) {
            for (int x = 0; x < srcWidth; x++) {
                pixel = glyph[x];
                value = 0;
                if (pixel) {
                    if (pixel > 200) value = 3;
                    else if (pixel > 150) value = 2;
                    else if (pixel > 100) value = 1;
                }
                packed = (uint8_t)((packed << 2) | value);
                bitcount += 2;
                if (bitcount == 8) {
                    *zz++ = (char)packed;
                    packed = 0;
                    bitcount = 0;
                }
            }
            glyph += srcPitch;
        }
        if (bitcount) {
            packed = (uint8_t)(packed << (8 - bitcount));
            *zz = (char)packed;
        }
    } else if (render->flags & FONTFLAGS::AA4) {
        uint8_t bitcount = 0;
        uint8_t packed = 0;
        for (int y = 0; y < (int)slot->bitmap.rows; y++) {
            for (int x = 0; x < srcWidth; x++) {
                pixel = glyph[x];
                if (pixel) value = (uint8_t)((pixel * 15 / 255) & 15);
                else value = 0;
                packed = (uint8_t)((packed << 4) | value);
                bitcount += 4;
                if (bitcount == 8) {
                    *zz++ = (char)packed;
                    packed = 0;
                    bitcount = 0;
                }
            }
            glyph += srcPitch;
        }
        if (bitcount) {
            packed = (uint8_t)(packed << (8 - bitcount));
            *zz = (char)packed;
        }
    } else if (render->flags & FONTFLAGS::ANTIALIAS) {
        for (int y = 0; y < (int)slot->bitmap.rows; y++) {
            for (int x = 0; x < srcWidth; x++) zz[x] = (char)glyph[x];
            glyph += srcPitch;
            zz += render->width;
        }
    } else if (render->flags & FONTFLAGS::MONO8) {
        // Die Quelldateien von FreeType liegen hier 1 Bit pro Pixel vor.
        for (int y = 0; y < (int)slot->bitmap.rows; y++) {
            for (int x = 0; x < srcWidth; x++) {
                if (glyph[x >> 3] & (uint8_t)(0x80 >> (x & 7))) zz[x] = (char)255;
                else zz[x] = 0;
            }
            glyph += srcPitch;
            zz += render->width;
        }
    } else if (render->flags & FONTFLAGS::MONO1) {
        uint8_t bitcount = 0;
        uint8_t packed = 0;
        for (int y = 0; y < (int)slot->bitmap.rows; y++) {
            for (int x = 0; x < srcWidth; x++) {
                packed = (uint8_t)(packed << 1);
                if (glyph[x >> 3] & (uint8_t)(0x80 >> (x & 7))) packed |= 1;
                bitcount++;
                if (bitcount == 8) {
                    *zz++ = (char)packed;
                    packed = 0;
                    bitcount = 0;
                }
            }
            glyph += srcPitch;
        }
        if (bitcount) {
            packed = (uint8_t)(packed << (8 - bitcount));
            *zz = (char)packed;
        }
    }

    free(boldSource);

    return 1;
}


// Loschen alle Faces, die zu Groesse, Pixelformat und Stil der angegebenen
// Commandline-Flags passen. Das Hints-Bit im Flag-Byte bleibt aussen vor.
void CFontGeneratorCommon::DeleteFace(int size, int flags)
{
    pplib::PFPFile::Iterator it;
    PFPChunk* c;
    char* b;
    int style = FaceStyleFlag(flags);
    int format = FacePixelFormat(flags);
    while ((c = (PFPChunk*)findNextChunk(it, "FACE"))) {
        b = (char*)c->data();
        if (!b) continue;
        if ((pplib::Peek8(b) & 7) == style && pplib::Peek8(b + 1) == format && (int)Peek16(b + 2) == size) {
            deleteChunk(c);
            reset(it);
        }
    }
}

void CFontGeneratorCommon::CopyFreeTypeName()
{
    if (face) {
        setName(face->family_name);
    }
}

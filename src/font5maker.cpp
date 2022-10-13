#include <stdlib.h>
#include "fontmaker.h"


Font5Glyph::Font5Glyph()
{
	bitmap=NULL;
	memset(header,0,8);
	size=0;
	unicode=0;
}

Font5Glyph::~Font5Glyph()
{
	if (bitmap) free(bitmap);
}

int Font5Glyph::CompareNode(CTreeItem *item)
{
	Font5Glyph *g=(Font5Glyph*)item;
	if (g->unicode<unicode) return -1;
	if (g->unicode>unicode) return 1;
	return 0;
}

int Font5Glyph::CompareValue(void *value)
{
	int *g=(int*)value;
	if (*g<unicode) return -1;
	if (*g>unicode) return 1;
	return 0;
}

CFont5Generator::CFont5Generator()
{
	SetVersion(5,0);
	fontchunk=new PFPChunk;
	fontchunk->SetName("FONT");
	AddChunk(fontchunk);
}

CFont5Generator::~CFont5Generator()
{
	Glyphs.Clear(true);

}

int CFont5Generator::AddCharRange(wchar_t start, wchar_t end)
{
	// Zuerst prüfen, ob es die Range schon gibt
	CharRanges.Reset();
	ppl6::CAssocArray *r;
	while ((r=CharRanges.GetNextArray())) {
		wchar_t s=(wchar_t)ppl6::atoi(r->Get("start"));
		wchar_t e=(wchar_t)ppl6::atoi(r->Get("end"));
		if (start>=s && start<=e) return 0;
		if (end>=s && end<=e) return 0;
		if (start<=s && end>=e) return 0;
	}
	// Ok, es gibt keine Überschneidungen
	ppl6::CAssocArray a;
	a.Setf("start","%i",start);
	a.Setf("end","%i",end);
	CharRanges.Set("[]",&a);
	return 1;
}

int CFont5Generator::AddChar(wchar_t unicode)
{
	return AddCharRange(unicode,unicode);
}


int CFont5Generator::AddGlyph(wchar_t code, FONTRENDER *render)
{
	Font5Glyph *g=new Font5Glyph;
	g->unicode=code;
	poke16(g->header+0,render->width);
	poke16(g->header+2,render->height);
	poke16(g->header+4,render->bearingx);
	poke16(g->header+6,render->bearingy);
	poke16(g->header+8,render->advance);
	g->bitmap=render->buffer;
	g->size=render->buffersize+10;
	if (!Glyphs.Add(g)) {
		PushError();
		delete g;
		PopError();
		return 0;
	}
	return 1;
}

int CFont5Generator::Generate(int fontsize, int flags)
{
	if (!bfaceloaded) return 0;
	// Fontgroesse einstellen
	//FT_Set_Char_Size(face,0,fontsize*64,72,72);
	FT_Set_Pixel_Sizes(face,0,fontsize+2);
	FONTRENDER glyph;
	DeleteFace(fontsize,flags&7);
	glyph.buffer=NULL;
	glyph.flags=flags;

	int size=0;
	int maxheight=0;
	int maxunderlength=-9999999;
	int maxbearingy=0;
	int start,end;
	int numJumpTables=0;
	int numGlyphs=0;
	CharRanges.Reset();
	ppl6::CAssocArray *r;
	while ((r=CharRanges.GetNextArray())) {
		numJumpTables++;
		start=ppl6::atoi(r->Get("start"));
		end=ppl6::atoi(r->Get("end"));
		for (int i=start;i<=end;i++) {
			numGlyphs++;
			if (Render (i,&glyph)) {
				if (glyph.bearingy>maxbearingy) maxbearingy=glyph.bearingy;
				if (0-glyph.bearingy+glyph.height > maxunderlength) maxunderlength=0-glyph.bearingy+glyph.height;
				if (!AddGlyph(i,&glyph)) {
					return 0;
				}
				size+=glyph.buffersize+10;
			}
		}
	}
	maxheight=maxunderlength+maxbearingy+1;
	// Speicherbedarf berechnen:
	int bytes=12;				// Header des FACE
	bytes+=numJumpTables*8;		// Anzahl Sprungtabellen
	bytes+=(numGlyphs*4);		// Anzahl Glyphs
	bytes+=size;				// Größe der Glyphs

	char *buffer=(char*)malloc(bytes);
	if (!buffer) {
		SetError(2);
		return 0;
	}
	memset(buffer,0,bytes);
	// Zuerst schreiben wir den Header
	int f=0;
	if (flags&FONTFLAGS::ANTIALIAS) f|=1;
	if (flags&FONTFLAGS::ISBOLD) f|=2;
	if (flags&FONTFLAGS::GENERATEBOLD) f|=2;
	if (flags&FONTFLAGS::ISITALIC) f|=4;
	poke8(buffer+0,f);
	if (flags&FONTFLAGS::AA2) poke8(buffer+1,4);
	else if (flags&FONTFLAGS::AA4) poke8(buffer+1,5);
	else if (flags&FONTFLAGS::ANTIALIAS) poke8(buffer+1,3);
	else if (flags&FONTFLAGS::MONO1) poke8(buffer+1,2);
	else poke8(buffer+1,1);
	poke16(buffer+2,fontsize);
	poke16(buffer+4,maxbearingy);
	poke16(buffer+6,maxheight);
	poke16(buffer+8,(0-face->underline_position)>>6);
	poke16(buffer+10,numJumpTables);
	// Nun bauen wir die Sprungtabelle und die Glyphs zusammen
	char *jumpindex=buffer+12;
	char *jump=buffer+12+numJumpTables*8;
	int p=12+numJumpTables*8+numGlyphs*4;
	char *b=buffer+p;
	Font5Glyph *g;
	CharRanges.Reset();
	while ((r=CharRanges.GetNextArray())) {
		start=ppl6::atoi(r->Get("start"));
		end=ppl6::atoi(r->Get("end"));
		poke16(jumpindex+0,start);
		poke16(jumpindex+2,end);
		poke32(jumpindex+4,jump-buffer);
		jumpindex+=8;
		for (int i=start;i<=end;i++) {
			g=(Font5Glyph *)Glyphs.Find(&i);
			if (g) {
				poke32(jump,p);
				memcpy(b,g->header,10);
				memcpy(b+10,g->bitmap,g->size-10);
				b+=g->size;
				p+=g->size;
			}
			jump+=4;
		}
	}

	PFPChunk *facechunk=new PFPChunk;
	facechunk->SetName("FACE");
	facechunk->SetData(buffer,bytes);
	AddChunk(facechunk);
	Glyphs.Clear(true);
	free(buffer);

	return 1;
}

int CFont5Generator::LoadRequest(const char *id, int mainversion ,int subversion)
{
	// Handelt es sich auch um eine Font5-Datei?
	if (strcmp(id,"FONT")!=0 || mainversion!=5 || subversion!=0) return 0;
	return 1;
}

void CFont5Generator::List()
{
	return List(false);
}

void CFont5Generator::List(bool withGlyphs)
{
	const char *comp="unkomprimiert";
	if (this->GetCompression()==1) comp="Zlib-Komprimierung";
	if (this->GetCompression()==2) comp="Bzip2-Komprimierung";
	printf ("PFP-File Version 3, %s Version %i.%i, %s\n",
			GetID(), GetMainVersion(), GetSubVersion(),comp);
	const char *tmp;
	if ((tmp=GetName())) printf ("Name:        %s\n",tmp);
	if ((tmp=GetAuthor())) printf ("Author:      %s\n",tmp);
	if ((tmp=GetCopyright())) printf ("Copyright:   %s\n",tmp);
	if ((tmp=GetDescription())) printf ("Description: %s\n",tmp);

	Reset();
	PFPChunk *c;
	char *b;
	char *jumpindex;
	int flags;
	int pixelformat;
	ppl6::CString s;
	while ((c=(PFPChunk*)GetNext())) {
		if (strcmp(c->Name(),"FACE")==0) {
			b=(char*)c->Data();
			jumpindex=b+12;
			printf ("FACE Fontsize: %i, Size: %i Byte, Chartable: ", peek16(b+2),
					c->Size());
			for (int i=0;i<(int)peek16(b+10);i++) {
				if (i) printf (", ");
				if (peek16(jumpindex)!=peek16(jumpindex+2))
					printf("%i-%i",peek16(jumpindex),peek16(jumpindex+2));
				else
					printf("%i",peek16(jumpindex));
				jumpindex+=8;
			}
			printf (", MaxBY: %i, MaxHeight: %i, Underscore: %i",peek16(b+4),peek16(b+6),peek16(b+8));
			flags=peek8(b);
			pixelformat=peek8(b+1);
			s.Clear();
			if (flags&1) {
				s+="Antialiased, ";
				if (pixelformat==3) s+="8 Bit/Pixel, ";
				if (pixelformat==4) s+="2 Bit/Pixel, ";
				if (pixelformat==5) s+="4 Bit/Pixel, ";
			} else {
				s+="Monochrom, ";
				if (pixelformat==1) s+="8 Bit/Pixel, ";
				if (pixelformat==2) s+="1 Bit/Pixel, ";
			}
			if (flags&2) s+="Bold, ";
			if (flags&4) s+="Italic, ";
			s.Chop(2);
			if (s.Len()) printf (", Flags: %s\n",(const char*)s);
			else printf(", Flags: keine\n");
			if (withGlyphs) ListGlyphs(c);
		}
	}
}

void CFont5Generator::ListGlyphs(PFPChunk *c)
{
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
}


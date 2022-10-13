#ifndef FONTMAKER_H
#define FONTMAKER_H
#include <ft2build.h>
#include FT_FREETYPE_H

#include <ppl6.h>
#include <ppl6-grafix.h>
#include <set>
#include <map>

using namespace ppl6;


#define FM_VERSION	"1.4.0"
#define FM_RELEASEDATE "04.02.2012"
#define FM_AUTHOR "Patrick Fedick"
#define FM_COPYRIGHT "(c) Copyright by Patrick Fedick in 2012"


namespace FONTFLAGS {
enum {
	ANTIALIAS		=	0x00000001,
	ISBOLD			=	0x00000002,
	ISITALIC		=	0x00000004,
	GENERATEBOLD	=	0x00000008,
	GENERATESHADOW	=	0x00000010,
	GENERATEOUTLINE	=	0x00000020,
	MONO1			=	0x00000040,
	MONO8			=	0x00000080,
	AA2				=	0x00000100,
	AA4				=	0x00000200
};
}

class CFontGeneratorCommon;

class Main
{
	private:
		const char *edit;
		const char *quelle;
		const char *target;
		int flags;
		ppl6::CArray Todo;
		int fontVersion;

		CFontGeneratorCommon *font;

		int list(const char *filename, bool withglyphs=false);
		int checkFlags(int argc, char **argv);
		int getSizes(int argc, char **argv);
		int getFiles(int argc, char **argv);
		int work(int argc, char **argv);
		int getChars(int argc, char **argv);


	public:
		Main();
		~Main();
		void help();
		void version();
		int start(int argc, char **argv);

};


typedef struct {
	//RECT	outline;
	int		flags;
	int		width,height;
	int		bearingy;
	int		bearingx;
	int		advance;
	char	*buffer;
	int		buffersize;
} FONTRENDER;

class CFont5Generator;

class Font5Glyph : public ppl6::CTreeItem
{
	friend class CFont5Generator;
	private:
		wchar_t unicode;
		char header[10];
		void *bitmap;
		int size;
	public:
		Font5Glyph();
		virtual ~Font5Glyph();
		virtual int CompareNode(CTreeItem *item);
		virtual int CompareValue(void *value);

};


class CFontGeneratorCommon : public ppl6::PFPFile
{
	friend class CFont5Generator;
	protected:
		FT_Library	ftlib;
		FT_Face		face;
		bool		bfaceloaded;

	public:
		CFontGeneratorCommon();
		virtual ~CFontGeneratorCommon();

		int  LoadFont(const char *file);

		int	Render(int code, FONTRENDER *render);
		int BltGlyph(ppl6::grafix::CDrawable &surface, int x, int y, FONTRENDER *glyph, ppl6::grafix::Color &c);

		void DeleteFace(int size, int flags);
		void CopyFreeTypeName();

		virtual int  AddCharRange(wchar_t start, wchar_t end) =0;
		virtual int  AddChar(wchar_t unicode)=0;
		virtual int  Generate(int size, int flags)=0;
		virtual void List()=0;
		virtual void List(bool withGlyphs)=0;
		virtual void ListGlyphs(PFPChunk *c)=0;

};


class CFont5Generator : public CFontGeneratorCommon
{
	private:
		PFPChunk			*fontchunk;
		int				debugx,debugy;
		ppl6::CAssocArray	CharRanges;
		ppl6::CTree	Glyphs;

		int AddGlyph(wchar_t code, FONTRENDER *render);

	public:
		CFont5Generator();
		virtual ~CFont5Generator();
		virtual int  AddCharRange(wchar_t start, wchar_t end);
		virtual int  AddChar(wchar_t unicode);
		virtual int  Generate(int size, int flags);

		virtual int LoadRequest(const char *id, int mainversion ,int subversion);
		virtual void List();
		virtual void List(bool withGlyphs);
		virtual void ListGlyphs(PFPChunk *c);
};


class Font6Glyph
{
	public:
		wchar_t unicode;
		char *header;
		size_t headersize;
		char *bitmap;
		size_t bitmapsize;

		Font6Glyph();
		Font6Glyph(const Font6Glyph &other);
		~Font6Glyph();

		Font6Glyph & operator =(const Font6Glyph &other);

};

class CFont6Generator : public CFontGeneratorCommon
{
	private:
		std::set<wchar_t>	CharList;
		std::map<wchar_t,Font6Glyph> Glyphs;
		size_t				totalGlyphSize;
		size_t				totalHintsSize;

		int AddGlyph(wchar_t code, FONTRENDER *render);

	public:
		CFont6Generator();
		virtual ~CFont6Generator();

		virtual int LoadRequest(const char *id, int mainversion ,int subversion);

		virtual int  AddCharRange(wchar_t start, wchar_t end);
		virtual int  AddChar(wchar_t unicode);
		virtual int  Generate(int size, int flags);
		virtual void List();
		virtual void List(bool withGlyphs);
		virtual void ListGlyphs(PFPChunk *c);
};

#endif

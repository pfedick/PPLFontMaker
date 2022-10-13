#include <stdlib.h>
#include "fontmaker.h"

CFontGeneratorCommon::CFontGeneratorCommon()
{
	SetId("FONT");
	// Freetype initialisieren
	FT_Init_FreeType( &ftlib );
	face=NULL;
	bfaceloaded=false;
}

CFontGeneratorCommon::~CFontGeneratorCommon()
{
	// FreeType freigeben
	if (bfaceloaded) FT_Done_Face(face);
	FT_Done_FreeType(ftlib);
}

int CFontGeneratorCommon::LoadFont(const char *file)
{
	if (bfaceloaded) FT_Done_Face(face);
	bfaceloaded=false;
	int error = FT_New_Face(ftlib, file, 0, &face ); /* create face object */
	if (error!=0) return 0;
	bfaceloaded=true;
	return 1;
}

int CFontGeneratorCommon::Render(int code, FONTRENDER *render)
{
	FT_GlyphSlot	slot=face->glyph;
	FT_UInt			glyph_index;
	int error;

	glyph_index=FT_Get_Char_Index(face,code);
	if (!glyph_index) return 0;

	// Anti Aliasing oder nicht?
	if (render->flags&FONTFLAGS::ANTIALIAS)
		error=FT_Load_Glyph(face,glyph_index,FT_LOAD_DEFAULT|FT_LOAD_RENDER);
	else
		error=FT_Load_Glyph(face,glyph_index,FT_LOAD_DEFAULT|FT_LOAD_TARGET_MONO|FT_LOAD_RENDER);
	if (error!=0) return 0;

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
	render->width=slot->bitmap.width;
	render->height=slot->bitmap.rows;
	render->advance=(slot->advance.x)>>6;
	render->bearingy=slot->bitmap_top;
	render->bearingx=slot->bitmap_left;
	/*
	HexDump(render,10);
	printf ("render->width: %i, ",render->width);
	printf ("render->height: %i\n",render->height);
	*/
	if (render->flags&FONTFLAGS::GENERATEBOLD) {
		render->width++;
		render->advance++;
	}
	render->buffersize=render->width*render->height;
	if (render->flags&FONTFLAGS::MONO1) render->buffersize=(render->buffersize>>3)+1;
	else if (render->flags&FONTFLAGS::AA2) render->buffersize=(render->buffersize>>2)+1;
	else if (render->flags&FONTFLAGS::AA4) render->buffersize=(render->buffersize>>1)+1;

	render->buffer=(char*)malloc(render->buffersize);
	if (!render->buffer) {
		SetError(2);
		return 0;
	}
	memset(render->buffer,0,render->buffersize);

	char *glyph=(char *)slot->bitmap.buffer;
	char *zz=render->buffer;


	ppldb w1,w2;
	int maxwidth=0;
	int minwidth=999999;
	if (render->flags&FONTFLAGS::AA2) {
		ppldb zbits=0;
		ppldb zv=0;
		for (int gy=0;gy<(int)slot->bitmap.rows;gy++) {
			for (int gx=0;gx<(int)slot->bitmap.width;gx++) {
				w1=glyph[gx];
				if (w1) {
					if (w1>200) w2=3;
					else if (w1>150) w2=2;
					else if (w1>100) w2=1;
					else w2=0;
					//w2=((w1*3)/255)&3;
					zv|=w2;
				}
				//printf ("w1=%i, w2=%i, zv=%i\n",w1,w2,zv);
				zbits+=2;
				if (zbits==8) {
					zz[0]=zv;
					zz++;
					zv=0;
					zbits=0;
				} else {
					zv=(zv<<2);
				}
			}
			glyph+=slot->bitmap.pitch;
		}
		if (zbits) {
			zv=zv<<(6-zbits);
			zz[0]=zv;
		}
		//HexDump(render->buffer,render->buffersize);
	} else if (render->flags&FONTFLAGS::AA4) {
		ppldb zbits=0;
		ppldb zv=0;
		for (int gy=0;gy<(int)slot->bitmap.rows;gy++) {
			for (int gx=0;gx<(int)slot->bitmap.width;gx++) {
				w1=glyph[gx];
				if (w1) {
					w2=(w1*15/255)&15;
					zv|=w2;
				}
				//printf ("w1=%i, w2=%i, zv=%i\n",w1,w2,zv);
				zbits+=4;
				if (zbits==8) {
					zz[0]=zv;
					zz++;
					zv=0;
					zbits=0;
				} else {
					zv=(zv<<4);
				}
			}
			glyph+=slot->bitmap.pitch;
		}
		if (zbits) {
			zv=zv<<(4-zbits);
			zz[0]=zv;
		}
		//HexDump(render->buffer,render->buffersize);
	} else if (render->flags&FONTFLAGS::ANTIALIAS) {
		for (int gy=0;gy<(int)slot->bitmap.rows;gy++) {
			for (int gx=0;gx<(int)slot->bitmap.width;gx++) {
				w1=glyph[gx];
				if (render->flags&FONTFLAGS::GENERATEBOLD) {
					if (w1>31) {
						if ((ppldb)zz[gx]<w1) zz[gx]=w1;
						zz[gx+1]=w1;
					} else {
						zz[gx]=w1;
					}
				} else {
					zz[gx]=w1;
				}
				if (w1) {
					if (gx<minwidth) minwidth=gx;
					if(gx>maxwidth) maxwidth=gx;
				}
			}
			glyph+=slot->bitmap.pitch;
			zz+=render->width;
		}
		//HexDump(render->buffer,render->buffersize);
		if (code>32) {		// Wie breit ist das Zeichen wirklich?
			//render->advance=maxwidth-minwidth+2;
			//if (minwidth>0) render->bearingx=0-minwidth;
		}
	} else if (render->flags&FONTFLAGS::MONO8) {
		ppldb bitcount=0;
		ppldb bytecount=0;
		for (int gy=0;gy<(int)slot->bitmap.rows;gy++) {
			for (int gx=0;gx<(int)slot->bitmap.width;gx++) {
				if (!bitcount) {
					w1=glyph[bytecount];
					bitcount=8;
					bytecount++;
				}
				if(w1&128) {
					zz[gx]=(ppldb)255;
					if (render->flags&FONTFLAGS::GENERATEBOLD) {
						zz[gx+1]=(ppldb)255;
					}
					if (gx<minwidth) minwidth=gx;
					if(gx>maxwidth) maxwidth=gx;
				}
				w1=w1<<1;
				bitcount--;
			}
			glyph+=slot->bitmap.pitch;
			bitcount=0;
			bytecount=0;
			zz+=render->width;
		}
		if (code>32) {		// Wie breit ist das Zeichen wirklich?
			//render->advance=maxwidth-minwidth+2;
			//if (minwidth>0) render->bearingx=0-minwidth;
		}
	} else if (render->flags&FONTFLAGS::MONO1) {
		ppldb bitcount=0;
		ppldb bytecount=0;
		ppldb zbits=0;
		ppldb zv=0;
		for (int gy=0;gy<(int)slot->bitmap.rows;gy++) {
			for (int gx=0;gx<(int)slot->bitmap.width;gx++) {
				if (!bitcount) {
					w1=glyph[bytecount];
					bitcount=8;
					bytecount++;
				}
				if(w1&128) {
					zv|=1;
					if (gx<minwidth) minwidth=gx;
					if(gx>maxwidth) maxwidth=gx;
				}
				w1=w1<<1;
				bitcount--;
				zbits++;
				if (zbits==8) {
					zz[0]=zv;
					zz++;
					zv=0;
					zbits=0;
				} else {
					zv=zv<<1;
				}
			}
			glyph+=slot->bitmap.pitch;
			bitcount=0;
			bytecount=0;
		}
		if (zbits) {
			zv=zv<<(7-zbits);
			zz[0]=zv;
		}

		if (code>32) {		// Wie breit ist das Zeichen wirklich?
			//render->advance=maxwidth-minwidth+2;
			//if (minwidth>0) render->bearingx=0-minwidth;
		}
	}


	return 1;
}


int CFontGeneratorCommon::BltGlyph(ppl6::grafix::CDrawable &surface, int x, int y, FONTRENDER *glyph, ppl6::grafix::Color &c)
{
	char *buffer=glyph->buffer;
	ppluint8 w;
	y-=glyph->bearingy;
	x+=glyph->bearingx;
	if (glyph->flags&FONTFLAGS::ANTIALIAS) {
		for (int yy=0;yy<glyph->height;yy++) {
			for (int xx=0;xx<glyph->width;xx++) {
				w=buffer[xx];
				if (w)	surface.blendPixel(x+xx,y+yy,c,(float)w/255);
			}
			buffer+=glyph->width;
		}
	} else {
		for (int yy=0;yy<glyph->height;yy++) {
			for (int xx=0;xx<glyph->width;xx++) {
				w=buffer[xx];
				if (w)	surface.putPixel(x+xx,y+yy,c);
			}
			buffer+=glyph->width;
		}
	}
	return 1;
}


void CFontGeneratorCommon::DeleteFace(int size, int flags)
{
	Reset();
	PFPChunk *c;
	char *b;
	int fflags;
	//printf ("DeleteFace, size=%i, flags=%i\n",size,flags);
	while ((c=(PFPChunk*)FindNextChunk("FACE"))) {
		b=(char*)c->Data();
		fflags=peek8(b)&7;
		//printf ("Face size=%i, flags=%i\n",peek16(b+2),fflags);
		if (fflags==flags && (int)peek16(b+2)==size) {
			DeleteChunk(c);
			Reset();
		}
	}
}

void CFontGeneratorCommon::CopyFreeTypeName()
{
	if (face) {
		SetName(face->family_name);
	}
}


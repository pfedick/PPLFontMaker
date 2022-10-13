#include <stdlib.h>
#include <string.h>
#include "fontmaker.h"
#include <locale.h>

/*
static char *demotext="FreeType 2 is a software font engine that is designed to\n\
be small, efficient, highly customizable and portable while\n\
capable of producing high-quality output (glyph images). It\n\
can be used in graphics libraries, display servers, font\n\
conversion tools, text image generation tools, and many\n\
other products as well..";
*/

int main (int argc, char **argv)
{
	Main m;
	setlocale(LC_ALL,"");
	return m.start(argc,argv);
}

Main::Main()
{
	fontVersion=5;
	font=NULL;
	flags=0;
	edit=NULL;
	quelle=NULL;
	target=NULL;
}

Main::~Main()
{

}

void Main::version()
{
	ppl6::CString s;
	s.Setf("PPL FONTMAKER Version %s vom %s",FM_VERSION,FM_RELEASEDATE);
	s.Print(true);
	s.Repeat("=",s.Len());
	s.Print(true);
	printf ("Author: %s\n",FM_AUTHOR);
	printf ("Copyright: %s\n",FM_COPYRIGHT);
}

void Main::help()
{
	version();
	printf ("\n"
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
			"  -5          PPL-Font Version 5 generieren (=Default)\n"
			"  -6          PPL-Font Version 6 generieren\n"
			"Flags:\n"
			"  --aa        Antialiased Fonts erstellen\n"
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
			"\n"
			"Sonstiges:\n"
			"  -h oder --help Zeigt diese Hilfe an\n"
			"  -l FILE     Zeigt den Inhalt der Font-Datei an\n"
			"  -ll FILE    Zeigt den Inhalt der Font-Datei inklusive Glyphs an\n"
			"  -e FILE     File editieren. Mit dieser Option kann Name, Author, etc.\n"
			"              und die Komprimierung geändert werden, sowie in Kombination\n"
			"              mit -r Fonts gelöscht werden.\n"
			"  -r          Fonts mit der angegebenen Größe und Flags werden gelöscht\n"
			"\n"
			"Enthält die Zieldatei bereits identische FACEs, werden diese überschrieben."
			"\n");
}

int Main::list(const char *filename, bool withglyphs)
{
	ppl6::PFPFile ff;
	if (!ff.Ident(filename)) {
		printf ("ERROR: Unbekanntes Dateiformat\n");
		return 1;
	}
	ppl6::CString Id=ff.GetID();
	if (Id!="FONT") {
		printf ("ERROR: Kein FONT-Format, sondern %s, Version %i.%i\n",ff.GetID(),ff.GetMainVersion(),ff.GetSubVersion());
		return 1;
	}
	if (ff.GetMainVersion()==5) {
		CFont5Generator font5;
		if (!font5.Load(quelle)) {
			ppl6::PrintError();
			return 1;
		}
		font5.List(withglyphs);
		return 0;
	} else if (ff.GetMainVersion()==6) {
		CFont6Generator font6;
		if (!font6.Load(quelle)) {
			ppl6::PrintError();
			return 1;
		}
		font6.List(withglyphs);
		return 0;
	}
	printf ("ERROR: Unbekanntes Font-Format: Version %i.%i",ff.GetMainVersion(),ff.GetSubVersion());
	return 1;
}

int Main::checkFlags(int argc, char **argv)
{
	flags=0;
	// Die Flags --aa, --mono1 und --mono8 können nicht gemeinsam verwendet werden
	if (ppl6::getargv(argc,argv,"--aa")) flags++;
	if (ppl6::getargv(argc,argv,"--mono1")) flags++;
	if (ppl6::getargv(argc,argv,"--mono8")) flags++;
	if (flags>1) {
		printf ("Die Flags \"--aa\", \"--mono1\" und \"--mono8\" können nicht gemeinsam verwendet werden\n");
		return 0;
	}

	flags=0;
	if (ppl6::getargv(argc,argv,"--aa2")) flags|=FONTFLAGS::ANTIALIAS|FONTFLAGS::AA2;
	else if (ppl6::getargv(argc,argv,"--aa4")) flags|=FONTFLAGS::ANTIALIAS|FONTFLAGS::AA4;
	else if (ppl6::getargv(argc,argv,"--aa")) flags|=FONTFLAGS::ANTIALIAS;
	else if (ppl6::getargv(argc,argv,"--mono8")) flags|=FONTFLAGS::MONO8;
	else flags|=FONTFLAGS::MONO1;
	if (ppl6::getargv(argc,argv,"--isbold")) flags|=FONTFLAGS::ISBOLD;
	if (ppl6::getargv(argc,argv,"--isitalic")) flags|=FONTFLAGS::ISITALIC;
	if (ppl6::getargv(argc,argv,"--genbold")) {
		if ((flags&FONTFLAGS::AA2)==0 && (flags&FONTFLAGS::AA4)==0 && (flags&FONTFLAGS::MONO1)==0) {
			flags|=FONTFLAGS::GENERATEBOLD;
		} else {
			printf ("ERROR: --genbold kann zur Zeit nicht zusammen mit --aa2, --aa4 oder --mono\nverwendet werden.\n");
			return 0;
		}
	}
	return 1;
}

int Main::getSizes(int argc, char **argv)
{
	const char *tmp;
	tmp=NULL;
	if ((!edit) && (!(tmp=ppl6::getargv(argc,argv,"-s")))) {
		printf ("Fontsize fehlt\n");
		return 0;
	}
	int start,end;
	ppl6::CString Size=tmp;
	ppl6::CString Tmp;

	if (Size.PregMatch("/^([0-9]+)\\-([0-9]+)$")) {		// Range
		start=ppl6::atoi(Size.GetMatch(1));
		end=ppl6::atoi(Size.GetMatch(2));
		if (end<start) {
			start=ppl6::atoi(Size.GetMatch(2));
			end=ppl6::atoi(Size.GetMatch(1));
		}
		for (int i=start;i<=end;i++) {
			Tmp.Setf("%i",i);
			Todo.Add(Tmp);
		}
	} else if (Size.Instr(",",0)>=0) {		// Liste
		Todo.Explode(tmp,",");
	} else {								// Einzeln
		start=ppl6::atoi(tmp);
		Tmp.Setf("%i",start);
		Todo.Add(Tmp);
	}
	if (Todo.Num()==0) {
		printf ("Keine Fontgroessen angegeben\n");
		return 0;
	}
	return 1;
}

int Main::getFiles(int argc, char **argv)
{
	edit=ppl6::getargv(argc,argv,"-e");
	quelle=ppl6::getargv(argc,argv,"-q");
	target=ppl6::getargv(argc,argv,"-t");
	if (edit) {
		target=edit;
	} else {
		if (!target) {
			printf ("Ziel fehlt\n");
			return 0;
		}
	}
	if (!edit) {
		if (!quelle) {
			printf ("Quelle fehlt\n");
			return 0;
		}
	}
	if (quelle) {
		// Läßt sich die Quelle öffnen?
		ppl6::CFile ff;
		if (!ff.Open(quelle,"rb")) {
			ppl6::PrintError();
			return 0;
		}
		ff.Close();
	}
	return 1;
}

int Main::getChars(int argc, char **argv)
{
	const char *tmp;
	// Unicode Bereich
	CArray CharsTodo;
	if ((tmp=ppl6::getargv(argc,argv,"-c"))) {
		int start,end;
		CharsTodo.Clear();
		CharsTodo.Explode(tmp,",");
		CharsTodo.Reset();
		while ((tmp=CharsTodo.GetNext())) {
			ppl6::CString Size=tmp;
			if (Size.PregMatch("/^([0-9]+)\\-([0-9]+)$/")) {		// Range
				start=ppl6::atoi(Size.GetMatch(1));
				end=ppl6::atoi(Size.GetMatch(2));
				if (end<start) {
					start=ppl6::atoi(Size.GetMatch(2));
					end=ppl6::atoi(Size.GetMatch(1));
				}
				font->AddCharRange(start,end);
			} else if (Size.PregMatch("/^([0-9]+)$/")) {		// Einzelnes Zeichen
				start=ppl6::atoi(Size.GetMatch(1));
				font->AddChar(start);
			} else {
				printf ("Unicode-Bereich wurde falsch angegeben\n");
				return 0;
			}
		}
	} else {
		font->AddCharRange(32,255);
		font->AddChar(8364);	// €
		font->AddChar(0x1E9E); // Grosses ß: ẞ
	}
	if (ppl6::getargv(argc,argv,"--idn")) {
		// Zusätzliche IDN-Zeichen:
		font->AddChar(263);	// 263: ć
		font->AddChar(265);	// 265: ĉ
		font->AddChar(267);	// 267: ċ
		font->AddChar(269);	// 269: č
		font->AddChar(271);	// 271: ď
		font->AddChar(273);	// đ  	273
		font->AddChar(275);	// ē  	275
		font->AddChar(277);	// ĕ  	277
		font->AddChar(279);	// ė  	279
		font->AddChar(281);	// ę  	281
		font->AddChar(283);	// ě  	283
		font->AddChar(285);	// ĝ  	285
		font->AddChar(287);	// ğ  	287
		font->AddChar(289);	// ġ  	289
		font->AddChar(291);	// ģ  	291
		font->AddChar(293);	// ĥ  	293
		font->AddChar(297);	// ĩ  	297
		font->AddChar(299);	// ī  	299
		font->AddChar(301);	// ĭ  	301
		font->AddChar(303);	// į  	303
		font->AddChar(305);	// ı  	305
		font->AddChar(309);	// ĵ  	309
		font->AddChar(311);	// ķ  	311
		font->AddChar(312);	// ĸ  	312
		font->AddChar(314);	// ĺ  	314
		font->AddChar(316);	// ļ  	316
		font->AddChar(318);	// ľ  	318
		font->AddChar(322);	// ł  	322
		font->AddChar(324);	// ń  	324
		font->AddChar(326);	// ņ  	326
		font->AddChar(328);	// ň  	328
		font->AddChar(331);	// ŋ  	331
		font->AddChar(333);	// ō  	333
		font->AddChar(335);	// ŏ  	335
		font->AddChar(337);	// ő  	337
		font->AddChar(339);	// œ  	339
		font->AddChar(341);	// ŕ  	341
		font->AddChar(343);	// ŗ  	343
		font->AddChar(345);	// ř  	345
		font->AddChar(347);	// ś  	347
		font->AddChar(349);	// ŝ  	349
		font->AddChar(351);	// ş  	351
		font->AddChar(353);	// š  	353
		font->AddChar(355);	// ţ  	355
		font->AddChar(357);	// ť  	357
		font->AddChar(359);	// ŧ  	359
		font->AddChar(361);	// ũ  	361
		font->AddChar(363);	// ū  	363
		font->AddChar(365);	// ŭ  	365
		font->AddChar(367);	// ů  	367
		font->AddChar(369);	// ű  	369
		font->AddChar(371);	// ų  	371
		font->AddChar(373);	// ŵ  	373
		font->AddChar(375);	// ŷ  	375
		font->AddChar(378);	// ź  	378
		font->AddChar(380);	// ż  	380
		font->AddChar(382);	// ž  	382
	}
	return 1;
}



int Main::start(int argc, char **argv)
{
	if(argc<2 || ppl6::getargv(argc,argv,"-h")!=NULL || ppl6::getargv(argc,argv,"--help")!=NULL ) {
		help();
		return 0;
	}
	if (ppl6::getargv(argc,argv,"-v")!=NULL || ppl6::getargv(argc,argv,"--version")!=NULL) {
		version();
		return 0;
	}
	// Fontversion
	if (ppl6::getargv(argc,argv,"-5")) fontVersion=5;
	if (ppl6::getargv(argc,argv,"-6")) fontVersion=6;

	// Dateiinhalt Listen?
	quelle=ppl6::getargv(argc,argv,"-ll");
	if (quelle) return list(quelle,true);
	quelle=ppl6::getargv(argc,argv,"-l");
	if (quelle) return list(quelle,false);

	if (!checkFlags(argc, argv)) return 1;
	if (!getFiles(argc, argv)) return 1;
	if (!getSizes(argc, argv)) return 1;

	if (fontVersion==5) font=new CFont5Generator;
	else font=new CFont6Generator;

	if (!getChars(argc, argv)) return 1;

	if (!work(argc, argv)) {
		delete font;
		return 1;
	}
	delete font;
	return 0;

}

int Main::work(int argc, char **argv)
{
	const char *tmp;
	// Das Quellfile wird geladen, wenn es existiert
	if (!font->Load(target)) {
		if (ppl6::GetErrorCode()==435) {
			printf ("Zieldatei existiert und ist keine Font-Datei!\n");
			return 0;
		}
	}
	if ((tmp=ppl6::getargv(argc,argv,"--name"))) {
		if (!font->SetName(tmp)) {
			ppl6::PrintError();
			return 0;
		}
	}
	if ((tmp=ppl6::getargv(argc,argv,"--author"))) {
		if (!font->SetAuthor(tmp)) {
			ppl6::PrintError();
			return 0;
		}
	}
	if ((tmp=ppl6::getargv(argc,argv,"--copy"))) {
		if (!font->SetCopyright(tmp)) {
			ppl6::PrintError();
			return ppl6::GetErrorCode();
		}
	}
	if (ppl6::getargv(argc,argv,"--zlib")) font->SetCompression(ppl6::CCompression::Algo_ZLIB);
	else if (ppl6::getargv(argc,argv,"--bzip2")) font->SetCompression(ppl6::CCompression::Algo_BZIP2);
	else font->SetCompression(ppl6::CCompression::Algo_NONE);


	if (quelle) {
		if (!font->LoadFont(quelle)) {
			ppl6::PrintError();
			return 0;
		}
		// Hat unser Font einen Namen?
		const char *name=font->GetName();
		if ((!name)  || strlen(name)<2) {
			font->CopyFreeTypeName();
		}
	}

	if (edit) {
		// FACEs löschen?
		if (ppl6::getargv(argc,argv,"-r")) {
			Todo.Reset();
			while ((tmp=Todo.GetNext())) {
				font->DeleteFace(ppl6::atoi(tmp),flags);
			}
		}
	} else {
		while ((tmp=Todo.GetNext())) {
			if (!font->Generate(ppl6::atoi(tmp),flags)) {
				ppl6::PrintError();
				return 0;
			}
		}
	}
	if (!font->Save(target)) {
		ppl6::PrintError();
		return 0;
	}
	font->List();
	return 1;
}





/*
UINT GetWindowsDirectory(
  LPTSTR lpBuffer,
  UINT uSize
);


*/

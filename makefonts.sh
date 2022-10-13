#!/bin/sh
SYSTEM=`uname -s`
FORMAT=-6
SUFFIX=fnt6
ANTIALIASING=--aa4

XORG_FONTPATH=/usr/local/lib/X11/fonts


genfontfile() {
	rm -f $FILE
	pplfontmaker -t $FILE -q $FONTPATH/$REGULAR -s $SIZE --idn --mono1 -n $NAME --zlib $FORMAT
	if [ $? -ne 0 ] ;then exit; fi
	pplfontmaker -t $FILE -q $FONTPATH/$REGULAR -s $SIZE --idn $ANTIALIASING -n $NAME --zlib $FORMAT
	if [ $? -ne 0 ] ;then exit; fi
	pplfontmaker -t $FILE -q $FONTPATH/$BOLD -s $SIZE --idn --mono1 --isbold -n $NAME --zlib $FORMAT
	if [ $? -ne 0 ] ;then exit; fi
	pplfontmaker -t $FILE -q $FONTPATH/$BOLD -s $SIZE --idn $ANTIALIASING --isbold -n $NAME --zlib $FORMAT
	if [ $? -ne 0 ] ;then exit; fi
	pplfontmaker -t $FILE -q $FONTPATH/$ITALIC -s $SIZE --idn --mono1 --isitalic -n $NAME --zlib $FORMAT
	if [ $? -ne 0 ] ;then exit; fi
	pplfontmaker -t $FILE -q $FONTPATH/$ITALIC -s $SIZE --idn $ANTIALIASING --isitalic -n $NAME --zlib $FORMAT
	if [ $? -ne 0 ] ;then exit; fi
	pplfontmaker -t $FILE -q $FONTPATH/$BOLDITALIC -s $SIZE --idn --mono1 --isbold --isitalic -n $NAME --zlib $FORMAT
	if [ $? -ne 0 ] ;then exit; fi
	pplfontmaker -t $FILE -q $FONTPATH/$BOLDITALIC -s $SIZE --idn $ANTIALIASING --isbold --isitalic -n $NAME --zlib $FORMAT
	if [ $? -ne 0 ] ;then exit; fi
}

genmonospace() {
	rm -f $FILE
	pplfontmaker -t $FILE -q $FONTPATH/$REGULAR -s $SIZE --idn --mono1 -n $NAME --zlib $FORMAT
	if [ $? -ne 0 ] ;then exit; fi
	pplfontmaker -t $FILE -q $FONTPATH/$REGULAR -s $SIZE --idn $ANTIALIASING -n $NAME --zlib $FORMAT
	if [ $? -ne 0 ] ;then exit; fi
	pplfontmaker -t $FILE -q $FONTPATH/$BOLD -s $SIZE --idn --mono1 --isbold -n $NAME --zlib $FORMAT
	if [ $? -ne 0 ] ;then exit; fi
	pplfontmaker -t $FILE -q $FONTPATH/$BOLD -s $SIZE --idn $ANTIALIASING --isbold -n $NAME --zlib $FORMAT
	if [ $? -ne 0 ] ;then exit; fi
}

# Windows Vista/7 SegoeUI

SIZE="8,10,12,14,16,18,20"
FONTPATH=~/fonts/Windows7
REGULAR=segoeui.ttf
BOLD=segoeuib.ttf
ITALIC=segoeuii.ttf
BOLDITALIC=segoeuiz.ttf
NAME="SegoeUI"
FILE=segoeui2.$SUFFIX
ANTIALIASING=--aa2
genfontfile
FILE=segoeui4.$SUFFIX
ANTIALIASING=--aa4
genfontfile
FILE=segoeui8.$SUFFIX
ANTIALIASING=--aa8
genfontfile

###################################################################################################
# FreeSans
SIZE="8,10,12,14,16,18,20"
FONTPATH=/usr/share/fonts/truetype/freefont
if [ "$SYSTEM" = "FreeBSD" ] ; then FONTPATH="/usr/local/lib/X11/fonts/freefont-ttf"; fi
REGULAR=FreeSans.ttf
BOLD=FreeSansBold.ttf
ITALIC=FreeSansOblique.ttf
BOLDITALIC=FreeSansBoldOblique.ttf
NAME="FreeSans"
FILE=freesans4.$SUFFIX
ANTIALIASING=--aa4
genfontfile

ANTIALIASING=--aa8
FILE=freesans8.$SUFFIX
genfontfile

ANTIALIASING=--aa2
FILE=freesans2.$SUFFIX
genfontfile

# FreeSerif
REGULAR=FreeSerif.ttf
BOLD=FreeSerifBold.ttf
ITALIC=FreeSerifItalic.ttf
BOLDITALIC=FreeSerifBoldItalic.ttf
NAME="FreeSerif"
FILE=freeserif4.$SUFFIX
ANTIALIASING=--aa4
genfontfile
FILE=freeserif2.$SUFFIX
ANTIALIASING=--aa2
genfontfile

# FreeMono
REGULAR=FreeMono.ttf
BOLD=FreeMonoBold.ttf
NAME="FreeMono"
FILE=freemono4.$SUFFIX
ANTIALIASING=--aa4
genmonospace
FILE=freemono2.$SUFFIX
ANTIALIASING=--aa2
genmonospace


###################################################################################################
# DejaVu

# DejaVu Sans
SIZE="8,10,12,14,16,18,20"
FONTPATH=/usr/share/fonts/truetype/ttf-dejavu
if [ "$SYSTEM" = "FreeBSD" ] ; then FONTPATH="/usr/local/lib/X11/fonts/dejavu"; fi
REGULAR=DejaVuSans.ttf
BOLD=DejaVuSans-Bold.ttf
ITALIC=DejaVuSans-Oblique.ttf
BOLDITALIC=DejaVuSans-BoldOblique.ttf
NAME="DejaVu Sans"
FILE=dejavusans2.$SUFFIX
ANTIALIASING=--aa2
genfontfile
ANTIALIASING=--aa4
FILE=dejavusans4.$SUFFIX
genfontfile

# DejaVu Serif
REGULAR=DejaVuSerif.ttf
BOLD=DejaVuSerif-Bold.ttf
ITALIC=DejaVuSerif-Italic.ttf
BOLDITALIC=DejaVuSerif-BoldItalic.ttf
NAME="DejaVu Serif"
FILE=dejavuserif2.$SUFFIX
ANTIALIASING=--aa2
genfontfile

FILE=dejavuserif4.$SUFFIX
ANTIALIASING=--aa4
genfontfile

# DejaVu Sans Mono
REGULAR=DejaVuSansMono.ttf
BOLD=DejaVuSansMono-Bold.ttf
NAME="DejaVu Sans Mono"
FILE=dejavusansmono2.$SUFFIX
ANTIALIASING=--aa2
genmonospace

FILE=dejavusansmono4.$SUFFIX
ANTIALIASING=--aa4
genmonospace

###################################################################################################
# Liberation Sans
SIZE="8,10,12,14,16,18,20"
FONTPATH=/usr/share/fonts/truetype/ttf-liberation
if [ "$SYSTEM" = "FreeBSD" ] ; then FONTPATH="/usr/local/lib/X11/fonts/Liberation"; fi
REGULAR=LiberationSans-Regular.ttf
BOLD=LiberationSans-Bold.ttf
ITALIC=LiberationSans-Italic.ttf
BOLDITALIC=LiberationSans-BoldItalic.ttf
NAME="Liberation Sans"
FILE=liberationsans2.$SUFFIX
ANTIALIASING=--aa2
genfontfile
FILE=liberationsans4.$SUFFIX
ANTIALIASING=--aa4
genfontfile

# Liberation Serif
REGULAR=LiberationSerif-Regular.ttf
BOLD=LiberationSerif-Bold.ttf
ITALIC=LiberationSerif-Italic.ttf
BOLDITALIC=LiberationSerif-BoldItalic.ttf
NAME="Liberation Serif"
FILE=liberationserif2.$SUFFIX
ANTIALIASING=--aa2
genfontfile

FILE=liberationserif4.$SUFFIX
ANTIALIASING=--aa4
genfontfile


# Liberation Mono
# Monospace-Fonts, ohne Italic
SIZE="8,10,12,14,16,18,20"
FONTPATH=/usr/share/fonts/truetype/ttf-liberation
if [ "$SYSTEM" = "FreeBSD" ] ; then FONTPATH="/usr/local/lib/X11/fonts/Liberation"; fi
REGULAR=LiberationMono-Regular.ttf
BOLD=LiberationMono-Bold.ttf
ITALIC=LiberationMono-Italic.ttf
BOLDITALIC=LiberationMono-BoldItalic.ttf
NAME="Liberation Mono"
FILE=liberationmono2.$SUFFIX
ANTIALIASING=--aa2
genmonospace

FILE=liberationmono4.$SUFFIX
ANTIALIASING=--aa4
genmonospace


###################################################################################################
# Ubuntu
SIZE="8,10,12,14,16,18,20"
FONTPATH=/usr/share/fonts/truetype/ttf-ubuntu-font
if [ "$SYSTEM" = "FreeBSD" ] ; then FONTPATH="/usr/local/lib/X11/fonts/ubuntu-font"; fi
REGULAR=Ubuntu-R.ttf
BOLD=Ubuntu-B.ttf
ITALIC=Ubuntu-RI.ttf
BOLDITALIC=Ubuntu-BI.ttf
NAME="Ubuntu"
FILE=ubuntu2.$SUFFIX
ANTIALIASING=--aa2
genfontfile
FILE=ubuntu4.$SUFFIX
ANTIALIASING=--aa4
genfontfile

###################################################################################################
# Ubuntu-Mono
SIZE="8,10,12,14,16,18,20"
FONTPATH=/usr/share/fonts/truetype/ttf-ubuntu-font
if [ "$SYSTEM" = "FreeBSD" ] ; then FONTPATH="/usr/local/lib/X11/fonts/ubuntu-font"; fi
REGULAR=UbuntuMono-R.ttf
BOLD=UbuntuMono-B.ttf
ITALIC=UbuntuMono-RI.ttf
BOLDITALIC=UbuntuMono-BI.ttf
NAME="Ubuntu Mono"
FILE=ubuntumono2.$SUFFIX
ANTIALIASING=--aa2
genfontfile
FILE=ubuntumono4.$SUFFIX
ANTIALIASING=--aa4
genfontfile


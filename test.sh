#!/usr/bin/bash

if [ -d /c/Users/patrick/appdata/local/microsoft/windows/fonts ]
then
    FONTDIR="/c/Users/patrick/appdata/local/microsoft/windows/fonts"
    rm -rf "sourcecode.fnt6"
    ./pplfontmaker -t sourcecode.fnt6 -q "$FONTDIR/SourceCodePro-Regular.ttf" --idn -s8 -6 -mono1
    #./pplfontmaker -t sourcecode.fnt6 -q "$FONTDIR/SourceCodePro-Regular.ttf" --idn -s8,10,12,14,16,18,20,22 -6 -mono1
    #./pplfontmaker -t sourcecode.fnt6 -q "$FONTDIR/SourceCodePro-Bold.ttf" --idn --isbold -s8,10,12,14,16,18,20,22 -6 -mono1

fi

exit 0
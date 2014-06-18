#!/bin/bash

DST=../guicast/images
SRC=guicast/images
CMP=bin2cinecutie_header-1.2
PGM=bin2cinecutie_header
INP=$CMP/bin2cinecutie_header.c
gcc $INP -o $PGM
#exit 0
#mkdir -p $TMP

for file in `ls $SRC`
	do
	TGT=$(basename $file .svg)
	if [ -e $DST/$TGT.png ]
		then
		
		inkscape -e $DST/$TGT.png $SRC/$TGT.svg 
		./$PGM $DST/$TGT.png $DST/$TGT"_png.h"	
			
		piconup=$(echo $TGT"_png_h" | tr '[:lower:]' '[:upper:]')
		
		var=$TGT"_png"
		
		sed -i s/PICON_PNG_H/$piconup/g $DST/$TGT"_png.h"
		
		sed -i s/picon_png/$var/g $DST/$TGT"_png.h"

		fi
done

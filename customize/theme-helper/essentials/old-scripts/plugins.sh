#!/bin/bash

DST=../plugins
SRC=plugins
CMP=bin2cinecutie_header-1.2
PGM=bin2cinecutie_header
INP=$CMP/bin2cinecutie_header.c
gcc $INP -o $PGM
#exit 0
#mkdir -p $TMP

for i in `ls $SRC`
	do
	TGT=$(basename $i .svg)
	if [ -d $DST/$TGT ]
		then
		inkscape -w 52 -h 52 -e $DST/$TGT/picon.png $SRC/$i 
		./$PGM $DST/$TGT/picon.png $DST/$TGT/picon_png.h
		fi
done

	

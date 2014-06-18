#!/bin/bash
# This Script update all icons on cinelerra to fit tango guideline
# All icons can changed easyly by editing svg on cinelerra guicast and plugins folder 
if [ "$(dirname $0)" = "." ]
then 
BASE=`pwd`
else
BASE=$(dirname $0)
fi

# Cinelerra source
CINSRC="$BASE/../../"
# Script source
SCRSRC="$BASE"
# Data Folder
DATASRC=$SCRSRC/global
# Backup Folder
BACKUP=$SCRSRC/backup

# check for inkscape
if ! which inkscape &> /dev/null
 	then
	echo "This Script require inkscape! Install it first"
	exit 0
fi

# check for gcc
if ! which gcc &> /dev/null
 	then
	echo "This Script require gcc! Install it first"
	exit 0
fi

# check for cinelerra
if ! cat $BASE/../../configure.ac | grep -i Cinelerra &> /dev/null
 	then
	echo "This Script must be unpacked and launched from cinelerraroot/cinelerra-theme-helper!"
	exit 0
fi

# build program that convert png to bin
CMP=$SCRSRC/essentials
PGM=$SCRSRC/bin2cinelerra_header
rm $PGM
INP=$CMP/bin2cinelerra_header.c
gcc $INP -o $PGM

# Update data icons on cinelerra
SOURCE=cinelerra/data
SRC=$DATASRC/$SOURCE
DST=$CINSRC/$SOURCE
BCK=$BACKUP/$SOURCE
[ ! -e $BCK ] && mkdir -p $BCK
# build redo script
echo "!#/bin/bash" > $BACKUP/redo.sh
echo cp -a $BACKUP/* $CINSRC/../ >> $BACKUP/redo.sh
chmod 755 $BACKUP/redo.sh
# Search svg made on script dir and convert it to _png.h (with a full backup)
for file in `ls $SRC | grep svg`
	do
	TGT=$(basename $file .svg)
	if [ -e $DST/$TGT.png ]
		then
		# backup first old icon
		[ ! -e $BCK/$TGT.png ] && cp $DST/$TGT.png $BCK/$TGT.png
		
		# convert svg to png with inkscape
		inkscape -e $DST/$TGT.png $SRC/$TGT.svg
		
		# backup first old _png.h
		[ ! -e $BCK/$TGT"_png.h" ] && cp $DST/$TGT"_png.h" $BCK/$TGT.png

		# convert png to _png.h		
		./$PGM $DST/$TGT.png $DST/$TGT"_png.h"	
		# _png_h needs some fixes to work	
		piconup=$(echo $TGT"_png_h" | tr '[:lower:]' '[:upper:]')
		var=$TGT"_png"
		sed -i s/PICON_PNG_H/$piconup/g $DST/$TGT"_png.h"
		sed -i s/picon_png/$var/g $DST/$TGT"_png.h"
		fi
done

# Update data icons on guicast
SOURCE=guicast/images
SRC=$DATASRC/$SOURCE
DST=$CINSRC/$SOURCE
BCK=$BACKUP/$SOURCE
[ ! -e $BCK ] && mkdir -p $BCK
for file in `ls $SRC`
	do
	TGT=$(basename $file .svg)
	if [ -e $DST/$TGT.png ]
		then
		# backup first old icon
		[ ! -e $BCK/$TGT.png ] && cp $DST/$TGT.png $BCK/$TGT.png
		
		# convert svg to png with inkscape
		inkscape -e $DST/$TGT.png $SRC/$TGT.svg
		
		# backup first old _png.h
		[ ! -e $BCK/$TGT"_png.h" ] && cp $DST/$TGT"_png.h" $BCK/$TGT.png

		# convert png to _png.h		
		$PGM $DST/$TGT.png $DST/$TGT"_png.h"	
		# _png_h needs some fixes to work	
		piconup=$(echo $TGT"_png_h" | tr '[:lower:]' '[:upper:]')
		var=$TGT"_png"
		sed -i s/PICON_PNG_H/$piconup/g $DST/$TGT"_png.h"
		sed -i s/picon_png/$var/g $DST/$TGT"_png.h"
		fi
done

# Update icons on all plugins
SOURCE=plugins
SRC=$DATASRC/$SOURCE
DST=$CINSRC/$SOURCE
BCK=$BACKUP/$SOURCE
for i in `ls $SRC`
	do
	TGT=$(basename $i .svg)
	if [ -d $DST/$TGT ]
		then
		inkscape -w 52 -h 52 -e $DST/$TGT/picon.png $SRC/$i 
		$PGM $DST/$TGT/picon.png $DST/$TGT/picon_png.h
		fi
done



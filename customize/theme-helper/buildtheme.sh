#!/bin/bash
# This script build theme from svg source, this script require a theme model
# All files with suffix .btheme will be used to build theme with its own name
# allow also to change colors to all svg files and theme.
# This script is under Gnu Public License v2. Paolo Rampino.

# set actual script folder
if [ "$(dirname $0)" = "." ]
	then
	BASE=`pwd`
	else
	BASE=$(dirname $0)
fi

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

#Set Log file and starting LOG
LOG="$BASE"/log.txt
echo BUILDING... > "$LOG"

#Set Global var
BACKUP="$BASE"/backup/plugins
DST="$BASE"/build
SRC="$BASE"/themes

# Inizialize backup folder
[ ! -e $BACKUP ] && mkdir -p $BACKUP
[ ! -e $DST ] && mkdir -p $DST
[ ! -e $SRC ] && mkdir -p $SRC


# Read all .btheme  from themes folder
for T in `ls "$SRC" | grep \.btheme`;
		do
		#Read VARS from .btheme files
		source "$SRC"/$T
		#Back to base folder
		cd "$BASE"
		#Remove previous builds
		rm -rf "$SRC"/$THEME
		#Copy theme model
		cp -a "$BASE"/$MODEL "$SRC"/$THEME
		#Start Theme building
		cd "$SRC"/$THEME
		#Some rename and basecolors change
		for i in THEME.C THEME.h Makefile.am data/Makefile.am;
			do
			sed -i s/THEMENAME/"$THEMENAME"/g $i
			sed -i s/THEME/$THEME/g $i
			sed -i s/THFUNC/$THFUNC/g $i
			sed -i s/BGCOLOR/"0x$BGCOLOR"/g $i
			sed -i s/BBCOLOR/"0x$BBCOLOR"/g $i
			sed -i s/TEXT/"0x$TEXT"/g $i
		done
		mv THEME.C $THEME.C
		mv THEME.h $THEME.h
		mkdir data/Source
		# Compile pngs files from svg
		cd data
		for i in `ls | grep svg`
			do
			# recolorize svg, if it needs
			[ "$INBGCOLOR" != "$BGCOLOR" ] && sed -i /\#/s/$INBGCOLOR/$BGCOLOR/g $i
			[ "$INBBCOLOR" != "$BBCOLOR" ] && sed -i /\#/s/$INBBCOLOR/$BBCOLOR/g $i
			[ "$INTEXT" != "$TEXT" ] && sed -i /\#/s/$INTEXT/$TEXT/g $i
			[ "$INICON" != "$OUTICON" ] && sed -i /\#/s/$INICON/$OUTICON/g $i
			[ "$INCHECKED" != "$OUTCHECKED" ] && sed -i /\#/s/$INCHECKED/$OUTCHECKED/g $i
			[ "$INBORDER" != "$OUTBORDER" ] && sed -i /\#/s/$INBORDER/$OUTBORDER/g $i
			[ "$INMISC" != "$OUTMISC" ] && sed -i /\#/s/$INMISC/$OUTMISC/g $i
			[ "$INDBUTTON" != "$OUTDBUTTON" ] && sed -i /\#/s/$INDBUTTON/$OUTDBUTTON/g $i
			[ "$INBASE" != "$OUTBASE" ] && sed -i /\#/s/$INBASE/$OUTBASE/g $i
			#Inkscape SVG > PNG
			inkscape -e $(basename $i .svg).png $i
			mv $i Source
		done
		[ -e "$DST"/$THEME ] && rm -rf "$DST"/$THEME
		# Install
		mv "$SRC"/$THEME $DST/

echo "the new themes are on $DST, you should copy on plugins and add theme on configure.ac and plugins/Makefile.am"
done



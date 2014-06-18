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

# check for cinelerra
if ! cat $BASE/../../configure.ac | grep -i Cinelerra &> /dev/null
 	then
	echo "This Script must be unpacked and launched from cinelerraroot/cinelerra-theme-helper!"
	exit 0
fi

#Set Log file and starting LOG
LOG="$BASE"/log.txt
echo BUILDING... > "$LOG"

#Set Global var
BACKUP="$BASE"/backup/plugins
DST="$BASE"../../plugins
SRC="$BASE"/themes

# Inizialize backup folder
[ ! -e $BACKUP ] && mkdir -p $BACKUP

# Read all .btheme  from themes folder
for T in `ls "$BASE"/themes | grep \.btheme`;
		do
		#Read VARS from .btheme files
		source "$BASE"/themes/$T
		#Back to base folder
		cd "$BASE"
		#Remove previous builds
		rm -rf "$BASE"/themes/$THEME
		#Copy theme model
		cp -a "$BASE"/$MODEL "$BASE"/themes/$THEME
		#Start Theme building
		cd "$BASE"/themes/$THEME
		#Some rename and basecolors change
		for i in THEME.C THEME.h Makefile.am data/Makefile.am;
			do
			sed -i s/THEMENAME/$THEMENAME/g $i
			sed -i s/THEME/$THEME/g $i
			sed -i s/THFUNC/$THFUNC/g $i
			sed -i s/BGCOLOR/"0x$BGCOLOR"/g $i
			sed -i s/BBCOLOR/"0x$BBCOLOR"/g $i
			sed -i s/TEXT/"0x$TEXT"/g $i
		done
		mv THEME.C $THEME.C
		mv THEME.h $THEME.h
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
		done
		#Backup old theme
		if [ -e $BACKUP/$THEME ]
			then
			echo $THEME Already Backuped! >> "$LOG"
			else
			mv $BASE/../plugins/$THEME $BACKUP/
		fi
		[ -e $BASE/../plugins/$THEME ] && rm -rf $BASE/../plugins/$THEME
		# Install
		cp -a "$BASE"/themes/$THEME $DST/
		#IF theme not exists on configure.in then add it
		if ! cat $BASE/../configure.in | grep $THEME &> /dev/null
			then
			sed -i /"suv\/data"/s/Makefile/"Makefile "\\\\"\\n\tplugins\/$THEME\/Makefile"\\\\"\\n\tplugins\/$THEME\/data\/Makefile "/ $BASE/../configure.in

		fi
		#IF theme not exists on plugins Makefile.am then add it
		if ! cat $BASE/../plugins/Makefile.am | grep $THEME  &> /dev/null
			then
			sed -i s/suv/"suv "\\\\"\\n\t$THEME"/ $BASE/../plugins/Makefile.am
		fi

echo "building and installing $THEME done." >> "$LOG"
done
# Finish
echo Log is on $LOG

cat "$LOG"

cd $BASE/..
# this rebuild all makefiles
./autogen.sh

cd $BASE



#!/bin/bash
SAMPLE=sample.svg
plugins=/home/user/Desktop/cutiehd/cinecutiecv-2.1.1.orig/plugins

filein=$(cat ../plugins/Makefile.am  | grep -v \= | sed -e s/\\\\// | grep -v THEMES | sed -e s/" "//g | sed -e s/$'\t'//g | grep -v fonts | grep -v theme | grep -v suv)

for i in $filein
	do 
	if [ ! -e plugins/$i.svg ]
		then
		if [ -e ../plugins/$i/picon_png.h ]
			then
			cp $SAMPLE plugins/$i.svg
	        	sed -i s/DIRNAMETOCHANGE/$i/ plugins/$i.svg
	        fi
	fi
done


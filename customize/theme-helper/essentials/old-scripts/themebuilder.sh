#!/bin/bash

DST=../plugins
SRC=themes


for i in `ls $SRC`
	do
	TGT=$i/data
	rm -rf $DST/$i/x86_64
	if [ -z $1 ]; then
	for svg in `ls $SRC/$TGT | grep \\.svg`
		do
		if [ -d $DST/$TGT ]
			then
			inkscape -e $DST/$TGT/$(basename $svg .svg).png $SRC/$TGT/$svg 
		fi
	done
	else
	for e in $@
	do
	for svg in `ls $SRC/$TGT | grep \\.svg | grep $e`
		do
		if [ -d $DST/$TGT ]
			then
			inkscape -e $DST/$TGT/$(basename $svg .svg).png $SRC/$TGT/$svg 
		fi
	done
	done
	fi
make -C $DST/$i clean
make -C $DST/$i 
sudo make -C $DST/$i install
done



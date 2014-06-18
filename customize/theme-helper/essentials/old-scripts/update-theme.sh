#!/bin/bash

filein=$(cat ../plugins/suv/suv.* | grep \\.png | grep -v \\/\\/ | sed -e s/\"/\\n/g | grep \\.png | sort -u)
theme=suv
for o in $filein
	do 
	i=$(basename "$o" .png)
	if [ ! -e themes/$theme/data/$i.svg ]
		then
		cp ../plugins/$theme/data/$i.png themes/$theme/data/$i.png
	fi
done


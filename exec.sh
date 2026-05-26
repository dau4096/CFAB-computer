#!/bin/bash

clear

fileName=$1
if [ $# -gt 0 ]; then
	cd tools
	python3 fabricator.py $fileName.cfab $fileName.dat
	cd ..
else
	echo "No source CFAB file supplied."
	exit -1
fi
shift 1

./prgm.x86_64 $@ -f $fileName.dat

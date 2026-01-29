#!/bin/bash

clear
if [ $# -gt 0 ]; then
	cd tools
	python3 fabricator.py $1.cfab $1.dat
	cd ..
else
	echo "No source CFAB file supplied."
	exit -1
fi

if [ $# -gt 1 ]; then
	./app $2 -f $1.dat
else
	./app -f $1.dat
fi

#!/bin/bash

clear
if [ $# -gt 0 ]; then
	python3 fabricator.py $1 $1
else
	echo "No source CFAB file supplied."
	exit -1
fi

./app -f data/$1.dat
#!/bin/bash

clear
if [ $# -gt 0 ]; then
	python3 fabricator.py $1.cfab $1.dat
else
	echo "No source CFAB file supplied."
	exit -1
fi

./app -f $1.dat
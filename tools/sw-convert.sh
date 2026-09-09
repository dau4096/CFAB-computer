#!/bin/bash

fileName=$1
if [ $# -gt 0 ]; then
	echo "Fabricating.."
	python3 fabricator.py $fileName.cfab $fileName.dat
else
	echo "No source CFAB file supplied."
	exit -1
fi

echo "Re-encoding.."
python3 sw-reencode.py $fileName
echo "Done"

#!/bin/bash

clear

find ../cfab -maxdepth 1 -type f -name "*.cfab" -printf "%f\n" |
while IFS= read -r file; do
	if [[ "$file" != "test.cfab" ]]; then
	    python3 fabricator.py "$file"
	fi
done
#!/bin/sh

grep -v ";" k | sed "s/dd //" | awk '!/^$/ { print toupper($1); }' > .ops

let a=0
for s in `cat .ops`; do
	echo "	$s = $a<<24,"
	let a=$a+1
done

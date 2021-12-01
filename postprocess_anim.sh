#!/bin/bash -x

for dir in `ls -d */`
do
	convert -delay 20 -loop 0 $dir*.png ${dir::-1}.gif
done

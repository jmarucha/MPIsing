#!/bin/bash

for i in {1..48}
do
	mpiexec -n $i ./main
done

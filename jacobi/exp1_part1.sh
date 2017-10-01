#!/bin/bash
# Repeats, Grid size, Num Stages, (Num workers, Num iterations, Set-pin bool, Num cores)

NUM_RUNS=32


STEP=$(bc -l <<< "100 / $NUM_RUNS")
TOTAL=$STEP

printf "0.000%%"

for ((i=1; i<=$NUM_RUNS; i++))
do 
	bin/jacobi 9 256 1   $i 5000 1 32 >> /dev/null
	printf "\r%.3f%%" "$TOTAL"
	TOTAL=$(bc -l <<< "$TOTAL + $STEP")
done
#!/bin/bash
# Num runs, Grid size, Num Stages, (Num workers, Num iterations, Set-pin bool, Num cores)

NUM_RUNS=32


STEP=$(bc -l <<< "100 / $NUM_RUNS")
TOTAL=$STEP

printf "0.000%%"
#scripts/update-motd.sh "Experiments_running!_0.000%_complete_-Mark_Jenkins_(s1309061)"

for ((i=1; i<=$NUM_RUNS; i++))
do 
	head -n -1 configs/exp1.1_config.ini > temp.ini ; echo "num_workers_0: \"$i\"" >> temp.ini ; mv temp.ini configs/exp1.1_config.ini
	bin/jacobi configs/exp1.1_config.ini >> /dev/null
	printf "\r%.3f%%" "$TOTAL"
#        scripts/update-motd.sh "Experiments_running!_$TOTAL%_complete_-Mark_Jenkins_(s1309061)"
	TOTAL=$(bc -l <<< "$TOTAL + $STEP")
done

#!/bin/bash
# Num runs, Grid size, Num Stages, (Num workers, Num iterations, Set-pin bool, Num cores)

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT

function ctrl_c() {
        echo "Trapped CTRL-C, resetting MOTD..."
        echo 
        sudo scripts/update-motd.sh ""
        exit
}



POWERS_MIN=10
POWERS_MAX=16

NUM_WORKERS_MIN=2
NUM_WORKERS_MAX=32
NUM_WORKERS_STEP=2

# NUM_RUNS=($POWERS_MAX - $POWERS_MIN + 1) * (((NUM_WORKERS_MAX - NUM_WORKERS_MIN) / NUM_WORKERS_STEP) + 1)



NUM_RUNS=112

STEP=$(bc -l <<< "100 / $NUM_RUNS")
TOTAL=$STEP

printf "0.000%%"
sudo scripts/update-motd.sh "Experiments running! 0.000% complete -Mark Jenkins (s1309061)" >> /dev/null

for ((i=$POWERS_MIN; i<=$POWERS_MAX; i++))
do 
	head -n -2 configs/exp1.2.ini > temp.ini ; echo "grid_size: \"$((2**$i))\"" >> temp.ini ; echo "num_workers_0: \"1\"" >> temp.ini ; mv temp.ini configs/exp1.2.ini

	for ((j=$NUM_WORKERS_MIN; j<=$NUM_WORKERS_MAX; j+=NUM_WORKERS_STEP))
	do
		head -n -1 configs/exp1.2.ini > temp.ini ; echo "num_workers_0: \"$j\"" >> temp.ini ; mv temp.ini configs/exp1.2.ini
		bin/jacobi configs/exp1.2.ini >> /dev/null
		printf "\r%.3f%%" "$TOTAL"
	        sudo scripts/update-motd.sh "$(printf "Experiments running! %.3f%% complete -Mark Jenkins (s1309061)" "$TOTAL")" >> /dev/null
		TOTAL=$(bc -l <<< "$TOTAL + $STEP")
	done
done

sudo scripts/update-motd.sh "" >> /dev/null

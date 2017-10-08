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



NUM_RUNS=4



STEP=$(bc -l <<< "100 / $NUM_RUNS")
TOTAL=$STEP

printf "0.000%%"
sudo scripts/update-motd.sh "Experiments running! 0.000% complete -Mark Jenkins (s1309061)" >> /dev/null

for ((i=1; i<=$NUM_RUNS; i++))
do 
	head -n -1 configs/exp1.1_config.ini > temp.ini ; echo "num_workers_0: \"$i\"" >> temp.ini ; mv temp.ini configs/exp1.1_config.ini
	bin/jacobi configs/exp1.1_config.ini >> /dev/null
	printf "\r%.3f%%" "$TOTAL"
        sudo scripts/update-motd.sh "$(printf "Experiments running! %.3f%% complete -Mark Jenkins (s1309061)" "$TOTAL")" >> /dev/null
	TOTAL=$(bc -l <<< "$TOTAL + $STEP")
done

sudo scripts/update-motd.sh "" >> /dev/null

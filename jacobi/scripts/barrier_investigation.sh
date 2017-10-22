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
POWERS_MAX=15

# NUM_RUNS=($POWERS_MAX - $POWERS_MIN + 1) * (((NUM_WORKERS_MAX - NUM_WORKERS_MIN) / NUM_WORKERS_STEP) + 1)

NUM_RUNS=42

STEP=$(bc -l <<< "100 / $NUM_RUNS")
TOTAL=$STEP

printf "0.000%%"
sudo scripts/update-motd.sh "Experiments running! 0.000% complete -Mark Jenkins (s1309061)" >> /dev/null


make clean
make flags="-DDETAILED_METRICS -DMYBARRIER" main


for ((i=$POWERS_MIN; i<=$POWERS_MAX; i++))
do 
	head -n -1 configs/convergence_test_investigation.ini > temp.ini ; echo "grid_size: \"$((2**$i))\"" >> temp.ini ; echo "num_workers_0: \"1\"" >> temp.ini ; mv temp.ini configs/convergence_test_investigation.ini

	bin/jacobi configs/convergence_test_investigation.ini >> /dev/null
	printf "\r%.3f%%" "$TOTAL"
        sudo scripts/update-motd.sh "$(printf "Experiments running! %.3f%% complete -Mark Jenkins (s1309061)" "$TOTAL")" >> /dev/null
	TOTAL=$(bc -l <<< "$TOTAL + $STEP")
done


make clean
make flags="-DDETAILED_METRICS -DMYBARRIER -DCONVERGE_TEST" main



sudo scripts/update-motd.sh "" >> /dev/null

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



NUM_WORKERS_MIN=1
NUM_WORKERS_MAX=64
NUM_WORKERS_STEP=1

# NUM_RUNS=($POWERS_MAX - $POWERS_MIN + 1) * (((NUM_WORKERS_MAX - NUM_WORKERS_MIN) / NUM_WORKERS_STEP) + 1)

NUM_RUNS=64

STEP=$(bc -l <<< "100 / $NUM_RUNS")
TOTAL=$STEP

# Create config file

FILENAME="configs/optimal_threads_3.ini"

if [ -e $FILENAME ]; then
  rm $FILENAME
fi

echo "num_runs: \"1\"" > $FILENAME
echo "num_stages: \"1\"" >> $FILENAME
echo "num_iterations_0: \"10000\"" >> $FILENAME
echo "set_pin_bool_0: \"0\"" >> $FILENAME
echo "pinnings_0: \"\"" >> $FILENAME
echo "kernels_0: \"mulpd\"" >> $FILENAME
echo "kernel_durations_0: \"\"" >> $FILENAME
echo "kernel_repeats_0: \"10\"" >> $FILENAME 
echo "grid_size: \"256\"" >> $FILENAME
echo "num_workers_0: \"1\"" >> $FILENAME

# Start experiments

printf "0.000%%"
sudo scripts/update-motd.sh "Experiments running! 0.000% complete -Mark Jenkins (s1309061)" >> /dev/null

START=$(date +%s.%N)



make clean > /dev/null
make flags="-DPTHREADBARRIER -DBASIC_KERNEL_SMALL -DMULPD -DCONVERGE_TEST" main > /dev/null



head -n -1 $FILENAME > temp.ini
echo "num_workers_0: \"1\"" >> temp.ini
mv temp.ini $FILENAME

for ((k=$NUM_WORKERS_MIN; k<=$NUM_WORKERS_MAX; k+=$NUM_WORKERS_STEP))
do
	head -n -1 $FILENAME > temp.ini ; echo "num_workers_0: \"$k\"" >> temp.ini ; mv temp.ini $FILENAME
	bin/jacobi $FILENAME 10
	printf "\r%.3f%%" "$TOTAL"
        sudo scripts/update-motd.sh "$(printf "Experiments running! %.3f%% complete -Mark Jenkins (s1309061)" "$TOTAL")" >> /dev/null
	TOTAL=$(bc -l <<< "$TOTAL + $STEP")
done



END=$(date +%s.%N)
DIFF=$(echo "$END - $START" | bc)

sh send-encrypted.sh -k qGE5Pn -p Archimedes -s klvlqmhb -t "Experiments complete" -m "Optimal threads 3 completed! Time taken: $DIFF seconds"

sudo scripts/update-motd.sh "" > /dev/null

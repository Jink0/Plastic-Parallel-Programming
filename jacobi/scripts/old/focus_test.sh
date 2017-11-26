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



POWERS_MIN=8
POWERS_MAX=11

NUM_WORKERS_MIN=8
NUM_WORKERS_MAX=32
NUM_WORKERS_STEP=8

# NUM_RUNS=($POWERS_MAX - $POWERS_MIN + 1) * (((NUM_WORKERS_MAX - NUM_WORKERS_MIN) / NUM_WORKERS_STEP) + 1)

NUM_RUNS=48

STEP=$(bc -l <<< "100 / $NUM_RUNS")
TOTAL=$STEP

# Create config file

FILENAME="configs/performance_investigation.ini"

if [ -e $FILENAME ]; then
  rm $FILENAME
fi

echo "num_runs: \"1\"" > $FILENAME
echo "num_stages: \"1\"" >> $FILENAME
echo "num_iterations_0: \"500\"" >> $FILENAME
echo "set_pin_bool_0: \"0\"" >> $FILENAME
echo "pinnings_0: \"\"" >> $FILENAME
echo "kernels_0: \"addpd\"" >> $FILENAME
echo "kernel_repeats_0: \"1000\"" >> $FILENAME 
echo "kernel_durations_0: \"\"" >> $FILENAME
echo "grid_size: \"256\"" >> $FILENAME
echo "num_workers_0: \"8\"" >> $FILENAME

# Start experiments

make clean > /dev/null
make flags="-DPTHREADBARRIER -DBASIC_KERNEL_SMALL -DCONVERGE_TEST" main > /dev/null

printf "0.000%%"
sudo scripts/update-motd.sh "Experiments running! 0.000% complete -Mark Jenkins (s1309061)" >> /dev/null

START=$(date +%s.%N)


make clean > /dev/null
make flags="-DPTHREADBARRIER -DBASIC_KERNEL_SMALL -DEXECUTE_KERNELS -DCONVERGE_TEST" main > /dev/null


head -n -5 $FILENAME > temp.ini
echo "kernels_0: \"mulpd\"" >> temp.ini
echo "kernel_repeats_0: \"700\"" >> temp.ini 
echo "kernel_durations_0: \"\"" >> temp.ini
echo "grid_size: \"256\"" >> temp.ini
echo "num_workers_0: \"8\"" >> temp.ini
mv temp.ini $FILENAME


for ((i=$POWERS_MIN; i<=$POWERS_MAX; i++))
do 
	head -n -2 $FILENAME > temp.ini ; echo "grid_size: \"$((2**$i))\"" >> temp.ini ; echo "num_workers_0: \"8\"" >> temp.ini ; mv temp.ini $FILENAME

	for ((j=$NUM_WORKERS_MIN; j<=$NUM_WORKERS_MAX; j+=$NUM_WORKERS_STEP))
	do
		head -n -1 $FILENAME > temp.ini ; echo "num_workers_0: \"$j\"" >> temp.ini ; mv temp.ini $FILENAME
		bin/jacobi $FILENAME > /dev/null
		printf "\r%.3f%%" "$TOTAL"
	        sudo scripts/update-motd.sh "$(printf "Experiments running! %.3f%% complete -Mark Jenkins (s1309061)" "$TOTAL")" >> /dev/null
		TOTAL=$(bc -l <<< "$TOTAL + $STEP")
	done
done


head -n -5 $FILENAME > temp.ini
echo "kernels_0: \"shared_mem_read_small\"" >> temp.ini
echo "kernel_repeats_0: \"1\"" >> temp.ini 
echo "kernel_durations_0: \"\"" >> temp.ini
echo "grid_size: \"256\"" >> temp.ini
echo "num_workers_0: \"8\"" >> temp.ini
mv temp.ini $FILENAME


for ((i=$POWERS_MIN; i<=$POWERS_MAX; i++))
do 
	head -n -2 $FILENAME > temp.ini ; echo "grid_size: \"$((2**$i))\"" >> temp.ini ; echo "num_workers_0: \"8\"" >> temp.ini ; mv temp.ini $FILENAME

	for ((j=$NUM_WORKERS_MIN; j<=$NUM_WORKERS_MAX; j+=$NUM_WORKERS_STEP))
	do
		head -n -1 $FILENAME > temp.ini ; echo "num_workers_0: \"$j\"" >> temp.ini ; mv temp.ini $FILENAME
		bin/jacobi $FILENAME > /dev/null
		printf "\r%.3f%%" "$TOTAL"
	        sudo scripts/update-motd.sh "$(printf "Experiments running! %.3f%% complete -Mark Jenkins (s1309061)" "$TOTAL")" >> /dev/null
		TOTAL=$(bc -l <<< "$TOTAL + $STEP")
	done
done


head -n -5 $FILENAME > temp.ini
echo "kernels_0: \"shared_mem_read_large\"" >> temp.ini
echo "kernel_repeats_0: \"1\"" >> temp.ini 
echo "kernel_durations_0: \"\"" >> temp.ini
echo "grid_size: \"256\"" >> temp.ini
echo "num_workers_0: \"8\"" >> temp.ini
mv temp.ini $FILENAME


for ((i=$POWERS_MIN; i<=$POWERS_MAX; i++))
do 
	head -n -2 $FILENAME > temp.ini ; echo "grid_size: \"$((2**$i))\"" >> temp.ini ; echo "num_workers_0: \"8\"" >> temp.ini ; mv temp.ini $FILENAME

	for ((j=$NUM_WORKERS_MIN; j<=$NUM_WORKERS_MAX; j+=$NUM_WORKERS_STEP))
	do
		head -n -1 $FILENAME > temp.ini ; echo "num_workers_0: \"$j\"" >> temp.ini ; mv temp.ini $FILENAME
		bin/jacobi $FILENAME > /dev/null
		printf "\r%.3f%%" "$TOTAL"
	        sudo scripts/update-motd.sh "$(printf "Experiments running! %.3f%% complete -Mark Jenkins (s1309061)" "$TOTAL")" >> /dev/null
		TOTAL=$(bc -l <<< "$TOTAL + $STEP")
	done
done


END=$(date +%s.%N)
DIFF=$(echo "$END - $START" | bc)

sh send-encrypted.sh -k qGE5Pn -p Archimedes -s klvlqmhb -t "Experiments complete" -m "Convergence tests have completed! Time taken: $DIFF seconds"

sudo scripts/update-motd.sh "" > /dev/null
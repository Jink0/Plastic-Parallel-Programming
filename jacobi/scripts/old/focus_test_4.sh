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



MULPD_REPEATS_MIN=100
MULPD_REPEATS_MAX=1000
MULPD_REPEATS_STEP=100

GRID_SIZE_MIN=32
GRID_SIZE_MAX=128
GRID_SIZE_STEP=32

NUM_WORKERS_MIN=8
NUM_WORKERS_MAX=32
NUM_WORKERS_STEP=8

# NUM_RUNS=($POWERS_MAX - $POWERS_MIN + 1) * (((NUM_WORKERS_MAX - NUM_WORKERS_MIN) / NUM_WORKERS_STEP) + 1)

NUM_RUNS=240

STEP=$(bc -l <<< "100 / $NUM_RUNS")
TOTAL=$STEP

# Create config file

FILENAME="configs/focus_test_4.ini"

if [ -e $FILENAME ]; then
  rm $FILENAME
fi

echo "num_runs: \"1\"" > $FILENAME
echo "num_stages: \"1\"" >> $FILENAME
echo "num_iterations_0: \"100\"" >> $FILENAME
echo "set_pin_bool_0: \"0\"" >> $FILENAME
echo "pinnings_0: \"\"" >> $FILENAME
echo "kernels_0: \"\"" >> $FILENAME
echo "kernel_durations_0: \"\"" >> $FILENAME
echo "kernel_repeats_0: \"\"" >> $FILENAME 
echo "grid_size: \"32\"" >> $FILENAME
echo "num_workers_0: \"8\"" >> $FILENAME

# Start experiments

printf "0.000%%"
sudo scripts/update-motd.sh "Experiments running! 0.000% complete -Mark Jenkins (s1309061)" >> /dev/null

START=$(date +%s.%N)


make clean > /dev/null
make flags="-DPTHREADBARRIER -DBASIC_KERNEL_SMALL -DMULPD -DCONVERGE_TEST" main > /dev/null


for ((i=$MULPD_REPEATS_MIN; i<=$MULPD_REPEATS_MAX; i+=$MULPD_REPEATS_STEP))
do

	for ((j=$GRID_SIZE_MIN; j<=$GRID_SIZE_MAX; j+=$GRID_SIZE_STEP))
	do 
		head -n -2 $FILENAME > temp.ini ; echo "grid_size: \"$j\"" >> temp.ini ; echo "num_workers_0: \"$NUM_WORKERS_MIN\"" >> temp.ini ; mv temp.ini $FILENAME

		for ((k=$NUM_WORKERS_MIN; k<=$NUM_WORKERS_MAX; k+=$NUM_WORKERS_STEP))
		do
			if [ $k != 24 ]
			then
				head -n -1 $FILENAME > temp.ini ; echo "num_workers_0: \"$k\"" >> temp.ini ; mv temp.ini $FILENAME
				bin/jacobi $FILENAME $i
				printf "\r%.3f%%" "$TOTAL"
			    sudo scripts/update-motd.sh "$(printf "Experiments running! %.3f%% complete -Mark Jenkins (s1309061)" "$TOTAL")" >> /dev/null
				TOTAL=$(bc -l <<< "$TOTAL + $STEP")
			fi
		done
	done
done


make clean > /dev/null
make flags="-DPTHREADBARRIER -DBASIC_KERNEL_SMALL -DEXECUTE_KERNELS -DCONVERGE_TEST" main > /dev/null


head -n -5 $FILENAME > temp.ini
echo "kernels_0: \"mulpd\"" >> temp.ini
echo "kernel_durations_0: \"\"" >> temp.ini
echo "kernel_repeats_0: \"100\"" >> temp.ini 
echo "grid_size: \"32\"" >> temp.ini
echo "num_workers_0: \"8\"" >> temp.ini
mv temp.ini $FILENAME

for ((i=$MULPD_REPEATS_MIN; i<=$MULPD_REPEATS_MAX; i+=$MULPD_REPEATS_STEP))
do
	head -n -3 $FILENAME > temp.ini ; echo "kernel_repeats_0: \"$i\"" >> temp.ini ; echo "grid_size: \"$GRID_SIZE_MIN\"" >> temp.ini ; echo "num_workers_0: \"$NUM_WORKERS_MIN\"" >> temp.ini ; mv temp.ini $FILENAME

	for ((j=$GRID_SIZE_MIN; j<=$GRID_SIZE_MAX; j+=$GRID_SIZE_STEP))
	do 
		head -n -2 $FILENAME > temp.ini ; echo "grid_size: \"$j\"" >> temp.ini ; echo "num_workers_0: \"$NUM_WORKERS_MIN\"" >> temp.ini ; mv temp.ini $FILENAME

		for ((k=$NUM_WORKERS_MIN; k<=$NUM_WORKERS_MAX; k+=$NUM_WORKERS_STEP))
		do
			if [ $k != 24 ]
			then
				head -n -1 $FILENAME > temp.ini ; echo "num_workers_0: \"$k\"" >> temp.ini ; mv temp.ini $FILENAME
				bin/jacobi $FILENAME 
				printf "\r%.3f%%" "$TOTAL"
			        sudo scripts/update-motd.sh "$(printf "Experiments running! %.3f%% complete -Mark Jenkins (s1309061)" "$TOTAL")" >> /dev/null
				TOTAL=$(bc -l <<< "$TOTAL + $STEP")
			fi
		done
	done
done


END=$(date +%s.%N)
DIFF=$(echo "$END - $START" | bc)

sh send-encrypted.sh -k qGE5Pn -p Archimedes -s klvlqmhb -t "Experiments complete" -m "Focustest 2 completed! Time taken: $DIFF seconds"

sudo scripts/update-motd.sh "" > /dev/null

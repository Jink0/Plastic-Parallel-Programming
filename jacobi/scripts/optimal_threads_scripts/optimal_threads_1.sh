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



NUM_WORKERS_MIN=2
NUM_WORKERS_MAX=24
NUM_WORKERS_STEP=2

NUM_CORES_MIN=2
NUM_CORES_MAX=24
NUM_CORES_STEP=2

NUM_RUNS=144

STEP=$( bc -l <<< "100 / $NUM_RUNS" )
TOTAL=$STEP

# Create config file

FILENAME="configs/optimal_threads_1.ini"

if [ -e $FILENAME ]; then
  rm $FILENAME
fi

echo "num_runs: \"1\"" > $FILENAME
echo "num_stages: \"1\"" >> $FILENAME
echo "num_iterations_0: \"10\"" >> $FILENAME
echo "set_pin_bool_0: \"2\"" >> $FILENAME
echo "kernels_0: \"cpu\"" >> $FILENAME
echo "kernel_durations_0: \"\"" >> $FILENAME
echo "kernel_repeats_0: \"1000\"" >> $FILENAME 
echo "grid_size: \"128\"" >> $FILENAME
echo "pinnings_0: \"1..2 1..2\"" >> $FILENAME
echo "num_workers_0: \"2\"" >> $FILENAME



# Start experiments

printf "0.000%%"
sudo scripts/update-motd.sh "Experiments running! 0.000% complete -Mark Jenkins (s1309061)" >> /dev/null

START=$(date +%s.%N)



make clean > /dev/null
make flags="-DPTHREADBARRIER -DBASIC_KERNEL_SMALL -DEXECUTE_KERNELS -DCONVERGE_TEST" main > /dev/null



for ((i=$NUM_CORES_MIN; i<=$NUM_CORES_MAX; i+=$NUM_CORES_STEP))
do

    for ((j=$NUM_WORKERS_MIN; j<=$NUM_WORKERS_MAX; j+=$NUM_WORKERS_STEP))
    do

        STRING="1..$i "

        FULL_STRING=$STRING

        for ((p=1; p<j; p++))
        do
            FULL_STRING=${FULL_STRING}${STRING}
        done

        head -n -2 $FILENAME > temp.ini

        echo "pinnings_0: \"$FULL_STRING\"" >> temp.ini
        echo "num_workers_0: \"$j\"" >> temp.ini

        mv temp.ini $FILENAME

    	bin/jacobi $FILENAME

    	printf "\r%.3f%%" "$TOTAL"
        sudo scripts/update-motd.sh "$(printf "Experiments running! %.3f%% complete -Mark Jenkins (s1309061)" "$TOTAL")" >> /dev/null
    	TOTAL=$(bc -l <<< "$TOTAL + $STEP")
    done
done



END=$(date +%s.%N)
DIFF=$(echo "$END - $START" | bc)

sh send-encrypted.sh -k qGE5Pn -p Archimedes -s klvlqmhb -t "Experiments complete" -m "Optimal threads 1 completed! Time taken: $DIFF seconds"

sudo scripts/update-motd.sh "" > /dev/null

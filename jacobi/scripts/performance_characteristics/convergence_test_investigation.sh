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



DEL_PREV_RUNS=false
MACHINE="NONE"

POSITIONAL=()
while [[ $# -gt 0 ]]
do
key="$1"

case $key in
    -d|--delete)
    DEL_PREV_RUNS=true
    shift # past argument
    ;;
    -m|--machine)
    MACHINE="$2"
    shift # past argument
    shift # past value
    ;;
esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

SUPPORTED_MACHINES="spa XXXII"

# Check if we have a valid machine argument
if [[ " $SUPPORTED_MACHINES " =~ .*\ $MACHINE\ .* ]]; then
    echo "Machine: $MACHINE"
    echo
else
    echo "Unsupported machine! : $MACHINE"
    exit 1
fi



if [ "$DEL_PREV_RUNS" = true ] ; then
    echo 'Deleting previous runs...'
    echo
    rm -rf runs/
fi




POWERS_MIN=10
POWERS_MAX=15

NUM_WORKERS_MIN=4
NUM_WORKERS_STEP=4

NUM_WORKERS_MAX=32
NUM_RUNS=96

if [ "$MACHINE" = "spa" ] ; then
    NUM_WORKERS_MAX=12
	NUM_RUNS=36
    STRING="0..11 "
    UPDATE_MOTD=false
    FILENAME="configs/spa/barrier_investigation.ini"
fi

if [ "$MACHINE" = "XXXII" ] ; then
    NUM_WORKERS_MAX=32
	NUM_RUNS=96
    STRING="0..31 "
    UPDATE_MOTD=true
    FILENAME="configs/XXXII/barrier_investigation.ini"
fi













STEP=$(bc -l <<< "100 / $NUM_RUNS")
TOTAL=$STEP

# Create config file

if [ -e $FILENAME ]; then
  rm $FILENAME
fi

echo "num_runs: \"10\"" > $FILENAME
echo "num_stages: \"1\"" >> $FILENAME
echo "num_iterations_0: \"10\"" >> $FILENAME
echo "set_pin_bool_0: \"0\"" >> $FILENAME
echo "pinnings_0: \"\"" >> $FILENAME
echo "kernels_0: \"\"" >> $FILENAME
echo "kernel_repeats_0: \"\"" >> $FILENAME 
echo "kernel_durations_0: \"\"" >> $FILENAME
echo "grid_size: \"1024\"" >> $FILENAME
echo "num_workers_0: \"4\"" >> $FILENAME

# Start experiments

printf "0.000%%"
if [ "$MACHINE" = "XXXII" ] ; then
	sudo scripts/update-motd.sh "Experiments running! 0.000% complete -Mark Jenkins (s1309061)" >> /dev/null
fi

START=$(date +%s.%N)


make clean > /dev/null
make flags="-DPTHREADBARRIER -DBASIC_KERNEL_SMALL" main > /dev/null


for ((i=$POWERS_MIN; i<=$POWERS_MAX; i++))
do 
	head -n -2 $FILENAME > temp.ini ; echo "grid_size: \"$((2**$i))\"" >> temp.ini ; echo "num_workers_0: \"4\"" >> temp.ini ; mv temp.ini $FILENAME

	for ((j=$NUM_WORKERS_MIN; j<=$NUM_WORKERS_MAX; j+=$NUM_WORKERS_STEP))
	do
		head -n -1 $FILENAME > temp.ini ; echo "num_workers_0: \"$j\"" >> temp.ini ; mv temp.ini $FILENAME
		bin/jacobi $FILENAME > /dev/null
		printf "\r%.3f%%" "$TOTAL"
		TOTAL=$(bc -l <<< "$TOTAL + $STEP")
	done
done


make clean > /dev/null
make flags="-DPTHREADBARRIER -DBASIC_KERNEL_SMALL -DCONVERGE_TEST" main > /dev/null


for ((i=$POWERS_MIN; i<=$POWERS_MAX; i++))
do 
	head -n -2 $FILENAME > temp.ini ; echo "grid_size: \"$((2**$i))\"" >> temp.ini ; echo "num_workers_0: \"4\"" >> temp.ini ; mv temp.ini $FILENAME

	for ((j=$NUM_WORKERS_MIN; j<=$NUM_WORKERS_MAX; j+=$NUM_WORKERS_STEP))
	do
		head -n -1 $FILENAME > temp.ini ; echo "num_workers_0: \"$j\"" >> temp.ini ; mv temp.ini $FILENAME
		bin/jacobi $FILENAME > /dev/null
		printf "\r%.3f%%" "$TOTAL"
		TOTAL=$(bc -l <<< "$TOTAL + $STEP")
	done
done


END=$(date +%s.%N)
DIFF=$(echo "$END - $START" | bc)

sh send-encrypted.sh -k qGE5Pn -p Archimedes -s klvlqmhb -t "Experiments complete" -m "Convergence tests have completed! Time taken: $DIFF seconds"

if [ "$MACHINE" = "XXXII" ] ; then
	sudo scripts/update-motd.sh "" > /dev/null
fi
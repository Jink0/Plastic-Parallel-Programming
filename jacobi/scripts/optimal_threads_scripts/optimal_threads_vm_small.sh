#!/bin/bash
# Num runs, Grid size, Num Stages, (Num workers, Num iterations, Set-pin bool, Num cores)

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT

function ctrl_c() {
        if [ "$UPDATE_MOTD" = true ] ; then
            echo "\nTrapped CTRL-C, resetting MOTD..."
            echo 
            echo -e "Trapped CTRL-C, resetting MOTD...\n" >> $LOG_FILENAME
            sudo scripts/update-motd.sh >> $LOG_FILENAME
        else
            echo "\nTrapped CTRL-C"
            echo -e "Trapped CTRL-C\n" >> $LOG_FILENAME
        fi
        
        exit
}

DEL_PREV_RUNS=false
MACHINE="NONE"
RANDOMISE=false

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
    -r|--randomise)
    RANDOMISE=true
    shift # past argument
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



NUM_WORKERS_MIN=2
NUM_WORKERS_STEP=2

NUM_CORES_MIN=2
NUM_CORES_STEP=2

if [ "$MACHINE" = "spa" ] ; then
    NUM_WORKERS_MAX=24
    NUM_CORES_MAX=24
    NUM_RUNS=144
    STRING="0..11 "
    UPDATE_MOTD=false
    FILENAME="configs/spa/optimal_threads_vm_small.ini"
    LOG_FILENAME="logs/spa/optimal_threads_vm_small.log"
fi

if [ "$MACHINE" = "XXXII" ] ; then
    NUM_WORKERS_MAX=48
    NUM_CORES_MAX=48
    NUM_RUNS=576
    STRING="0..31 "
    UPDATE_MOTD=true
    FILENAME="configs/XXXII/optimal_threads_vm_small.ini"
    LOG_FILENAME="logs/XXXII/optimal_threads_vm_small.log"
fi




STEP=$( bc -l <<< "100 / $NUM_RUNS" )
TOTAL=$STEP

# Create config file

# Delete old config if it exists
if [ -e $FILENAME ]; then
  rm $FILENAME
fi

echo "num_runs: \"101\"" > $FILENAME
echo "num_stages: \"1\"" >> $FILENAME
echo "num_iterations_0: \"1000\"" >> $FILENAME
echo "set_pin_bool_0: \"2\"" >> $FILENAME
echo "kernels_0: \"vm\"" >> $FILENAME
echo "kernel_durations_0: \"\"" >> $FILENAME
echo "kernel_repeats_0: \"10\"" >> $FILENAME 
echo "grid_size: \"32\"" >> $FILENAME
echo "num_workers_0: \"2\"" >> $FILENAME
echo "pinnings_0: \"$STRING\"" >> $FILENAME

# Overwrite log
echo -e "Machine: $MACHINE\n\n\n\n" > $LOG_FILENAME



# Start experiments

START=$(date +%s.%N)

make clean >> $LOG_FILENAME
echo -e "\n\n\n\n" >> $LOG_FILENAME

if [ "$RANDOMISE" = true ] ; then
    make flags="-DPTHREAD_BARRIER -DBASIC_KERNEL_SMALL -DRANDOMISE_KERNEL_LOAD -DEXECUTE_KERNELS -DCONVERGENCE_TEST" main >> $LOG_FILENAME
else
    make flags="-DPTHREAD_BARRIER -DBASIC_KERNEL_SMALL -DEXECUTE_KERNELS -DCONVERGENCE_TEST" main >> $LOG_FILENAME
fi

echo -e "\n\n\n\n" >> $LOG_FILENAME

printf "0.000%%"
if [ "$UPDATE_MOTD" = true ] ; then
    sudo scripts/update-motd.sh "Experiments running! 0.000% complete -Mark Jenkins (s1309061)" >> $LOG_FILENAME
    echo -e "\n\n\n\n" >> $LOG_FILENAME
fi



for ((i=$NUM_CORES_MIN; i<=$NUM_CORES_MAX; i+=$NUM_CORES_STEP))
do

    for ((j=$NUM_WORKERS_MIN; j<=$NUM_WORKERS_MAX; j+=$NUM_WORKERS_STEP))
    do

        STRING="0..$(($i-1)) "

        FULL_STRING=$STRING

        for ((p=1; p<j; p++))
        do
            FULL_STRING=${FULL_STRING}${STRING}
        done

        head -n -2 $FILENAME > temp.ini

        echo "num_workers_0: \"$j\"" >> temp.ini
        echo "pinnings_0: \"$FULL_STRING\"" >> temp.ini

        mv temp.ini $FILENAME

        bin/jacobi $FILENAME >> $LOG_FILENAME
        echo -e "\n\n\n\n" >> $LOG_FILENAME

        printf "\r%.3f%%" "$TOTAL"

        if [ "$UPDATE_MOTD" = true ] ; then
            sudo scripts/update-motd.sh "$(printf "Experiments running! %.3f%% complete -Mark Jenkins (s1309061)" "$TOTAL")" >> $LOG_FILENAME
            echo -e "\n\n\n\n" >> $LOG_FILENAME
        fi

        TOTAL=$(bc -l <<< "$TOTAL + $STEP")
    done
done



END=$(date +%s.%N)
DIFF=$(echo "$END - $START" | bc)

if [ "$MACHINE" = "spa" ] ; then
    sh send-encrypted.sh -k qGE5Pn -p Archimedes -s klvlqmhb -t "spa: experiment complete" -m "spa: optimal threads vm small completed! Time taken: $DIFF seconds"
fi

if [ "$MACHINE" = "XXXII" ] ; then
    sh send-encrypted.sh -k qGE5Pn -p Archimedes -s klvlqmhb -t "XXXII: experiment complete" -m "XXXII: optimal threads vm small completed! Time taken: $DIFF seconds"
fi

if [ "$UPDATE_MOTD" = true ] ; then
    sudo scripts/update-motd.sh >> $LOG_FILENAME
    echo -e "\n\n\n\n" >> $LOG_FILENAME
fi
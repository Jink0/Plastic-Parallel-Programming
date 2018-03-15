#!/bin/bash
# Num runs, Grid size, Num Stages, (Num workers, Num iterations, Set-pin bool, Num cores)

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT

function ctrl_c() {
        if [ "$UPDATE_MOTD" = true ] ; then
            echo "\nTrapped CTRL-C, resetting MOTD..."
            echo 
            echo -e "Trapped CTRL-C, resetting MOTD...\n" >> $LOG_FILENAME1
            sudo scripts/update-motd.sh >> $LOG_FILENAME1
        else
            echo "\nTrapped CTRL-C"
            echo -e "Trapped CTRL-C\n" >> $LOG_FILENAME1
        fi

        rm /dev/shm/*
        
        exit
}

DEL_PREV_RUNS=false
MACHINE="NONE"
VARY=false

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
    -v|--vary)
    VARY=true
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



FILENAME1="configs/spa/$(basename $BASH_SOURCE .sh)_1.ini"
LOG_FILENAME1="logs/spa/$(basename $BASH_SOURCE .sh)_1.log"
FILENAME2="configs/spa/$(basename $BASH_SOURCE .sh)_2.ini"
LOG_FILENAME2="logs/spa/$(basename $BASH_SOURCE .sh)_2.log"

if [ "$MACHINE" = "spa" ] ; then
    UPDATE_MOTD=false
fi

if [ "$MACHINE" = "XXXII" ] ; then
    UPDATE_MOTD=true
fi



STEP=$( bc -l <<< "100 / 1" )
TOTAL=$STEP



# Delete old logs if they exist
if [ -e $LOG_FILENAME1 ]; then
  rm $LOG_FILENAME1
fi

if [ -e $LOG_FILENAME2 ]; then
  rm $LOG_FILENAME2
fi

# Log the machine we are on
echo -e "Machine: $MACHINE\n\n\n\n" | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null



# Record start time
START=$(date +%s.%N)

# Clean old version
make clean | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null
echo -e "\n\n\n\n" | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null

# Make new version
if [ "$VARY" = true ] ; then
    make flags="-DSYNC_PROCS=2 -DPTHREAD_BARRIER -DBASIC_KERNEL_SMALL -DVARY_KERNEL_LOAD -DEXECUTE_KERNELS -DCONVERGENCE_TEST" main | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null
else
    make flags="-DSYNC_PROCS=2 -DPTHREAD_BARRIER -DBASIC_KERNEL_SMALL -DEXECUTE_KERNELS -DCONVERGENCE_TEST" main | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null
fi

echo -e "\n\n\n\n" | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null

printf "0.000%%"
if [ "$UPDATE_MOTD" = true ] ; then
    sudo scripts/update-motd.sh "Experiments running! 0.000% complete -Mark Jenkins (s1309061)" | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null
    echo -e "\n\n\n\n" | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null
fi



# Create config files
if [ "$MACHINE" = "spa" ] ; then
    NumCoresOne=24
    NumCoresOneOne=24

    NumWorkersOne=22
    NumWorkersOneOne=24

    NumCoresTwo=24
    NumCoresTwoTwo=24

    NumWorkersTwo=24
    NumWorkersTwoTwo=24
fi

# Delete old config if it exists
if [ -e $FILENAME1 ]; then
  rm $FILENAME1
fi

# Setup parameters for program 1
STRING="0..$(($i-1)) "

FULL_STRING=$STRING

for ((p=1; p<NumCoresOne; p++))
do
    FULL_STRING=${FULL_STRING}${STRING}
done

FULL_STRINGOne=$STRING

for ((p=1; p<NumCoresOneOne; p++))
do
    FULL_STRINGOne=${FULL_STRINGOne}${STRING}
done

# CPU Large

echo "num_runs: \"101\"" > $FILENAME1
echo "num_stages: \"2\"" >> $FILENAME1
echo "num_iterations_0: \"1\"" >> $FILENAME1
echo "set_pin_bool_0: \"2\"" >> $FILENAME1
echo "kernels_0: \"cpu\"" >> $FILENAME1
echo "kernel_durations_0: \"\"" >> $FILENAME1
echo "kernel_repeats_0: \"500\"" >> $FILENAME1 
echo "grid_size: \"256\"" >> $FILENAME1
echo "num_workers_0: \"NumWorkersOne\"" >> $FILENAME1
echo "pinnings_0: \"$FULL_STRING\"" >> $FILENAME1

echo "num_iterations_1: \"1\"" >> $FILENAME1
echo "set_pin_bool_1: \"2\"" >> $FILENAME1
echo "kernels_1: \"cpu\"" >> $FILENAME1
echo "kernel_durations_1: \"\"" >> $FILENAME1
echo "kernel_repeats_1: \"500\"" >> $FILENAME1 
echo "num_workers_1: \"NumWorkersOneOne\"" >> $FILENAME1
echo "pinnings_1: \"$FULL_STRINGOne\"" >> $FILENAME1



# Delete old config if it exists
if [ -e $FILENAME2 ]; then
  rm $FILENAME2
fi

# Setup parameters for program 2
STRING="0..$(($i-1)) "

FULL_STRING=$STRING

for ((p=1; p<NumCoresTwo; p++))
do
    FULL_STRING=${FULL_STRING}${STRING}
done

FULL_STRINGTwo=$STRING

for ((p=1; p<NumCoresTwoTwo; p++))
do
    FULL_STRINGTwo=${FULL_STRINGTwo}${STRING}
done

echo "num_runs: \"101\"" > $FILENAME2
echo "num_stages: \"2\"" >> $FILENAME2
echo "num_iterations_0: \"1\"" >> $FILENAME2
echo "set_pin_bool_0: \"2\"" >> $FILENAME2
echo "kernels_0: \"vm\"" >> $FILENAME2
echo "kernel_durations_0: \"\"" >> $FILENAME2
echo "kernel_repeats_0: \"500\"" >> $FILENAME2 
echo "grid_size: \"256\"" >> $FILENAME2
echo "num_workers_0: \"NumWorkersTwo\"" >> $FILENAME2
echo "pinnings_0: \"$FULL_STRING\"" >> $FILENAME2

echo "num_iterations_1: \"1\"" >> $FILENAME2
echo "set_pin_bool_1: \"2\"" >> $FILENAME2
echo "kernels_1: \"vm\"" >> $FILENAME2
echo "kernel_durations_1: \"\"" >> $FILENAME2
echo "kernel_repeats_1: \"500\"" >> $FILENAME2 
echo "num_workers_1: \"NumWorkersTwoTwo\"" >> $FILENAME2
echo "pinnings_1: \"$FULL_STRINGTwo\"" >> $FILENAME2






COUNT=1

# Start experiments

# Run programs
bin/jacobi $FILENAME1 $COUNT "$(basename $BASH_SOURCE .sh)_1" >> $LOG_FILENAME1 &
bin/jacobi $FILENAME2 $COUNT "$(basename $BASH_SOURCE .sh)_2" >> $LOG_FILENAME2

echo -e "\n\n\n\n" | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null

printf "\r%.3f%%" "$TOTAL"

if [ "$UPDATE_MOTD" = true ] ; then
    sudo scripts/update-motd.sh "$(printf "Experiments running! %.3f%% complete -Mark Jenkins (s1309061)" "$TOTAL")" | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null
    echo -e "\n\n\n\n" | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null
fi

TOTAL=$(bc -l <<< "$TOTAL + $STEP")
COUNT=$( bc -l <<< "$COUNT + 1" )



END=$(date +%s.%N)
DIFF=$(echo "$END - $START" | bc)

if [ "$MACHINE" = "spa" ] ; then
    sh send-encrypted.sh -k qGE5Pn -p Archimedes -s klvlqmhb -t "spa: experiment complete" -m "spa: $(basename $BASH_SOURCE .sh) completed! Time taken: $DIFF seconds"
fi

if [ "$MACHINE" = "XXXII" ] ; then
    sh send-encrypted.sh -k qGE5Pn -p Archimedes -s klvlqmhb -t "XXXII: experiment complete" -m "XXXII: $(basename $BASH_SOURCE .sh) completed! Time taken: $DIFF seconds"
fi

if [ "$UPDATE_MOTD" = true ] ; then
    sudo scripts/update-motd.sh | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null
    echo -e "\n\n\n\n" | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null
fi
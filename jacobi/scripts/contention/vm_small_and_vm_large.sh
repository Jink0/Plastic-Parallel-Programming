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



NUM_WORKERS_MIN=4
NUM_WORKERS_STEP=4

NUM_CORES_MIN=4
NUM_CORES_STEP=4



FILENAME1="configs/spa/$(basename $BASH_SOURCE .sh)_1.ini"
LOG_FILENAME1="logs/spa/$(basename $BASH_SOURCE .sh)_1.log"
FILENAME2="configs/spa/$(basename $BASH_SOURCE .sh)_2.ini"
LOG_FILENAME2="logs/spa/$(basename $BASH_SOURCE .sh)_2.log"

if [ "$MACHINE" = "spa" ] ; then
    NUM_WORKERS_MAX=24
    NUM_CORES_MAX=24
    NUM_RUNS1=36
    STRING="0..11 "
    UPDATE_MOTD=false
fi

if [ "$MACHINE" = "XXXII" ] ; then
    NUM_WORKERS_MAX=48
    NUM_CORES_MAX=48
    NUM_RUNS1=144
    STRING="0..31 "
    UPDATE_MOTD=true
fi

if [ "$MACHINE" = "spa" ] ; then
    NUM_WORKERS_MAX=24
    NUM_CORES_MAX=24
    NUM_RUNS2=36
    STRING="0..11 "
    UPDATE_MOTD=false
fi

if [ "$MACHINE" = "XXXII" ] ; then
    NUM_WORKERS_MAX=48
    NUM_CORES_MAX=48
    NUM_RUNS2=144
    STRING="0..31 "
    UPDATE_MOTD=true
fi



STEP=$( bc -l <<< "100 / ($NUM_RUNS1 * $NUM_RUNS2)" )
TOTAL=$STEP



# Write new configs
scripts/config_generation/vm_small.sh $FILENAME1
scripts/config_generation/vm_large.sh $FILENAME2



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



COUNT=1

# Start experiments

for ((i=$NUM_CORES_MIN; i<=$NUM_CORES_MAX; i+=$NUM_CORES_STEP))
do
    for ((j=$NUM_WORKERS_MIN; j<=$NUM_WORKERS_MAX; j+=$NUM_WORKERS_STEP))
    do
        # Setup parameters for program 1
        STRING="0..$(($i-1)) "

        FULL_STRING=$STRING

        for ((p=1; p<j; p++))
        do
            FULL_STRING=${FULL_STRING}${STRING}
        done

        head -n -2 $FILENAME1 > temp.ini

        echo "num_workers_0: \"$j\"" >> temp.ini
        echo "pinnings_0: \"$FULL_STRING\"" >> temp.ini

        mv temp.ini $FILENAME1



        for ((k=$NUM_CORES_MIN; k<=$NUM_CORES_MAX; k+=$NUM_CORES_STEP))
        do
            for ((l=$NUM_WORKERS_MIN; l<=$NUM_WORKERS_MAX; l+=$NUM_WORKERS_STEP))
            do
                # Setup parameters for program 2
                STRING="0..$(($k-1)) "

                FULL_STRING=$STRING

                for ((q=1; q<l; q++))
                do
                    FULL_STRING=${FULL_STRING}${STRING}
                done

                head -n -2 $FILENAME2 > temp.ini

                echo "num_workers_0: \"$l\"" >> temp.ini
                echo "pinnings_0: \"$FULL_STRING\"" >> temp.ini

                mv temp.ini $FILENAME2



                RAND_VAL=$( bc -l <<< "$i * $j * $k * $l" )

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
            done
        done
    done
done



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
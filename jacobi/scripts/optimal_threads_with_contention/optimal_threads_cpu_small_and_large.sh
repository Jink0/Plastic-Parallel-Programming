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



NUM_WORKERS_MIN=22
NUM_WORKERS_STEP=2

NUM_CORES_MIN=22
NUM_CORES_STEP=2

if [ "$MACHINE" = "spa" ] ; then
    NUM_WORKERS_MAX=24
    NUM_CORES_MAX=24
    NUM_RUNS1=144
    STRING="0..11 "
    UPDATE_MOTD=false
    FILENAME1="configs/spa/otwc_cpu_small.ini"
    LOG_FILENAME1="logs/spa/otwc_cpu_small.log"
fi

if [ "$MACHINE" = "XXXII" ] ; then
    NUM_WORKERS_MAX=48
    NUM_CORES_MAX=48
    NUM_RUNS1=576
    STRING="0..31 "
    UPDATE_MOTD=true
    FILENAME1="configs/spa/otwc_cpu_small.ini"
    LOG_FILENAME1="logs/spa/otwc_cpu_small.log"
fi

if [ "$MACHINE" = "spa" ] ; then
    NUM_WORKERS_MAX=24
    NUM_CORES_MAX=24
    NUM_RUNS2=144
    STRING="0..11 "
    UPDATE_MOTD=false
    FILENAME2="configs/spa/otwc_cpu_large.ini"
    LOG_FILENAME2="logs/spa/otwc_cpu_large.log"
fi

if [ "$MACHINE" = "XXXII" ] ; then
    NUM_WORKERS_MAX=48
    NUM_CORES_MAX=48
    NUM_RUNS2=576
    STRING="0..31 "
    UPDATE_MOTD=true
    FILENAME2="configs/spa/otwc_cpu_large.ini"
    LOG_FILENAME2="logs/spa/otwc_cpu_large.log"
fi



# NUM_RUNS=$(($NUM_RUNS1>$NUM_RUNS2?$NUM_RUNS1:$NUM_RUNS2))

# STEP=$( bc -l <<< "100 / $NUM_RUNS" )
# TOTAL=$STEP

# Create config file



# Delete old config if it exists
if [ -e $FILENAME1 ]; then
  rm $FILENAME1
fi

# Delete old log if it exists
if [ -e $LOG_FILENAME1 ]; then
  rm $LOG_FILENAME1
fi

echo "num_runs: \"1\"" > $FILENAME1
echo "num_stages: \"1\"" >> $FILENAME1
echo "num_iterations_0: \"100\"" >> $FILENAME1
echo "set_pin_bool_0: \"2\"" >> $FILENAME1
echo "kernels_0: \"cpu\"" >> $FILENAME1
echo "kernel_durations_0: \"\"" >> $FILENAME1
echo "kernel_repeats_0: \"75\"" >> $FILENAME1 
echo "grid_size: \"32\"" >> $FILENAME1
echo "num_workers_0: \"2\"" >> $FILENAME1
echo "pinnings_0: \"$STRING\"" >> $FILENAME1

# Delete old config if it exists
if [ -e $FILENAME2 ]; then
  rm $FILENAME2
fi

# Delete old log if it exists
if [ -e $LOG_FILENAME2 ]; then
  rm $LOG_FILENAME2
fi

echo "num_runs: \"1\"" > $FILENAME2
echo "num_stages: \"1\"" >> $FILENAME2
echo "num_iterations_0: \"1\"" >> $FILENAME2
echo "set_pin_bool_0: \"2\"" >> $FILENAME2
echo "kernels_0: \"cpu\"" >> $FILENAME2
echo "kernel_durations_0: \"\"" >> $FILENAME2
echo "kernel_repeats_0: \"100\"" >> $FILENAME2 
echo "grid_size: \"256\"" >> $FILENAME2
echo "num_workers_0: \"2\"" >> $FILENAME2
echo "pinnings_0: \"$STRING\"" >> $FILENAME2

echo -e "Machine: $MACHINE\n\n\n\n" | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null



# Start experiments

START=$(date +%s.%N)

make clean | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null
echo -e "\n\n\n\n" | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null

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



NUM_REPEATS=2

for ((i=$NUM_CORES_MIN; i<=$NUM_CORES_MAX; i+=$NUM_CORES_STEP))
do

    for ((j=$NUM_WORKERS_MIN; j<=$NUM_WORKERS_MAX; j+=$NUM_WORKERS_STEP))
    do

        for ((r=0; r<$NUM_REPEATS; r+=1))
        do

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

            head -n -2 $FILENAME2 > temp.ini

            echo "num_workers_0: \"$j\"" >> temp.ini
            echo "pinnings_0: \"$FULL_STRING\"" >> temp.ini

            mv temp.ini $FILENAME2

        	bin/jacobi $FILENAME1 >> $LOG_FILENAME1 &
            bin/jacobi $FILENAME2 >> $LOG_FILENAME2

            echo -e "\n\n\n\n" | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null

        	printf "\r%.3f%%" "$TOTAL"

            if [ "$UPDATE_MOTD" = true ] ; then
                sudo scripts/update-motd.sh "$(printf "Experiments running! %.3f%% complete -Mark Jenkins (s1309061)" "$TOTAL")" | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null
                echo -e "\n\n\n\n" | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null
            fi

        	# TOTAL=$(bc -l <<< "$TOTAL + $STEP")
        done
    done
done



END=$(date +%s.%N)
DIFF=$(echo "$END - $START" | bc)

if [ "$MACHINE" = "spa" ] ; then
    sh send-encrypted.sh -k qGE5Pn -p Archimedes -s klvlqmhb -t "spa: experiment complete" -m "spa: otwc cpu small cpu large completed! Time taken: $DIFF seconds"
fi

if [ "$MACHINE" = "XXXII" ] ; then
    sh send-encrypted.sh -k qGE5Pn -p Archimedes -s klvlqmhb -t "XXXII: experiment complete" -m "XXXII: otwc cpu small cpu large completed! Time taken: $DIFF seconds"
fi

if [ "$UPDATE_MOTD" = true ] ; then
    sudo scripts/update-motd.sh | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null
    echo -e "\n\n\n\n" | tee $LOG_FILENAME1 $LOG_FILENAME2 > /dev/null
fi
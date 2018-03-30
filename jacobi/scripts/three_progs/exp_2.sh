#!/bin/bash

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



NUM_WORKERS_MIN=2
NUM_WORKERS_STEP=2

NUM_CORES_MIN=2
NUM_CORES_STEP=2



FILENAME1="configs/spa/$(basename $BASH_SOURCE .sh)_1.ini"
LOG_FILENAME1="logs/spa/$(basename $BASH_SOURCE .sh)_1.log"
FILENAME2="configs/spa/$(basename $BASH_SOURCE .sh)_2.ini"
LOG_FILENAME2="logs/spa/$(basename $BASH_SOURCE .sh)_2.log"
FILENAME3="configs/spa/$(basename $BASH_SOURCE .sh)_3.ini"
LOG_FILENAME3="logs/spa/$(basename $BASH_SOURCE .sh)_3.log"

if [ "$MACHINE" = "spa" ] ; then
    NUM_WORKERS_MAX=12
    NUM_CORES_MAX=12
    NUM_RUNS=729
    STRING="0..11 "
fi

if [ "$MACHINE" = "XXXII" ] ; then
    NUM_WORKERS_MAX=32
    NUM_CORES_MAX=32
    NUM_RUNS=144
    STRING="0..31 "
fi



STEP=$( bc -l <<< "100 / $NUM_RUNS" )
TOTAL=$STEP



# Write new configs
scripts/config_generation/cpu_small.sh $FILENAME1
scripts/config_generation/vm_small.sh $FILENAME2
scripts/config_generation/cpu_large.sh $FILENAME3



# Delete old logs if they exist
if [ -e $LOG_FILENAME1 ]; then
  rm $LOG_FILENAME1
fi

if [ -e $LOG_FILENAME2 ]; then
  rm $LOG_FILENAME2
fi

if [ -e $LOG_FILENAME3 ]; then
  rm $LOG_FILENAME3
fi

# Log the machine we are on
echo -e "Machine: $MACHINE\n\n\n\n" | tee $LOG_FILENAME1 $LOG_FILENAME2 $LOG_FILENAME3 > /dev/null



# Record start time
START=$(date +%s.%N)

# Clean old version
make clean | tee $LOG_FILENAME1 $LOG_FILENAME2 $LOG_FILENAME3 > /dev/null
echo -e "\n\n\n\n" | tee $LOG_FILENAME1 $LOG_FILENAME2 $LOG_FILENAME3 > /dev/null

# Make new version
if [ "$VARY" = true ] ; then
    make flags="-DSYNC_PROCS=3 -DPTHREAD_BARRIER -DBASIC_KERNEL_SMALL -DVARY_KERNEL_LOAD -DEXECUTE_KERNELS -DCONVERGENCE_TEST" main | tee $LOG_FILENAME1 $LOG_FILENAME2 $LOG_FILENAME3 > /dev/null
else
    make flags="-DSYNC_PROCS=3 -DPTHREAD_BARRIER -DBASIC_KERNEL_SMALL -DEXECUTE_KERNELS -DCONVERGENCE_TEST" main | tee $LOG_FILENAME1 $LOG_FILENAME2 $LOG_FILENAME3 > /dev/null
fi

echo -e "\n\n\n\n" | tee $LOG_FILENAME1 $LOG_FILENAME2 $LOG_FILENAME3 > /dev/null

printf "0.000%%"



COUNT=1000

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



                for ((m=$NUM_CORES_MIN; m<=$NUM_CORES_MAX; m+=$NUM_CORES_STEP))
                do
                    for ((n=$NUM_WORKERS_MIN; n<=$NUM_WORKERS_MAX; n+=$NUM_WORKERS_STEP))
                    do
                        # Setup parameters for program 3
                        STRING="0..$(($m-1)) "

                        FULL_STRING=$STRING

                        for ((q=1; q<n; q++))
                        do
                            FULL_STRING=${FULL_STRING}${STRING}
                        done

                        head -n -2 $FILENAME3 > temp.ini

                        echo "num_workers_0: \"$n\"" >> temp.ini
                        echo "pinnings_0: \"$FULL_STRING\"" >> temp.ini

                        mv temp.ini $FILENAME3



                        RAND_VAL=$( bc -l <<< "$i * $j * $k * $l * $m * $n" )

                        # Run programs
                        bin/jacobi $FILENAME1 $COUNT "$(basename $BASH_SOURCE .sh)_1" >> $LOG_FILENAME1 &
                        bin/jacobi $FILENAME2 $COUNT "$(basename $BASH_SOURCE .sh)_2" >> $LOG_FILENAME2 &
                        bin/jacobi $FILENAME3 $COUNT "$(basename $BASH_SOURCE .sh)_3" >> $LOG_FILENAME3
			
                        echo -e "\n\n\n\n" | tee $LOG_FILENAME1 $LOG_FILENAME2 $LOG_FILENAME3 > /dev/null

                        printf "\r%.3f%%" "$TOTAL"

                        TOTAL=$(bc -l <<< "$TOTAL + $STEP")
                        COUNT=$( bc -l <<< "$COUNT + 1" )
                    done
                done
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

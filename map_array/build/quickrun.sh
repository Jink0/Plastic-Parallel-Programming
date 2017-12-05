#!/bin/bash
echo "Deleting previous runs"

rm -rf runs/

echo "Running experiment..."  

start=`date +%s`

gnome-terminal -x ../bin/controller
gnome-terminal -x ../bin/map_array_test $1

ps cax | grep map_array_test > /dev/null

while [ $? -eq 0 ]
do
	ps cax | grep map_array_test > /dev/null
done

end=`date +%s`

echo "Experiment finished in $((end-start)) seconds"

PID=`ps -eaf | grep controller | grep -v grep | awk '{print $2}'`

echo Killing controller process with PID: $PID

kill -9 $PID
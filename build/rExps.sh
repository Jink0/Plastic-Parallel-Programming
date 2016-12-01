#!/bin/bash  
echo "Deleting previous runs"

rm -rf runs/

echo "Running experiments..."  

start=`date +%s`

for i in {1..4}
do
	../bin/map_array_test config$i.ini > /dev/null
done

end=`date +%s`

echo
echo "Experiments finished in $((end-start)) seconds"
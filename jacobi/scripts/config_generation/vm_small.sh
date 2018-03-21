#!/bin/bash

# Create config file

# Delete old config if it exists
if [ -e $1 ]; then
  rm $1
fi

echo "num_runs: \"1\"" > $1
echo "num_stages: \"1\"" >> $1
echo "num_iterations_0: \"1000\"" >> $1
echo "set_pin_bool_0: \"2\"" >> $1
echo "kernels_0: \"vm\"" >> $1
echo "kernel_durations_0: \"\"" >> $1
echo "kernel_repeats_0: \"10\"" >> $1 
echo "grid_size: \"32\"" >> $1
echo "num_workers_0: \"2\"" >> $1
echo "pinnings_0: \"\"" >> $1
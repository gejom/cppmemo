#!/bin/bash

EXECUTABLE=./knapsack

# Feel free to change the two variables below as needed
KNAPSACK_CAPACITIES_LIST="20000 50000 100000 150000 200000"
NUMBER_OF_THREADS_LIST="1 2 4 8"

export CPPMEMO_PRINT_AS_ROW=1

# Print table header
echo "Number of items   Knapsack capacity   Number of threads   Elapsed time (sec.)"
echo "-----------------------------------------------------------------------------"

for KNAPSACK_CAPACITY in $KNAPSACK_CAPACITIES_LIST
do
    for NUMBER_OF_THREADS in $NUMBER_OF_THREADS_LIST
    do
        $EXECUTABLE $NUMBER_OF_THREADS $KNAPSACK_CAPACITY
    done
done

unset CPPMEMO_PRINT_AS_ROW

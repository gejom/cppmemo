#!/bin/bash

EXECUTABLE=./matrix_chain

# Feel free to change the two variables below as needed
NUMBER_OF_MATRICES_LIST="100 200 300 400 500"
NUMBER_OF_THREADS_LIST="1 2 4 8"

export CPPMEMO_PRINT_AS_ROW=1

# Print table header
echo "Number of matrices   Number of threads   Elapsed time (sec.)"
echo "------------------------------------------------------------"

for NUMBER_OF_MATRICES in $NUMBER_OF_MATRICES_LIST
do
    for NUMBER_OF_THREADS in $NUMBER_OF_THREADS_LIST
    do
        $EXECUTABLE $NUMBER_OF_THREADS $NUMBER_OF_MATRICES
    done
done

unset CPPMEMO_PRINT_AS_ROW

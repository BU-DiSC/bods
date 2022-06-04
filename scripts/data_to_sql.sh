#!/bin/bash

# arguments for this script
# arg1: input_file_name or data file path
# arg2: workload option
# arg3: preload_threshold as percentage 
# arg4: number of queries in mixed workload

INPUT_FILE_NAME=$1
LOAD="load"
OPERATIONS="operations"
EXT=".txt"
N=$(wc -l <${INPUT_FILE_NAME})
# first remove files if it exists so we don't mess up statements
rm -rf $LOAD$EXT
rm -rf $OPERATIONS$EXT

# Full bulkload = 1
# Insert only = 2
# Mixed with no pre-load = 3
# Mixed with pre-load as bulk load = 4
# Mixed with pre-load as one by one insert = 5
WORKLOAD_OPT=$2

# pre-load threshold as fraction
PRELOAD_THRESH=$3
NUM_PRELOAD=$(($N * $PRELOAD_THRESH/100))
printf "Num preload = ${NUM_PRELOAD}\n"

NUM_QUERIES=$4

case $WORKLOAD_OPT in
1)
    printf "Workload Option 1: Fully Bulkload Data\n"
    # call dedicated script
    ;;

2)
    printf "Workload Option 2: One-by-one Insert Only\n"
    # call dedicated script
    ./insert_only_workload.sh $INPUT_FILE_NAME ${OPERATIONS}${EXT}
    ;;

3)
    printf "Workload Option 3: Mixed workload with no pre-loading\n"
    # mixed.sh input_file_name num_load num_queries load_file operations_file
    ./mixed.sh $INPUT_FILE_NAME 0 100 $LOAD$EXT $OPERATIONS$EXT
    ;;

4)
    printf "Workload Option 4: Mixed with pre-loading using bulk load\n"
    # script to be done yet
    ;;

5)
    printf "Workload Option 5: Mixed with pre-loading using one-by-one inserts\n"
    # mixed.sh input_file_name num_load num_queries load_file operations_file
    ./mixed.sh $INPUT_FILE_NAME $NUM_PRELOAD $NUM_QUERIES $LOAD$EXT $OPERATIONS$EXT
    ;;

*) echo -n "wrong option" ;;
esac

# while IFS=, read -r field1 field2
# do
#     # echo "$field1 and $field2"

# done < $INPUT_FILE_NAME

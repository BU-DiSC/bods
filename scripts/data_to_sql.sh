#!/bin/bash

INPUT_FILE_NAME=$1
LOAD="load"
OPERATIONS="operations"
EXT=".txt"

# first remove files if it exists so we don't mess up statements
rm -rf $LOAD$EXT
rm -rf $OPERATIONS$EXT

# Full bulkload = 1
# Insert only = 2
# Mixed with no pre-load = 3
# Mixed with pre-load as bulk load = 4
# Mixed with pre-load as one by one insert = 5
WORKLOAD_OPT=$2

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
    ./mixed_no_preload.sh $INPUT_FILE_NAME 100 100 $LOAD$EXT $OPERATIONS$EXT 
    ;;

*) echo -n "wrong option" ;;
esac

# while IFS=, read -r field1 field2
# do
#     # echo "$field1 and $field2"

# done < $INPUT_FILE_NAME

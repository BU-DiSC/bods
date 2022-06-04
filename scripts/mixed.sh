#!/bin/bash

coinflip() { return $(($RANDOM % 2)); }

INPUT_FILE_NAME=$1
NUM_LOAD=$2
NUM_QUERIES=$3
LOAD_OPS_FILE=$4
OUTPUT_OPS_FILE=$5

LOAD_CTR=0
OPS_CTR=0

while IFS=, read -r field1 field2; do
    # echo "$field1 and $field2"
    printf "INSERT INTO TABLE1 VALUES (${field1}, \"${field2}\");\n" >>$LOAD_OPS_FILE
    ((LOAD_CTR = LOAD_CTR + 1))
    # echo $LOAD_CTR
    if [[ $LOAD_CTR -eq $NUM_LOAD ]]; then
        break
    fi
done <$INPUT_FILE_NAME

START_FROM_LINE=$((LOAD_CTR + 1))
# now we will do mixed workload

TOT_INS=$((LOAD_CTR))
TOT_QRS=0
N=$(wc -l <${INPUT_FILE_NAME})

LINE_NUM=$((LOAD_CTR + 1))
while true; do
    if [[ $TOT_INS -le $N && $TOT_QRS -lt $NUM_QUERIES ]]; then
        # P=$(echo "scale=4 ; ${RANDOM}/32767" | bc -l)
        # echo $P
        FLIP=$((RANDOM % 2))
        # echo "flip "

        # echo "flip${FLIP}, ${TOT_INS}, ${N}, ${TOT_QRS}, ${NUM_QUERIES}"
        #
        if [[ $FLIP -eq 1 ]]; then
            while IFS=, read -r field1 field2; do
                printf "INSERT INTO TABLE1 VALUES (${field1}, \"${field2}\");\n" >>$OUTPUT_OPS_FILE
            done < <(sed -n ${LINE_NUM}p ${INPUT_FILE_NAME})
            LINE_NUM=$((LINE_NUM + 1))
            TOT_INS=$((TOT_INS + 1))
            # printf "flip heads\n"
        else
            QUERY_INDEX=$(((RANDOM % $N) + 1))
            printf "SELECT * FROM TABLE1 WHERE COL1=${QUERY_INDEX};\n" >>$OUTPUT_OPS_FILE
            TOT_QRS=$((TOT_QRS + 1))
            # printf "flip tails\n"
        fi
        OPS_CTR=$((OPS_CTR + 1))

    elif [[ $TOT_INS -le $N && $TOT_QRS -ge $NUM_QUERIES ]]; then
        # printf "done with queries"
        while IFS=, read -r field1 field2; do
            printf "INSERT INTO TABLE1 VALUES (${field1}, \"${field2}\");\n" >>$OUTPUT_OPS_FILE
        done < <(sed -n ${LINE_NUM}p ${INPUT_FILE_NAME})
        LINE_NUM=$((LINE_NUM + 1))
        TOT_INS=$((TOT_INS + 1))
        OPS_CTR=$((OPS_CTR + 1))

    elif [[ $TOT_INS -ge $N && $TOT_QRS -le $NUM_QUERIES ]]; then
        # printf "done with inserts"
        QUERY_INDEX=$(((RANDOM % $N) + 1))
        printf "SELECT * FROM TABLE1 WHERE COL1=${QUERY_INDEX};\n" >>$OUTPUT_OPS_FILE
        TOT_QRS=$((TOT_QRS + 1))
        OPS_CTR=$((OPS_CTR + 1))
    fi

    if [[ $TOT_INS -ge $N && $TOT_QRS -ge $NUM_QUERIES ]]; then
        # printf "reached limit"
        break
    fi
done

printf "Total number of Loads = ${LOAD_CTR}\n"
printf "Total number of Inserts = ${TOT_INS}\n"
printf "Total number of queries = ${TOT_QRS}\n"
printf "Total number of operations = ${OPS_CTR}\n"

# while IFS=, read -r field1 field2; do
#     # echo "$field1 and $field2"
#     RAND=`echo "scale=4 ; ${RANDOM}/32767" | bc -l`

#     printf "INSERT INTO TABLE1 VALUES (${field1}, \"${field2}\");\n" >>$OUTPUT_OPS_FILE
#     ((OPS_CTR=OPS_CTR+1))
#     echo $OPS_CTR
#     if [[ $OPS_CTR -eq $NUM_LOAD ]]; then
#         break
#     fi
# done < <(tail -n +${START_FROM_LINE} $INPUT_FILE_NAME)

#!/bin/bash

if [ -z "$DB" ]; then
  DB="POSTGRES"
fi
if [ -z "$N" ]; then
  N=1000
fi
if [ -z "$K" ]; then
  K=50
fi
if [ -z "$L" ]; then
  L=50
fi
if [ -z "$SEED" ]; then
  SEED=1234
fi
if [ -z "$ALPHA" ]; then
  ALPHA=1
fi
if [ -z "$BETA" ]; then
  BETA=1
fi
if [ -z "$OUTPUT_DIR" ]; then
  OUTPUT_DIR=./workloads
fi
if [ -z "$ENTRY_SIZE" ]; then
  ENTRY_SIZE=252
fi

RANDOM=$SEED
WORKLOAD=$OUTPUT_DIR/createdata_N${N}_K${K}_L${L}_S${SEED}_a${ALPHA}_b${BETA}_P${ENTRY_SIZE}

# Full bulk-load = 1
# Insert only = 2
# Mixed with no pre-load = 3
# Mixed with pre-load as bulk load = 4
# Mixed with pre-load as one by one insert = 5
WORKLOAD_OPT=$1

# pre-load threshold as fraction
if [ "$2" ]; then
  NUM_PRELOAD=$((N * $2 / 100))
else
  NUM_PRELOAD=0
fi
if [ "$3" ]; then
  NUM_QUERIES=$3
else
  NUM_QUERIES=0
fi

TMP_FILE=$OUTPUT_DIR/partial.csv
OPERATIONS=$OUTPUT_DIR/operations.sql
PRELOAD=$OUTPUT_DIR/preload.sql
if [ $DB == "POSTGRES" ]; then
  DB_INIT=postgres_init.sql
elif [ $DB == "MONETDB" ]; then
  DB_INIT=monet_init.sql
elif [ $DB == "MYSQL" ]; then
  DB_INIT=mysql_init.sql
fi
LOG_FILE=/dev/null
WORKLOAD_FILE=$WORKLOAD.csv

# first remove files if it exists so we don't mess up statements
echo $N, $K, $L, $SEED, $ALPHA, $BETA, $ENTRY_SIZE, "$1", $NUM_PRELOAD, $NUM_QUERIES >>$LOG_FILE
if [ ! -f $WORKLOAD_FILE ]; then
  ./work_gen_mod -N $N -K $K -L $L -S $SEED -a $ALPHA -b $BETA -o $WORKLOAD_FILE -P $ENTRY_SIZE >>$LOG_FILE
fi

{
  case $WORKLOAD_OPT in
  1)
    if [ $DB == "POSTGRES" ]; then
      echo "\COPY test_table FROM '$(realpath $WORKLOAD_FILE)' CSV;"
    elif [ $DB == "MONETDB" ]; then
      echo "COPY INTO test_table FROM '$(realpath $WORKLOAD_FILE)' ON CLIENT USING DELIMITERS ',';"
    elif [ $DB == "MYSQL" ]; then
      echo "LOAD DATA LOCAL INFILE '$(realpath $WORKLOAD_FILE)' INTO TABLE test_table FIELDS TERMINATED BY ',';"
    fi
    ;;
  2)
    while IFS=, read -r field1 field2; do
      echo "INSERT INTO test_table VALUES ($field1, '$field2');"
    done <$WORKLOAD_FILE
    ;;
  3)
    NUM_PRELOAD=0
    ;;
  4)
    head $WORKLOAD_FILE -n $NUM_PRELOAD >$TMP_FILE
    if [ $DB == "POSTGRES" ]; then
      echo "\COPY test_table FROM '$(realpath $TMP_FILE)' CSV;"
    elif [ $DB == "MONETDB" ]; then
      echo "COPY INTO test_table FROM '$(realpath $TMP_FILE)' ON CLIENT USING DELIMITERS ',';"
    elif [ $DB == "MYSQL" ]; then
      echo "LOAD DATA LOCAL INFILE '$(realpath $TMP_FILE)' INTO TABLE test_table FIELDS TERMINATED BY ',';"
    fi
    ;;
  5)
    head $WORKLOAD_FILE -n $NUM_PRELOAD |
      while IFS=, read -r field1 field2; do
        echo "INSERT INTO test_table VALUES ($field1, '$field2');"
      done
    ;;
  esac
} >$PRELOAD

{
  case $WORKLOAD_OPT in
  1 | 2)
    NUM_PRELOAD=$N
    NUM_QUERIES=0
    TOT_INS=$N
    TOT_QRS=0
    ;;
  3 | 4 | 5)
    TOT_INS=$NUM_PRELOAD
    TOT_QRS=0
    while IFS=, read -r field1 field2; do
      while [[ $TOT_QRS -lt $NUM_QUERIES ]] && ((RANDOM % 2)); do
        TOT_QRS=$((TOT_QRS + 1))
        QUERY_INDEX=$((RANDOM % (TOT_INS + 1)))
        echo "SELECT * FROM test_table WHERE id_col=$QUERY_INDEX;"
      done
      TOT_INS=$((TOT_INS + 1))
      echo "INSERT INTO test_table VALUES ($field1, '$field2');"
    done < <(tail -n +$((NUM_PRELOAD + 1)) $WORKLOAD_FILE)
    while [[ $TOT_QRS -lt $NUM_QUERIES ]]; do
      TOT_QRS=$((TOT_QRS + 1))
      QUERY_INDEX=$((RANDOM % (TOT_INS + 1)))
      echo "SELECT * FROM test_table WHERE id_col=$QUERY_INDEX;"
    done
    ;;
  esac
} >$OPERATIONS

if [ $DB == "POSTGRES" ]; then
  psql -U postgres -f $DB_INIT >$LOG_FILE
  PRELOAD_TIME=$(/usr/bin/time -f "%E" psql -U postgres -f $PRELOAD 2>&1 >>$LOG_FILE)
  OPERATIONS_TIME=$(/usr/bin/time -f "%E" psql -U postgres -f $OPERATIONS 2>&1 >>$LOG_FILE)
elif [ $DB == "MONETDB" ]; then
  mclient -d db <$DB_INIT >$LOG_FILE
  PRELOAD_TIME=$(/usr/bin/time -f "%E" mclient -d db <$PRELOAD 2>&1 >>$LOG_FILE)
  OPERATIONS_TIME=$(/usr/bin/time -f "%E" mclient -d db <$OPERATIONS 2>&1 >>$LOG_FILE)
elif [ $DB == "MYSQL" ]; then
  mysql sortedness_benchmark <$DB_INIT >$LOG_FILE
  PRELOAD_TIME=$(/usr/bin/time -f "%E" mysql --local-infile=1 sortedness_benchmark <$PRELOAD 2>&1 >>$LOG_FILE)
  OPERATIONS_TIME=$(/usr/bin/time -f "%E" mysql --local-infile=1 sortedness_benchmark <$OPERATIONS 2>&1 >>$LOG_FILE)
fi

echo $DB, $N, $K, $L, $SEED, $ALPHA, $BETA, $ENTRY_SIZE, "$WORKLOAD_OPT", $NUM_PRELOAD, $NUM_QUERIES, "$PRELOAD_TIME", "$OPERATIONS_TIME", $TOT_INS, $TOT_QRS
echo $DB, $N, $K, $L, $SEED, $ALPHA, $BETA, $ENTRY_SIZE, "$WORKLOAD_OPT", $NUM_PRELOAD, $NUM_QUERIES, "$PRELOAD_TIME", "$OPERATIONS_TIME", $TOT_INS, $TOT_QRS >>$LOG_FILE

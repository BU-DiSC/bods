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
WORKLOAD_FILE=$OUTPUT_DIR/createdata_N${N}_K${K}_L${L}_S${SEED}_a${ALPHA}_b${BETA}_P${ENTRY_SIZE}
if [ ! -f $WORKLOAD_FILE ]; then
#  make work_gen_mod
  ./work_gen_mod -N $N -K $K -L $L -S $SEED -a $ALPHA -b $BETA -o $WORKLOAD_FILE -P $ENTRY_SIZE
fi
echo $WORKLOAD_FILE

# Full bulk-load = 1
# Insert only = 2
# Mixed with no pre-load = 3
# Mixed with pre-load as bulk load = 4
# Mixed with pre-load as one by one insert = 5
WORKLOAD_OPT=$1

# pre-load threshold as fraction
if [ "$2" ]; then
  PRELOAD_THRESH=$2
  NUM_PRELOAD=$((N * PRELOAD_THRESH / 100))
  echo "Num preload = $NUM_PRELOAD"
fi
if [ "$3" ]; then
  NUM_QUERIES=$3
  echo "Num queries = $NUM_QUERIES"
fi

TMP_FILE=$OUTPUT_DIR/partial.csv
OPERATIONS=$OUTPUT_DIR/operations.sql
PRELOAD=$OUTPUT_DIR/preload.sql
DB_INIT=$OUTPUT_DIR/db_init.sql
# first remove files if it exists so we don't mess up statements
rm -rf $TMP_FILE $OPERATIONS $PRELOAD

case $WORKLOAD_OPT in
1)
  echo "Workload Option 1: Fully Bulk-load Data"
  # call dedicated script
  if [ $DB == "POSTGRES" ]; then
    echo "COPY test_table FROM '$WORKLOAD_FILE' CSV;" >$PRELOAD
  elif [ $DB == "MONETDB" ]; then
    echo "COPY INTO test_table FROM '$WORKLOAD_FILE' ON CLIENT USING DELIMITERS ',';" >$PRELOAD
  elif [ $DB == "MYSQL" ]; then
    echo "LOAD DATA INFILE '$WORKLOAD_FILE' INTO TABLE test_table FIELDS TERMINATED BY ',';" >$PRELOAD
  fi
  echo >$OPERATIONS
  ;;

2)
  echo "Workload Option 2: One-by-one Insert Only"
  while IFS=, read -r field1 field2; do
    echo "INSERT INTO test_table VALUES ($field1, '$field2');" >>$PRELOAD
  done <$WORKLOAD_FILE
  echo >$OPERATIONS
  ;;

3)
  echo "Workload Option 3: Mixed workload with no pre-loading"
  echo >$PRELOAD
  NUM_PRELOAD=0
  TOT_INS=0
  TOT_QRS=0
  while [[ $TOT_INS -lt $N || $TOT_QRS -lt $NUM_QUERIES ]]; do
    FLIP=$((RANDOM % 2))
    if [[ ($TOT_INS -lt $N && $FLIP -ne 0) || ($TOT_QRS -ge $NUM_QUERIES && $FLIP -eq 0) ]]; then
      TOT_INS=$((TOT_INS + 1))
      while IFS=, read -r field1 field2; do
        echo "INSERT INTO test_table VALUES ($field1, '$field2');" >>$OPERATIONS
      done < <(sed "${TOT_INS}q;d" $WORKLOAD_FILE)
    else
      QUERY_INDEX=$(((RANDOM % N) + 1))
      echo "SELECT * FROM test_table WHERE id_col=$QUERY_INDEX;" >>$OPERATIONS
      TOT_QRS=$((TOT_QRS + 1))
    fi
  done
  ;;

4)
  echo "Workload Option 4: Mixed with pre-loading using bulk load"
  head $WORKLOAD_FILE -n $NUM_PRELOAD >$TMP_FILE
  if [ $DB == "POSTGRES" ]; then
    echo "COPY test_table FROM '$TMP_FILE' CSV;" >$PRELOAD
  elif [ $DB == "MONETDB" ]; then
    echo "COPY INTO test_table FROM '$TMP_FILE' ON CLIENT USING DELIMITERS ',';" >$PRELOAD
  elif [ $DB == "MYSQL" ]; then
    echo "LOAD DATA INFILE '$TMP_FILE' INTO TABLE test_table FIELDS TERMINATED BY ',';" >$PRELOAD
  fi
  TOT_INS=$NUM_PRELOAD
  TOT_QRS=0
  while [[ $TOT_INS -lt $N || $TOT_QRS -lt $NUM_QUERIES ]]; do
    FLIP=$((RANDOM % 2))
    if [[ ($TOT_INS -lt $N && $FLIP -ne 0) || ($TOT_QRS -ge $NUM_QUERIES && $FLIP -eq 0) ]]; then
      TOT_INS=$((TOT_INS + 1))
      while IFS=, read -r field1 field2; do
        echo "INSERT INTO test_table VALUES ($field1, '$field2');" >>$OPERATIONS
      done < <(sed "${TOT_INS}q;d" $WORKLOAD_FILE)
    else
      QUERY_INDEX=$(((RANDOM % N) + 1))
      echo "SELECT * FROM test_table WHERE id_col=$QUERY_INDEX;" >>$OPERATIONS
      TOT_QRS=$((TOT_QRS + 1))
    fi
  done
  ;;

5)
  echo "Workload Option 5: Mixed with pre-loading using one-by-one inserts"
  head $WORKLOAD_FILE -n $NUM_PRELOAD |
    while IFS=, read -r field1 field2; do
      echo "INSERT INTO test_table VALUES ($field1, '$field2');" >>$PRELOAD
    done
  TOT_INS=$NUM_PRELOAD
  TOT_QRS=0
  while [[ $TOT_INS -lt $N || $TOT_QRS -lt $NUM_QUERIES ]]; do
    FLIP=$((RANDOM % 2))
    if [[ ($TOT_INS -lt $N && $FLIP -ne 0) || ($TOT_QRS -ge $NUM_QUERIES && $FLIP -eq 0) ]]; then
      TOT_INS=$((TOT_INS + 1))
      while IFS=, read -r field1 field2; do
        echo "INSERT INTO test_table VALUES ($field1, '$field2');" >>$OPERATIONS
      done < <(sed "${TOT_INS}q;d" $WORKLOAD_FILE)
    else
      QUERY_INDEX=$(((RANDOM % N) + 1))
      echo "SELECT * FROM test_table WHERE id_col=$QUERY_INDEX;" >>$OPERATIONS
      TOT_QRS=$((TOT_QRS + 1))
    fi
  done
  ;;

*) echo "wrong option" ;;
esac

echo "Total number of preloads = $NUM_PRELOAD"
echo "Total number of inserts = $TOT_INS"
echo "Total number of queries = $TOT_QRS"

if [ $DB == "POSTGRES" ]; then
  psql -U postgres -f $DB_INIT
  time psql -U postgres -f $PRELOAD
  time psql -U postgres -f $OPERATIONS
elif [ $DB == "MONETDB" ]; then
  mclient -d db <$DB_INIT
  time mclient -d db <$PRELOAD
  time mclient -d db <$OPERATIONS
elif [ $DB == "MYSQL" ]; then
  mysql sortedness_benchmark <$DB_INIT
  time mysql sortedness_benchmark <$PRELOAD
  time mysql sortedness_benchmark <$OPERATIONS
fi

#!/bin/bash

export OUT_CSV=results.csv
export DB=POSTGRES
export N=16000000
export WORKLOAD_OPT=4
export NUM_QUERIES=3200000
export NUM_PERCENT=80
{
  K=0 L=5 ./benchmark.sh $WORKLOAD_OPT $NUM_PERCENT $NUM_QUERIES
  K=100 L=1 ./benchmark.sh $WORKLOAD_OPT $NUM_PERCENT $NUM_QUERIES
  K=50 L=1 ./benchmark.sh $WORKLOAD_OPT $NUM_PERCENT $NUM_QUERIES
  rm workloads/createdata_N${N}_K0_L5_*
  K=25 L=1 ./benchmark.sh $WORKLOAD_OPT $NUM_PERCENT $NUM_QUERIES
  rm workloads/createdata_N${N}_K100_L1_*
  K=10 L=1 ./benchmark.sh $WORKLOAD_OPT $NUM_PERCENT $NUM_QUERIES
  rm workloads/createdata_N${N}_K50_L1_*
  K=5 L=1 ./benchmark.sh $WORKLOAD_OPT $NUM_PERCENT $NUM_QUERIES
  rm workloads/createdata_N${N}_K25_L1_*
  K=1 L=1 ./benchmark.sh $WORKLOAD_OPT $NUM_PERCENT $NUM_QUERIES
  rm workloads/createdata_N${N}_K10_L1_*
  K=1 L=5 ./benchmark.sh $WORKLOAD_OPT $NUM_PERCENT $NUM_QUERIES
  rm workloads/createdata_N${N}_K5_L1_*
  K=1 L=10 ./benchmark.sh $WORKLOAD_OPT $NUM_PERCENT $NUM_QUERIES
  rm workloads/createdata_N${N}_K1_L1_*
  K=1 L=25 ./benchmark.sh $WORKLOAD_OPT $NUM_PERCENT $NUM_QUERIES
  rm workloads/createdata_N${N}_K1_L5_*
  K=1 L=50 ./benchmark.sh $WORKLOAD_OPT $NUM_PERCENT $NUM_QUERIES
  rm workloads/createdata_N${N}_K1_L10_*
  K=1 L=100 ./benchmark.sh $WORKLOAD_OPT $NUM_PERCENT $NUM_QUERIES
  rm workloads/createdata_N${N}_K1_L25_*
  K=10 L=5 ./benchmark.sh $WORKLOAD_OPT $NUM_PERCENT $NUM_QUERIES
  rm workloads/createdata_N${N}_K1_L50_*
  K=5 L=5 ./benchmark.sh $WORKLOAD_OPT $NUM_PERCENT $NUM_QUERIES
  rm workloads/createdata_N${N}_K1_L100_*
  K=5 L=10 ./benchmark.sh $WORKLOAD_OPT $NUM_PERCENT $NUM_QUERIES
  rm workloads/createdata_N${N}_K10_L5_*
  K=10 L=10 ./benchmark.sh $WORKLOAD_OPT $NUM_PERCENT $NUM_QUERIES
#  rm workloads/createdata_N${N}_K5_L5_*
#  rm workloads/createdata_N${N}_K5_L10_*
#  rm workloads/createdata_N${N}_K10_L10_*
} >>$OUT_CSV

#!/bin/bash

export OUT_CSV=results.csv
export DB=MYSQL
export N=16000000
{
  K=0 L=5 ./benchmark.sh 4 80 3200000
  K=1 L=1 ./benchmark.sh 4 80 3200000
  K=5 L=5 ./benchmark.sh 4 80 3200000
  K=10 L=10 ./benchmark.sh 4 80 3200000
  K=25 L=25 ./benchmark.sh 4 80 3200000
  K=50 L=50 ./benchmark.sh 4 80 3200000
  K=100 L=100 ./benchmark.sh 4 80 3200000
} >>$OUT_CSV

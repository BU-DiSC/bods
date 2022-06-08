#!/bin/bash

export DB=MYSQL
export N=1000
export OUT_CSV=results.csv
K=50 L=20 ./benchmark.sh 1
K=50 L=30 ./benchmark.sh 5 100 0
K=50 L=50 ./benchmark.sh 2
K=50 L=60 ./benchmark.sh 4 100 0
K=50 L=70 ./benchmark.sh 3 10 200
K=50 L=80 ./benchmark.sh 4 10 200

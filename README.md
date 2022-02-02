# sortedness-workload

Workload generator for VLDB 2022 submission: "OSM-Tree: A Sortedness-Aware Index"

## About the workload generator
This workload generator uses the K,L metric to denote near-sorted collections, where K is the number of unordered entries and L is the maximum displacement of an out-of-order entry from its actual/ideal position. 

The workload generator can create both binary and txt files. It is recommended to only use the txt extension to visualize workloads (as it is easier to read in python or other visualization tools). 

As of now, this workload generator only supports creating ingestion workloads of integers. The maximum number of entries it can correctly generate is 2 Billion.

## How to run

1. Use the "make" command to compile the workload generator cpp file 
2. Run the file using the following format: 
  ```c
  ./sortedness_workload_generator <#. entries to generate> <domain> <K as #. entries> <L as #. entries> <seed> <type> <directory>
  ```

For example, a sample ingestion workload to create 1M entries with K=L=100,000 (10% of 1M entries) will look like: 
```c
./sortedness_workload_generator 1000000 1000000 100000 100000 1 bin workloads
```
Here, we used "1" as a seed value, 1M as the domain and "bin" to create a binary file. We place this created workload in the "workloads/" directory. 

Note: You will need to create the directory to place the workload if it does not pre-exist. The workload generator will not create this directory automatically. 

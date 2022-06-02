# sortedness-workload

Workload generator for VLDB 2022 submission: "OSM-Tree: A Sortedness-Aware Index"

## About the workload generator
This workload generator uses the K,L metric to denote near-sorted collections, where K is the number of unordered entries and L is the maximum displacement of an out-of-order entry from its actual/ideal position. 

The workload generator can create both binary and txt files. It is recommended to only use the txt extension to visualize workloads (as it is easier to read in python or other visualization tools). 

As of now, this workload generator only supports creating ingestion workloads of integers. The maximum number of entries it can correctly generate is 2 Billion.

## How to run

### To run Workload generator
1. Use the "make" command to compile the workload generator cpp file 
2. Run the file using the following format: 
  ```c
  ./sortedness_workload_generator -N <#. entries to generate> -K <K%> -L <L%> -S <seed> -o <directory> -a <alpha> -b <beta> -P <payload_size_in_bytes>
  ```

For example, a sample ingestion workload to create 1M entries with K=L=100,000 (10% of 1M entries) will look like: 
```c
./sortedness_workload_generator -N 1000000 -K 10 -L 10 -S 1 -o ./workloads -a 2.5 -b 2.9 -P 252
```
Here, we used "1" as a seed value, and alpha=2.5 while beta=2.9 for the sortedness distribution.  We place this created workload in the "workloads/" directory, and use a payload size of 252 Bytes. 

This will by default generate a comma separated txt file with the key in the first column and a randomly generated payload (string) in the second column.

Note: You will need to create the directory to place the workload if it does not pre-exist. The workload generator will not create this directory automatically. 

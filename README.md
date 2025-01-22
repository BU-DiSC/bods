# BoDS: A Benchmark on Data Sortedness

BoDS is a benchmark to compare the performance of database systems in terms of index construction and navigation costs when operaitng on 
data ingested with variable sortedness. The benchmark contains a synthetic data generator that quantifies sortedness using the 
(K,L)-sortedness metric to create differently sorted data collections. At present, the benchmark supports testing on PostgreSQL, a popular
row store.

## System Requirements
1. C++ std=11 to compile the data generator
2. An installation of the C++ Boost library
3. PostgreSQL to run the benchmark

## About the data generator
This data generator uses the K,L metric to denote near-sorted collections, where K is the number of unordered entries and L is the maximum displacement of an out-of-order entry from its actual/ideal position. Given a K and L, the data generator creates a collection of (K,L)-sorted keys with a payload (string). The payload is a randomly generated string of user-defined length. 

The data generator further expands the (K,L)-sortedness metric to incorporate the distribution of the L parameter of the unordered entries using 
a generalized beta distribution with fixed bounds (supported by the Boost library). 

## How to run

### To run Data generator
1. Create the `workloads` directory using 
   ```shell
   mkdir workloads/
   ```
2. Create `build` directory and switch directories
    ```shell
    mkdir build/
    cd build/
    ```
3. Compile using `CMAKE`
    ```shell
    cmake ..
    make
    ```
3. Run the workload generator using the following format:
   ```shell
   ./sortedness_data_generator -N <entries to generate> -K <K%> -L <L%> -S <seed> -O <output_file> -a <alpha> -b <beta> -P <payload_size_in_bytes>
   ```

For example, a sample ingestion workload to create 1M entries with K=L=10 (10% of 1M entries) will look like:
```shell
./sortedness_data_generator -N 1000000 -K 10 -L 10 -S 1 -O ./workloads/createdata_N1000000_K10_L10_S1234_a1_b1_P4.txt -a 2.5 -b 2.9 -P 252
```
Here, we used "1234" as a seed value, and alpha=1 while beta=1 for the sortedness distribution.  We place this created workload in the "workloads/" directory, and use a payload size of 252 Bytes. Note, the data generator requires the name of the output file as input. By default, the benchmark 
uses the following format for nomenclature: 
```shell
createdata_N<num_entries>_K<k%>_L<l%>_S<seed_val>_a<alpha_val>_b<beta_val>_P<payload_size>.txt
```

This will by default generate a comma separated txt file with the key in the first column and a randomly generated payload (string) in the second column.

### To run the BoDS Benchmark
One can use the runloads.sh script as a controller to run the benchmark. This script calls the benchmarking script for a specific set of input values. 
To run the benchmark with multiple data collections or workload inputs, the benchmarking script (benchmark.sh) can be called repeatedly through 
the runloads.sh controller. 
Note: both runloads.sh or benchmark.sh do not require the data collection to be created apriori, and can create the data file themselves. However, if 
a data file pre-exists, a new file is not created and the pre-existing file is used.

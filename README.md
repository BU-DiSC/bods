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
4. Input workload description through args.toml
5. Run the workload generator using the following format:
   ```shell
   ./sortedness_data_generator -P <total partitions> -F <relative workload description path>
   ```
For example, a sample ingestion workload to create 1M entries with K=L=10 (10% of 1M entries) will look like:
```shell
./sortedness_data_generator -F ../src/args.toml
```
Here, we used "1234" as a seed value, and alpha=1 while beta=1 for the sortedness distribution.  We place this created workload in the "workloads/" directory, and use a payload size of 252 Bytes. Note, the data generator requires the name of the output file as input. By default, the benchmark 
uses the following format for nomenclature: 
```shell
createdata_N<num_entries>_K<k%>_L<l%>_S<seed_val>_a<alpha_val>_b<beta_val>_P<payload_size>.txt
```

The following workload description is used to input the previous values.

```toml
title = "Single Workload Description"

[global]
domain = 10000000

[[partition]]
start_index = 0
number_of_entries = 1000000
K = 10
L = 10
seed = 1234
alpha = 1
beta = 1
payload = 252
window_size = 1
output_file = "../workloads/createdata_N1000000_K10_L10_S1234_a1_b1_P252.txt"
is_fixed = false
is_binary = false
reverse_order = true

```

Note: the TOML format supports two new optional fields to control where
generated files are placed and whether filenames are auto-generated:

- `global.output_dir` (string): directory to use when auto-generating output filenames. Default is `../workloads`.
- `partition.auto_output_filename` (bool): when `true` the generator builds a filename using the pattern `createdata_N..._K..._L..._S..._a..._b..._P....txt` and writes it into `global.output_dir`. If `false`, the generator uses `partition.output_file`.

See `src/args.toml` for an example that includes comments showing these options.

This will by default generate a comma separated txt file with the key in the first column and a randomly generated payload (string) in the second column.

To run the data generator with mutiple workload descriptions, follow the format shown below in the sample completed workload description:
```toml
title = "Multiple Workload Descriptions"

[global]
domain = 50000

[[partition]]
start_index = 0
number_of_entries = 2000
K = 10
L = 10
seed = 1
alpha = 1
beta = 1
payload = 0
window_size = 1
output_file = "../workloads/createdata.txt"
is_fixed = false
is_binary = false
reverse_order = true

[[partition]]
start_index = 2000
number_of_entries = 3000
K = 10
L = 20
seed = 1
alpha = 1
beta = 1
payload = 0
window_size = 5
output_file = "../workloads/createdata.txt"
is_fixed = true
is_binary = false
reverse_order = false

[[partition]]
start_index = 5000
number_of_entries = 5000
K = 50
L = 10
seed = 1
alpha = 1
beta = 1
payload = 0
window_size = 10
output_file = "../workloads/createdata.txt"
is_fixed = false
is_binary = false
reverse_order = false
```

To run with these multiple partions, use the following script:
```shell
./sortedness_data_generator -P 3 -F ../src/args.toml
```

### To run the BoDS Benchmark
One can use the runloads.sh script as a controller to run the benchmark. This script calls the benchmarking script for a specific set of input values. 
To run the benchmark with multiple data collections or workload inputs, the benchmarking script (benchmark.sh) can be called repeatedly through 
the runloads.sh controller.
Note: both runloads.sh or benchmark.sh do not require the data collection to be created apriori, and can create the data file themselves. However, if 
a data file pre-exists, a new file is not created and the pre-existing file is used.

## Calculating various sortedness metrics 

**Analyze Runs Script**

- **File**: `analyze_runs_from_input.py`
- **Purpose**: Read a workload file (one value per line). If a line contains multiple comma-separated values, the script keeps the first value and discards the rest of the line. The script reports:
    - **Total runs**: number of monotonic runs (strictly increasing by default; use `--allow-equal` for non-decreasing runs).
    - **Run length distribution**: counts of run lengths, and summary stats (min, max, mean, median).
    - **Diff distribution**: distribution of differences between consecutive values, and summary stats (min, max, mean, median).
- **Usage**:

```bash
python3 analyze_runs_from_input.py -f workloads/<relative-workload-file>
```

- **Options**:
    - `-f, --file_name` (required): path to workload file (relative to repo root or absolute path)
    - `--allow-equal`: treat equal consecutive values as part of a run (non-decreasing runs)
    - `--top-k N`: show only the top-N most frequent entries in the run-length and diff distributions (N=0 means show all)

- **Example**:

```bash
python3 analyze_runs_from_input.py -f workloads/createdata_N1000_K10_L10_S1234_a1_b1_P252.txt --top-k 10
```

- **Notes**:
    - The script reports the number of lines containing more than one comma-separated value (these lines are truncated to the first value).
    - If the input file contains invalid (non-integer) lines, they are skipped and counted in the summary.
    - Use `--allow-equal` when you want non-decreasing runs rather than strictly increasing runs.

See `README_analyze_runs.md` for additional examples and sample output.


**Estimate K,L From Input**

- **File**: `estimate_k_l_from_input.py`
- **Purpose**: Read a file of integers (one per line) and print a compact workload description using the (K,L) metrics used by the benchmark.
- **Behavior**:
    - Parses newline-separated integers; raises an error if a line is non-integer or the file is empty.
    - Computes and prints: number of values `N`, start index `I` (minimum value), `K` (number of misplaced entries), `L` (maximum displacement), plus whether the input has a fixed window and the maximum window size.
- **Usage**:

```bash
python3 estimate_k_l_from_input.py -f <relative workload description path>
```

Both utilities are small analysis helpers intended for quick inspection of generated workloads in the `workloads/` directory. If you'd like, I can add JSON output flags or limit the size of printed distributions.

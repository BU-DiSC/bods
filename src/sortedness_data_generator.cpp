#include <spdlog/spdlog.h>

#include <algorithm>
#include <boost/math/distributions.hpp>
#include <cassert>
#include <cmath>
#include <cxxopts.hpp>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>
#include "toml.hpp"
using namespace toml;
using namespace boost::math;


struct BoDSConfig {
    int total_numbers;
    double K;
    double L;
    int seed_value;
    std::string output_file;
    double alpha;
    double beta;
    int domain_right;
    int window_size;
    int payload_size;
    bool fixed_window;
    int start_index;
    bool binary;
    bool reversed;
};

std::vector<BoDSConfig> parse_args(int argc, char *argv[]) {
    toml::table tbl;
    int partitions;
    cxxopts::Options options("bods", "BoDS: Benchmark on Data Sortedness");
    try {
        options.add_options()("P,partitions",
                              "Number of partitions",
                              cxxopts::value<int>())(
                                "F, toml_file", "Toml File Name", cxxopts::value<std::string>());
        auto result = options.parse(argc, argv);
        tbl = toml::parse_file(result["toml_file"].as<std::string>());
        partitions = result["partitions"].as<int>();
    } catch (const std::exception &e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        std::cerr << options.help() << std::endl;
        exit(EXIT_FAILURE);
    }
    std::vector<BoDSConfig> configs;
    BoDSConfig config;
    for(int i = 0; i < partitions; i++) {
        config.total_numbers = tbl["partition"][i]["number_of_entries"].value_or(10000);
        config.domain_right = tbl["global"]["domain"].value_or(30000);
        config.K = tbl["partition"][i]["K"].value_or(0);
        config.L = tbl["partition"][i]["L"].value_or(0);
        config.seed_value = tbl["partition"][i]["seed"].value_or(0);
        config.output_file = tbl["partition"][i]["output_file"].value_or("/workloads/createdata_partitions_3");
        config.alpha = tbl["partition"][i]["alpha"].value_or(0);
        config.beta = tbl["partition"][i]["beta"].value_or(0);
        config.window_size = tbl["partition"][i]["window_size"].value_or(1);
        config.payload_size = tbl["partition"][i]["payload"].value_or(0);
        config.fixed_window = tbl["partition"][i]["is_fixed"].value_or(false);
        config.binary = tbl["partition"][i]["is_binary"].value_or(false);
        config.reversed = tbl["partition"][i]["reverse_order"].value_or(false);
        config.start_index = tbl["partition"][i]["start_index"].value_or(0);
        if (config.binary && config.payload_size > 0) {
        std::cerr << "Not implemented: binary output cannot accompany "
                     "payload_size > 0"
                  << std::endl;
        exit(EXIT_FAILURE);
        }
        configs.push_back(config);
    }
        
    return configs;
}

int generate_beta_random_in_range(long position,
                                            int total_numbers, int L,
                                            double alpha, double beta) {
    // configure jump ranges. Note: Both are inclusive
    long low_jump = -L;
    long high_jump = L;

    // now, lets do some basic bound checks for the jumps

    if (position + high_jump >= total_numbers) {
        // max_pos can be (total_numbers - 1)
        // high jump should be (max_pos - curr_pos)
        high_jump = (total_numbers - 1) - position;
    }

    if (position + low_jump < 0) {
        // min_pos can be 0
        // low jump should be (curr-pos - min_pos) = curr_pos
        low_jump = -position;
    }

    // now start beta distribution generation
    // ------------------------------------------- //

    // first pick a number uniformly at random between 0 and 1
    double rand_from_unif = ((double)rand() / (RAND_MAX));

    beta_distribution<> dist(alpha, beta);
    // get a number between 0 and 1 according to beta distribution by using
    // inverse transform sampling
    double rand_from_dist = quantile(dist, rand_from_unif);

    // now, we transform this to the range of low_jump to high_jump
    long jump = low_jump + ((high_jump - low_jump) * rand_from_dist);

    // we want to return the swap position
    long ret = position + jump;

    // sanity check
    // assert(ret >= 0 && ret < total_numbers);
    if (ret < 0) {
        spdlog::error("ret = {}", ret);
        spdlog::error("position = {}\tlow = {}\thigh = {}\trand = {}", position,
                      low_jump, high_jump, rand_from_dist);
        spdlog::error("position + low_jump = {}", (position + low_jump));

        exit(0);
    } else if (ret >= total_numbers) {
        spdlog::error("ret is more than total_numbers = {}", ret);
        exit(0);
    }
    assert(ret >= 0 && ret < total_numbers);
    return ret;
}

// this function code from GeekForGeeks
double findMedian(std::vector<long> a, int n) {
    if (a.size() == 0) {
        return 0;
    }
    // If size of the arr[] is even
    if (n % 2 == 0) {
        // Applying nth_element
        // on n/2th index
        nth_element(a.begin(), a.begin() + n / 2, a.end());

        // Applying nth_element
        // on (n-1)/2 th index
        nth_element(a.begin(), a.begin() + (n - 1) / 2, a.end());

        // Find the average of value at
        // index N/2 and (N-1)/2
        return (double)(a[(n - 1) / 2] + a[n / 2]) / 2.0;
    }

    // If size of the arr[] is odd
    else {
        // Applying nth_element
        // on n/2
        nth_element(a.begin(), a.begin() + n / 2, a.end());

        // Value at index (N/2)th
        // is the median
        return (double)a[n / 2];
    }
}

std::vector<int> unique_randoms(int n, int t) {
    std::vector<int> arr(n);
    // Fill the vector with numbers from 1 to n
    std::iota(arr.begin(), arr.end(), 1);

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(arr.begin(), arr.end(), g);

    if (t > arr.size()) {
        throw std::invalid_argument(
            "t is larger than the number of elements in the array");
    }

    arr.resize(t);
    return arr;
}

void write_data_to_file(int data_len, int payload_size,
                        std::string &file_name, int *data,
                        bool is_reversed, bool binary = false) {
    if (binary) {
        std::ofstream out_file(file_name, std::ios::out | std::ios::binary);
        out_file.write(reinterpret_cast<const char *>(data),
                       sizeof(int) * data_len);
        out_file.close();

        return;
    }
    // Otherwise default to text output
    std::ofstream myfile1;
    myfile1.open(file_name, std::ios::app);
    if(is_reversed) {
        for (int idx = data_len - 1; idx >= 0; idx--) {
            if (payload_size == 0) {
                myfile1 << data[idx] << std::endl;
            } else
                myfile1 << data[idx] << ","
                        << std::string(payload_size, 'a' + (rand() % 26))
                        << std::endl;
        }
    }
    else {
        for (int idx = 0; idx < data_len; idx++) {
            if (payload_size == 0) {
                myfile1 << data[idx] << std::endl;
            } else
                myfile1 << data[idx] << ","
                        << std::string(payload_size, 'a' + (rand() % 26))
                        << std::endl;
        }
    }
    myfile1.close();
}

void generate_data(int total_numbers, int start_index,
                   int domain_right, int window_size,
                   bool fixed_window, double k, double L, int seed,
                   std::string &output_file, double alpha = 1.0,
                   double beta = 1.0, int payload_size = 252,
                   bool binary = false, bool reversed = false) {
    std::srand(seed);
    // fix K for new definition
    double K = k / 2.0;

    double p_outOfRange = K;

    int *array = new int[total_numbers];

    int desired_num_sources = total_numbers * p_outOfRange / 100.0;
    int l_absolute = total_numbers * L / 100.0;

    int noise_counter = 0;

    std::unordered_set<int> swaps;
    std::unordered_set<int> left_out_sources;
    int w = 0;

    spdlog::info("start index = {}", start_index);
    spdlog::info("max displacement (L) = {}", l_absolute);
    spdlog::info("# required swaps = {}", desired_num_sources);

    int min_l = l_absolute, max_l = 0;

    std::vector<long> l_values;

    int prev_value = start_index;
    for (int i = 0; i < total_numbers; i++) {
        // generate random number between 1 and window_size and add it to
        // prev_value
        if (fixed_window) {
            array[i] = prev_value + window_size;
        } else {
            beta_distribution<> dist(alpha, beta);
            double rand_from_unif = ((double)rand() / (RAND_MAX));
            double rand_from_dist = quantile(dist, rand_from_unif);
            array[i] = prev_value + (rand_from_dist * window_size) + 1;
        }
        prev_value = array[i];
    }

    // generate desired_num_sources number of unique random numbers
    int ctr = 0;
    std::default_random_engine e(seed);

    // generate desired_num_sources number of unique random numbers that are
    // source indexes in original array for swaps
    std::vector<int> v =
        unique_randoms(total_numbers, desired_num_sources);
    std::unordered_set<int> source_set(v.begin(), v.end());
    spdlog::info("Generated # sources = {}", source_set.size());

    // We first ensure that the max displacement is L for at least one swap.
    int generated_source;
    int num_tries_for_max_displacement = 10;
    int tries_ctr = 0;
    bool generated_with_max_displacement = false;
    for (int i : source_set) {
        if (tries_ctr > num_tries_for_max_displacement) {
            break;
        }

        int r = i + l_absolute;

        if (r > total_numbers) {
            r = i - l_absolute;

            // check if this becomes a problem on the other side
            if (i < l_absolute) {
                // now, we have a problem
                // r would have been set to something garbage

                // since we have now found out that going forward and backward
                // is creating problems, we continue; however, we control this
                tries_ctr++;
                continue;
            }
        }

        // make sure r not another source for a swap
        if (source_set.find(r) != source_set.end()) {
            // if indeed this is a source, move on to the next source and then
            // pick a swap at L positions
            continue;
        }
        swaps.insert(r);

        if (abs(int(r - i)) < min_l)
            min_l = abs(int(r - i));
        else if (abs(int(r - i)) > max_l)
            max_l = abs(int(r - i));

        int temp = array[i];
        array[i] = array[r];
        array[r] = temp;

        noise_counter++;

        generated_source = i;

        l_values.push_back(r - i);
        generated_with_max_displacement = true;

        break;
    }

    if (generated_with_max_displacement)
        spdlog::info("Displaced at least one entry by {} positions...",
                     l_absolute);

    for (int i : source_set) {
        // check if this position is already the first generated source
        if (generated_source == i) continue;

        int num_tries = 128;
        while (num_tries > 0) {
            int r = generate_beta_random_in_range(
                i, total_numbers, l_absolute, alpha, beta);
            num_tries--;

            // check for cascading swaps, i.e. we should not pick a spot again
            // to swap also we should not pick one of our already defined source
            // places
            if (((L == 100) && (r == i)) ||
                ((L != 100) && ((r == i) || swaps.find(r) != swaps.end() ||
                                source_set.find(r) != source_set.end()))) {
                // if we ran out of tries
                if (num_tries == 0) {
                    // we will add this to left_out_sources
                    left_out_sources.insert(i);
                }
                continue;
            }

            else {
                swaps.insert(r);

                if (abs(int(r - i)) < min_l)
                    min_l = abs(int(r - i));
                else if (abs(int(r - i)) > max_l)
                    max_l = abs(int(r - i));

                int temp = array[i];
                array[i] = array[r];
                array[r] = temp;

                noise_counter++;

                l_values.push_back(abs(int(r - i)));
                break;
            }
        }

        if (noise_counter == desired_num_sources) break;
    }

    spdlog::info("Swaps completed in first attempt: {}", noise_counter);
    spdlog::info("Left out sources: {}", left_out_sources.size());

    // we potentially have left out sources
    // loop through them again and try another set of random jumps
    for (auto it = left_out_sources.begin(); it != left_out_sources.end();) {
        int r;
        int i = *it;

        int num_tries = 512;
        bool found = false;
        while (num_tries > 0) {
            // r = generate_random_in_range(i, total_numbers, l_absolute);
            r = generate_beta_random_in_range(i, total_numbers, l_absolute,
                                              alpha, beta);
            num_tries--;
            // check for cascading swaps, i.e. we should not pick a spot again
            // to swap also we should not pick one of our already defined source
            // places if ((r == i) || swaps.find(r) != swaps.end() ||
            // source_set.find(r) != source_set.end())
            if (((L == 100) && (r == i)) ||
                ((L != 100) && ((r == i) || swaps.find(r) != swaps.end() ||
                                source_set.find(r) != source_set.end()))) {
                // if we run out of tries, we keep the element in the
                // left_out_sources
                continue;
            } else {
                // if we found an eligible swap
                swaps.insert(r);

                if (abs(int(r - i)) < min_l)
                    min_l = abs(int(r - i));
                else if (abs(int(r - i)) > max_l)
                    max_l = abs(int(r - i));

                int temp = array[i];
                array[i] = array[r];
                array[r] = temp;

                noise_counter++;

                // remove the current source from left out sources
                it = left_out_sources.erase(it);
                found = true;

                l_values.push_back(abs(int(r - i)));

                // we can break out of the loop
                break;
            }
        }

        // manually increment iterator only if found is false
        // else, the erase operation would have automatically moved the iterator
        // ahead
        if (!found) {
            ++it;
        }
    }

    if (left_out_sources.size() > 0) {
        spdlog::critical(
            "Left out sources after re-trying with increased jumps = {}",
            left_out_sources.size());
    }

    // now let us give one final try with brute force
    // but we want to avoid this if K=100% (50% swaps needed) and we are within
    // 1% of left overs
    if (left_out_sources.size() > 0 &&
        !(K == 50 && left_out_sources.size() <= 0.01 * desired_num_sources)) {
        spdlog::info("Trying Brute force swaps for left out sources...");

        for (auto iter = left_out_sources.begin();
             iter != left_out_sources.end();) {
            int position = *iter;
            int start;
            int end;

            bool found = false;

            // start position should usually be (position - l) and end should be
            // (position + l)
            //  however, we need to check for edge cases
            float ran =
                static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

            bool move_forward = true;

            if (ran < 0.5) {
                // we move from start to end
                // if positon - l_absolute < 0
                if (position < l_absolute) {
                    start = 0;
                } else {
                    start = position - l_absolute;
                }

                if (long(position + l_absolute) > total_numbers) {
                    end = total_numbers - 1;
                } else {
                    end = position + l_absolute;
                }
            } else {
                // we move from end to start
                // if positon - l_absolute < 0
                if (position < l_absolute) {
                    end = 0;
                } else {
                    end = position - l_absolute;
                }

                if (long(position + l_absolute) > total_numbers) {
                    start = total_numbers - 1;
                } else {
                    start = position + l_absolute;
                }

                move_forward = false;
            }

            // now, loop through from start to end
            // we will pick the first valid swap spot
            for (int r = start; r != end;) {
                // if (r == position || swaps.find(r) != swaps.end() ||
                // source_set.find(r)
                // != source_set.end())
                if (((L == 100) && (r == position)) ||
                    ((L != 100) &&
                     ((r == position) || swaps.find(r) != swaps.end() ||
                      source_set.find(r) != source_set.end()))) {
                    // stopping condition
                    if (r == end) break;
                    if (move_forward)
                        r++;
                    else
                        r--;
                    continue;
                } else {
                    // if we found an eligible swap
                    swaps.insert(r);

                    if (abs(int(r - position)) < min_l)
                        min_l = abs(int(r - position));
                    else if (abs(int(r - position)) > max_l)
                        max_l = abs(int(r - position));

                    int temp = array[position];
                    array[position] = array[r];
                    array[r] = temp;

                    noise_counter++;

                    // remove the current source from left out sources
                    iter = left_out_sources.erase(iter);
                    found = true;

                    l_values.push_back(abs(int(r - position)));

                    // we can break out of the loop
                    break;
                }
            }

            // manually increment iterator only if found is false
            // else, the erase operation would have automatically moved the
            // iterator ahead
            if (!found) {
                ++iter;
            }
        }
    } else {
        spdlog::info(
            "Skipping brute force as we are within 1% of achieving our "
            "desired number of swaps...");
    }

    if (left_out_sources.size() > 0) {
        spdlog::critical(
            "Left out sources after brute force attempt (may not have "
            "executed) = {}",
            left_out_sources.size());
    }

    spdlog::info("************ Final Statistics:");
    spdlog::info("Swaps made = {}", noise_counter);
    spdlog::info("Min L = {}", min_l);
    spdlog::info("Max L = {}", max_l);

    double median_l = findMedian(l_values, l_values.size());
    spdlog::info("Median L = {}", median_l);
    write_data_to_file(total_numbers, payload_size, output_file, array, reversed, binary);
}

void generate_one_file(BoDSConfig config) {
    generate_data(config.total_numbers, config.start_index, config.domain_right,
                  config.window_size, config.fixed_window, config.K, config.L,
                  config.seed_value, config.output_file, config.alpha,
                  config.beta, config.payload_size, config.binary, config.reversed);
}

int main(int argc, char **argv) {
    int partitions;
    cxxopts::Options options("bods", "BoDS: Benchmark on Data Sortedness");
    try {
        options.add_options()("P,partitions",
                              "Number of partitions",
                              cxxopts::value<int>())(
                                "F, toml_file", "Toml File Name", cxxopts::value<std::string>());
        auto result = options.parse(argc, argv);
        partitions = result["partitions"].as<int>();
    } catch (const std::exception &e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        std::cerr << options.help() << std::endl;
        exit(EXIT_FAILURE);
    }
    auto configs = parse_args(argc, argv);
    for(int i = 0; i < partitions; i++) {
        if ((configs[i].window_size * configs[i].total_numbers) > configs[i].domain_right) {
            std::cerr << "Window size too large for domain and total entries"
                  << std::endl;
            return 1;
        }
        generate_one_file(configs[i]);
    }
    return 0;
}

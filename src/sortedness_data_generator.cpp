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

using namespace boost::math;

struct BoDSConfig {
    unsigned long totalNumbers;
    double K;
    double L;
    int seedValue;
    std::string outputFile;
    double alpha;
    double beta;
    unsigned long domain_right;
    int window_size;
    int payload_size;
    bool fixed_window;
    unsigned long start_index;
    bool binary;
};

BoDSConfig parse_args(int argc, char *argv[]) {
    BoDSConfig config;
    cxxopts::Options options("bods", "BoDS: Benchmark on Data Sortedness");
    try {
        options.add_options()("N,total_entries",
                              "Total number of entries to generate",
                              cxxopts::value<unsigned long>())(
            "O,output_file", "Output file", cxxopts::value<std::string>())(
            "D,domain", "Domain size (end from 0)",
            cxxopts::value<unsigned long>())(
            "K,k_pt", "% of out of order entries",
            cxxopts::value<double>()->default_value("0"))(
            "L,l_pt", "Maximum displacement of entries as %",
            cxxopts::value<double>()->default_value("0"))(
            "S,seed", "Seed Value", cxxopts::value<int>()->default_value("0"))(
            "a,alpha", "Alpha Value",
            cxxopts::value<double>()->default_value("0"))(
            "b,beta", "Beta Value",
            cxxopts::value<double>()->default_value("0"))(
            "W,window", "Window size",
            cxxopts::value<int>()->default_value("1"))(
            "F,fixed", "Fixed window size",
            cxxopts::value<bool>()->default_value("false"))(
            "B,binary", "File output binary format",
            cxxopts::value<bool>()->default_value("false"))(
            "P,payload", "Payload Size in Bytes",
            cxxopts::value<int>()->default_value("0"))(
            "I,start", "Start Index",
            cxxopts::value<unsigned long>()->default_value("0"));
        auto result = options.parse(argc, argv);
        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            exit(0);
        }
        if (result.count("total_entries") == 0 || result.count("k_pt") == 0 ||
            result.count("l_pt") == 0 || result.count("seed") == 0 ||
            result.count("output_file") == 0 || result.count("alpha") == 0 ||
            result.count("beta") == 0 || result.count("domain") == 0 ||
            result.count("window") == 0 || result.count("payload") == 0) {
            std::cerr << "Missing required arguments" << std::endl;
            std::cerr << options.help() << std::endl;
            exit(1);
        }
        config.totalNumbers = result["total_entries"].as<unsigned long>();
        config.K = result["k_pt"].as<double>();
        config.L = result["l_pt"].as<double>();
        config.seedValue = result["seed"].as<int>();
        config.outputFile = result["output_file"].as<std::string>();
        config.alpha = result["alpha"].as<double>();
        config.beta = result["beta"].as<double>();
        config.domain_right = result["domain"].as<unsigned long>();
        config.window_size = result["window"].as<int>();
        config.payload_size = result["payload"].as<int>();
        config.fixed_window = result["fixed"].as<bool>();
        config.binary = result["binary"].as<bool>();
        config.start_index = result["start"].as<unsigned long>();
    } catch (const std::exception &e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        std::cerr << options.help() << std::endl;
        exit(EXIT_FAILURE);
    }

    if (config.binary && config.payload_size > 0) {
        std::cerr << "Not implemented: binary output cannot accompany "
                     "payload_size > 0"
                  << std::endl;
        std::cerr << options.help() << std::endl;
        exit(EXIT_FAILURE);
    }
    return config;
}

unsigned long generate_beta_random_in_range(long position,
                                            unsigned long Total_Numbers, int L,
                                            double alpha, double beta) {
    // configure jump ranges. Note: Both are inclusive
    long low_jump = -L;
    long high_jump = L;

    // now, lets do some basic bound checks for the jumps

    if (position + high_jump >= Total_Numbers) {
        // max_pos can be (Total_Numbers - 1)
        // high jump should be (max_pos - curr_pos)
        high_jump = (Total_Numbers - 1) - position;
    }

    if (position + low_jump < 0) {
        // min_pos can be 0
        // low jump should be (curr-pos - min_pos) = curr_pos
        low_jump = -position;
    }

    // now start beta distribution generation
    // ------------------------------------------- //

    // first pick a number uniformly at random between 0 and 1
    double randFromUnif = ((double)rand() / (RAND_MAX));

    beta_distribution<> dist(alpha, beta);
    // get a number between 0 and 1 according to beta distribution by using
    // inverse transform sampling
    double randFromDist = quantile(dist, randFromUnif);

    // now, we transform this to the range of low_jump to high_jump
    long jump = low_jump + ((high_jump - low_jump) * randFromDist);

    // we want to return the swap position
    long ret = position + jump;

    // sanity check
    // assert(ret >= 0 && ret < Total_Numbers);
    if (ret < 0) {
        spdlog::error("ret = {}", ret);
        spdlog::error("position = {}\tlow = {}\thigh = {}\trand = {}", position,
                      low_jump, high_jump, randFromDist);
        spdlog::error("position + low_jump = {}", (position + low_jump));

        exit(0);
    } else if (ret >= Total_Numbers) {
        spdlog::error("ret is more than total_numbers = {}", ret);
        exit(0);
    }
    assert(ret >= 0 && ret < Total_Numbers);
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

std::vector<unsigned long> unique_randoms(unsigned long n, unsigned long t) {
    std::vector<unsigned long> arr(n);
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

void write_data_to_file(unsigned long data_len, int payload_size,
                        std::string &file_name, unsigned long *data,
                        bool binary = false) {
    if (binary) {
        std::ofstream out_file(file_name, std::ios::out | std::ios::binary);
        out_file.write(reinterpret_cast<const char *>(data),
                       sizeof(unsigned long) * data_len);
        out_file.close();

        return;
    }
    // Otherwise default to text output
    std::ofstream myfile1(file_name);
    for (unsigned long idx = 0; idx < data_len; idx++) {
        if (payload_size == 0) {
            myfile1 << data[idx] << std::endl;
        } else
            myfile1 << data[idx] << ","
                    << std::string(payload_size, 'a' + (rand() % 26))
                    << std::endl;
    }
    myfile1.close();
}

void generate_data(unsigned long TOTAL_NUMBERS, unsigned long start_index,
                   unsigned long domain_right, int window_size,
                   bool fixed_window, double k, double L, int seed,
                   std::string &outputFile, double alpha = 1.0,
                   double beta = 1.0, int payload_size = 252,
                   bool binary = false) {
    std::srand(seed);
    // fix K for new definition
    double K = k / 2.0;

    double p_outOfRange = K;

    unsigned long *array = new unsigned long[TOTAL_NUMBERS];

    unsigned long desired_num_sources = TOTAL_NUMBERS * p_outOfRange / 100.0;
    unsigned long l_absolute = TOTAL_NUMBERS * L / 100.0;

    unsigned long noise_counter = 0;

    std::unordered_set<unsigned long> swaps;
    std::unordered_set<unsigned long> left_out_sources;
    unsigned long w = 0;

    spdlog::info("start index = {}", start_index);
    spdlog::info("max displacement (L) = {}", l_absolute);
    spdlog::info("# required swaps = {}", desired_num_sources);

    unsigned long min_l = l_absolute, max_l = 0;

    std::vector<long> l_values;

    unsigned long prev_value = start_index;
    for (unsigned long i = 0; i < TOTAL_NUMBERS; i++) {
        // generate random number between 1 and window_size and add it to
        // prev_value
        if (fixed_window) {
            array[i] = prev_value + window_size;
        } else {
            array[i] = prev_value + (rand() % window_size) + 1;
        }
        prev_value = array[i];
    }

    // generate desired_num_sources number of unique random numbers
    int ctr = 0;
    std::default_random_engine e(seed);

    // generate desired_num_sources number of unique random numbers that are
    // source indexes in original array for swaps
    std::vector<unsigned long> v =
        unique_randoms(TOTAL_NUMBERS, desired_num_sources);
    std::unordered_set<unsigned long> source_set(v.begin(), v.end());
    spdlog::info("Generated # sources = {}", source_set.size());

    // We first ensure that the max displacement is L for at least one swap.
    unsigned long generated_source;
    int num_tries_for_max_displacement = 10;
    int tries_ctr = 0;
    bool generated_with_max_displacement = false;
    for (unsigned long i : source_set) {
        if (tries_ctr > num_tries_for_max_displacement) {
            break;
        }

        unsigned long r = i + l_absolute;

        if (r > TOTAL_NUMBERS) {
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

        unsigned long temp = array[i];
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

    for (unsigned long i : source_set) {
        // check if this position is already the first generated source
        if (generated_source == i) continue;

        int num_tries = 128;
        while (num_tries > 0) {
            unsigned long r = generate_beta_random_in_range(
                i, TOTAL_NUMBERS, l_absolute, alpha, beta);
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

                unsigned long temp = array[i];
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
        unsigned long r;
        unsigned long i = *it;

        int num_tries = 512;
        bool found = false;
        while (num_tries > 0) {
            // r = generate_random_in_range(i, TOTAL_NUMBERS, l_absolute);
            r = generate_beta_random_in_range(i, TOTAL_NUMBERS, l_absolute,
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

                unsigned long temp = array[i];
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
    for (auto iter = left_out_sources.begin();
         iter != left_out_sources.end();) {
        unsigned long position = *iter;
        // unsigned long start = position - l_absolute;
        // unsigned long end = position + l_absolute;
        unsigned long start;
        unsigned long end;

        bool found = false;

        // start position should usually be (position - l) and end should be
        // (position + l)
        //  however, we need to check for edge cases
        float ran = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

        bool move_forward = true;

        if (ran < 0.5) {
            // we move from start to end
            // if positon - l_absolute < 0
            if (position < l_absolute) {
                start = 0;
            } else {
                start = position - l_absolute;
            }

            if (long(position + l_absolute) > TOTAL_NUMBERS) {
                end = TOTAL_NUMBERS - 1;
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

            if (long(position + l_absolute) > TOTAL_NUMBERS) {
                start = TOTAL_NUMBERS - 1;
            } else {
                start = position + l_absolute;
            }

            move_forward = false;
        }

        // now, loop through from start to end
        // we will pick the first valid swap spot
        for (unsigned long r = start; r != end;) {
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

                unsigned long temp = array[position];
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
        // else, the erase operation would have automatically moved the iterator
        // ahead
        if (!found) {
            ++iter;
        }
    }

    if (left_out_sources.size() > 0) {
        spdlog::critical("Left out sources after brute force = {}",
                         left_out_sources.size());
    }

    spdlog::info(
        "*******************************************************"
        "\n\t\t\t\tFinal "
        "Statistics:");
    spdlog::info("Swaps made = {}", noise_counter);
    spdlog::info("Min L = {}", min_l);
    spdlog::info("Max L = {}", max_l);

    double median_l = findMedian(l_values, l_values.size());
    spdlog::info("Median L = {}", median_l);

    write_data_to_file(TOTAL_NUMBERS, payload_size, outputFile, array, binary);
}

void generate_one_file(BoDSConfig config) {
    generate_data(config.totalNumbers, config.start_index, config.domain_right,
                  config.window_size, config.fixed_window, config.K, config.L,
                  config.seedValue, config.outputFile, config.alpha,
                  config.beta, config.payload_size, config.binary);
}

// arguments to program:
// unsigned long pTOTAL_NUMBERS, unsigned int pdomain, unsigned long windowSize,
// short k, int pseed,

int main(int argc, char **argv) {
    auto config = parse_args(argc, argv);
    if ((config.window_size * config.totalNumbers) > config.domain_right) {
        std::cerr << "Window size too large for domain and total entries"
                  << std::endl;
        return 1;
    }
    generate_one_file(config);
    return 0;
}

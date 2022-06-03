#include <fstream>
#include <ctime>
#include <algorithm>
#include <vector>
#include <iostream>
#include <unordered_set>
#include <random>
#include <iomanip>
#include <cassert>
#include <chrono>
#include <cmath>
#include <boost/math/distributions.hpp>
#include <vector>
#include <string>
#include "args.hxx"

using namespace boost::math;

inline bool ledger_exists();
void generate_one_file(unsigned long pTOTAL_NUMBERS, int k, int l, int pseed, std::string folder, std::string type, double alpha, double beta, int payload_size);
unsigned int get_number_domain(unsigned long position, unsigned long total, unsigned long domain_);
std::string generate_partitions_stream(unsigned long TOTAL_NUMBERS, int K, int L, int seed, std::string folder, std::string type, double alpha, double beta, int payload_size);

std::string generateValue(int value_size)
{
    std::string value = std::string(value_size, 'a' + (rand() % 26));
    return value;
}

inline bool ledger_exists()
{
    std::ifstream f("dataledger.txt");
    return f.good();
}

void generate_one_file(unsigned long pTOTAL_NUMBERS, int k, int l, int pseed, std::string folder, std::string type, double alpha, double beta, int payload_size)
{
    std::ofstream outfile;

    srand(time(NULL));
    outfile.open("dataledger.txt", std::ios_base::app);

    std::string folder_name = "./";

    outfile << generate_partitions_stream(pTOTAL_NUMBERS, k, l, pseed, folder, type, alpha, beta, payload_size) << std::endl;

    outfile.close();
}

unsigned int get_number_domain(unsigned long position, unsigned long total, unsigned int domain_)
{
    return (position * domain_) / total;
}

unsigned long generate_beta_random_in_range(long position, unsigned long Total_Numbers, int L, double alpha, double beta)
{
    int l = L;

    long ret;

    unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
    std::default_random_engine e(seed);

    // configure jump ranges. Note: Both are inclusive
    long low_jump = l * -1;
    long high_jump = l;

    // now, lets do some basic bound checks for the jumps

    if (position + high_jump >= Total_Numbers)
    {
        // max_pos can be (Total_Numbers - 1)
        // high jump should be (max_pos - curr_pos)
        high_jump = (Total_Numbers - 1) - position;
    }

    if (position + low_jump < 0)
    {
        // min_pos can be 0
        // low jump should be (curr-pos - min_pos) = curr_pos
        // std::cout << "low jump!\t" << position << "\t" << low_jump << std::endl;
        low_jump = -1 * position;
    }

    // now start beta distribution generation
    // ------------------------------------------- //

    // first pick a number uniformly at random between 0 and 1
    double randFromUnif = ((double)rand() / (RAND_MAX));

    beta_distribution<> dist(alpha, beta);
    // get a number between 0 and 1 according to beta distribution by using inverse transform sampling
    double randFromDist = quantile(dist, randFromUnif);

    // now, we transform this to the range of low_jump to high_jump
    long jump = low_jump + ((high_jump - low_jump) * randFromDist);

    // we want to return the swap position
    ret = position + jump;

    // sanity check
    // assert(ret >= 0 && ret < Total_Numbers);
    if (ret < 0)
    {
        std::cout << "ret = " << ret << std::endl;
        std::cout << "position = " << position << "\tlow = " << low_jump << "\thigh = " << high_jump << "\trand = " << randFromDist << std::endl;
        std::cout << (position + low_jump) << std::endl;
        exit(0);
    }
    else if (ret >= Total_Numbers)
    {
        std::cout << "ret more = " << ret << std::endl;
        exit(0);
    }
    assert(ret >= 0 && ret < Total_Numbers);
    return ret;
}

// this function code from GeekForGeeks
double findMedian(std::vector<long> a,
                  int n)
{

    // If size of the arr[] is even
    if (n % 2 == 0)
    {

        // Applying nth_element
        // on n/2th index
        nth_element(a.begin(),
                    a.begin() + n / 2,
                    a.end());

        // Applying nth_element
        // on (n-1)/2 th index
        nth_element(a.begin(),
                    a.begin() + (n - 1) / 2,
                    a.end());

        // Find the average of value at
        // index N/2 and (N-1)/2
        return (double)(a[(n - 1) / 2] + a[n / 2]) / 2.0;
    }

    // If size of the arr[] is odd
    else
    {

        // Applying nth_element
        // on n/2
        nth_element(a.begin(),
                    a.begin() + n / 2,
                    a.end());

        // Value at index (N/2)th
        // is the median
        return (double)a[n / 2];
    }
}

/*
    Function which generates uniform data over some domain, and write it in binary format.
    Each partition of L elements is shuffled, and has some noise (randomness) linked to the
    percent_outRange parameter.
    */
std::string generate_partitions_stream(unsigned long TOTAL_NUMBERS, int K, int L, int seed, std::string folder = "./Data", std::string type = "bin", double alpha = 1.0, double beta = 1.0, int payload_size = 252)
{

    float p_outOfRange = K;

    std::srand(seed);

    unsigned long *array = new unsigned long[TOTAL_NUMBERS];

    std::string f1name = folder;
    f1name += "/createdata_";
    f1name += std::to_string(TOTAL_NUMBERS);
    f1name += "-elems_";
    f1name += std::to_string(K);
    f1name += "-K_";
    f1name += std::to_string(L);
    f1name += "-L_";
    f1name += std::to_string(seed);
    f1name += "seed";
    f1name += std::to_string(std::time(nullptr));

    std::ofstream myfile1;

    f1name += ".txt";
    myfile1.open(f1name);

    unsigned long noise_limit = TOTAL_NUMBERS * p_outOfRange / 100.0;
    unsigned long l_absolute = TOTAL_NUMBERS * L / 100.0;

    unsigned long noise_counter = 0;

    std::unordered_set<unsigned long> myset;
    std::unordered_set<unsigned long> swaps;
    std::unordered_set<unsigned long> left_out_sources;
    unsigned long w = 0;
    unsigned long windowSize = 1;

    std::cout << "L% = " << L << std::endl;
    std::cout << "L absolute = " << l_absolute << std::endl;
    std::cout << "No. of Required Swaps = " << noise_limit << std::endl;
    unsigned long min_l = l_absolute, max_l = 0;

    std::vector<long> l_values;

    for (unsigned long i = 0; i < TOTAL_NUMBERS; i++, w += windowSize)
    {
        array[i] = i;
    }

    // generate noise_limit number of unique random numbers
    int ctr = 0;
    // std::vector<unsigned int> ks;
    while (ctr < noise_limit)
    {
        unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
        std::default_random_engine e(seed);

        std::uniform_int_distribution<unsigned long> distr(0, TOTAL_NUMBERS - 1);
        unsigned long i = distr(e);

        if (myset.find(i) != myset.end())
        {
            continue;
        }
        else
        {
            myset.insert(i);
            if (i < 0)
            {
                std::cout << "neg" << std::endl;
            }
            // ks.push_back(i);
            ctr++;
        }
    }

    std::cout << "No. of sources generated for swaps = " << ctr << std::endl;

    // first, we want to ensure that the max displacement is L
    // this means at least one swap has to happen by L positions
    unsigned long generated_source;
    for (auto iterator = myset.begin(); iterator != myset.end(); iterator++)
    {
        unsigned long i = *iterator;
        unsigned long r = i + l_absolute;

        if (r > TOTAL_NUMBERS)
            r = i - l_absolute;

        // std::cout << "r = " << r << std::endl;

        // make sure r not another source for a swap
        if (myset.find(r) != myset.end())
        {
            // if indeed this is a source, move on to the next source and then pick a swap at L positions
            continue;
        }
        swaps.insert(r);
        if (r == 0)
        {
            std::cout << "swap 0 (1)" << std::endl;
        }
        // max_l = r - i;
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

        break;
    }

    std::cout << "We now have at least one element with L displacement..." << std::endl;

    // since the first source has already been taken care of, simply start from the next
    auto itr = myset.begin();

    for (; itr != myset.end(); itr++)
    {
        unsigned long r;
        unsigned long i = *itr;

        // check if this position is already the first generated source
        if (generated_source == i)
            continue;
        // std::cout << i << " ";
        int num_tries = 4;
        while (num_tries > 0)
        {
            // r = generate_random_in_range(i, TOTAL_NUMBERS, l_absolute);
            r = generate_beta_random_in_range(i, TOTAL_NUMBERS, l_absolute, alpha, beta);
            num_tries--;

            // check for cascading swaps, i.e. we should not pick a spot again to swap
            // also we should not pick one of our already defined source places
            if ((r == i) || swaps.find(r) != swaps.end() || myset.find(r) != myset.end())
            {
                // if we ran out of tries
                if (num_tries == 0)
                {
                    // we will add this to left_out_sources
                    left_out_sources.insert(i);
                }
                continue;
            }

            else
            {
                // std::cout << "found swap" << std::endl;
                swaps.insert(r);

                if (r == 0)
                {
                    std::cout << "swap 0 (2)" << std::endl;
                }

                if (r == i)
                {
                    std::cout << "same place" << std::endl;
                }

                if (abs(int(r - i)) < min_l)
                    min_l = abs(int(r - i));
                else if (abs(int(r - i)) > max_l)
                    max_l = abs(int(r - i));

                if (array[i] == 0 || array[r] == 0)
                {
                    std::cout << "here (1)" << std::endl;
                }
                unsigned long temp = array[i];
                array[i] = array[r];
                array[r] = temp;

                noise_counter++;

                l_values.push_back(abs(int(r - i)));
                break;
            }
        }
        if (noise_counter == noise_limit)
            break;

        // std::cout << noise_counter << std::endl;
    }

    std::cout << "Left out sources = " << left_out_sources.size() << std::endl;

    // we potentially have left out sources
    // loop through them again and try another set of random jumps
    for (auto it = left_out_sources.begin(); it != left_out_sources.end();)
    {
        unsigned long r;
        unsigned long i = *it;

        int num_tries = 16;
        bool found = false;
        while (num_tries > 0)
        {
            // r = generate_random_in_range(i, TOTAL_NUMBERS, l_absolute);
            r = generate_beta_random_in_range(i, TOTAL_NUMBERS, l_absolute, alpha, beta);
            num_tries--;
            // check for cascading swaps, i.e. we should not pick a spot again to swap
            // also we should not pick one of our already defined source places
            if ((r == i) || swaps.find(r) != swaps.end() || myset.find(r) != myset.end())
            {
                // if we run out of tries, we keep the element in the left_out_sources
                continue;
            }
            else
            {
                // if we found an eligible swap
                swaps.insert(r);

                if (r == 0)
                {
                    std::cout << "swap 0 (3)" << std::endl;
                }

                if (abs(int(r - i)) < min_l)
                    min_l = abs(int(r - i));
                else if (abs(int(r - i)) > max_l)
                    max_l = abs(int(r - i));

                if (array[i] == 0 || array[r] == 0)
                {
                    std::cout << "here (2)" << std::endl;
                }

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
        // else, the erase operation would have automatically moved the iterator ahead
        if (!found)
        {
            ++it;
        }
    }

    std::cout << "Left out sources after increased jumps = " << left_out_sources.size() << std::endl;

    // now let us give one final try with brute force
    for (auto iter = left_out_sources.begin(); iter != left_out_sources.end();)
    {
        unsigned long position = *iter;
        // unsigned long start = position - l_absolute;
        // unsigned long end = position + l_absolute;
        unsigned long start;
        unsigned long end;

        bool found = false;

        // start position should usually be (position - l) and end should be (position + l)
        //  however, we need to check for edge cases

        float ran = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

        long forward_move_value = 1;
        bool move_forward = true;

        if (ran < 0.5)
        {
            // we move from start to end
            // if positon - l_absolute < 0
            if (position < l_absolute)
            {
                start = 0;
            }
            else
            {
                start = position - l_absolute;
            }

            if (long(position + l_absolute) > TOTAL_NUMBERS)
            {
                end = TOTAL_NUMBERS - 1;
            }
            else
            {
                end = position + l_absolute;
            }
        }
        else
        {

            // we move from end to start
            // if positon - l_absolute < 0
            if (position < l_absolute)
            {
                end = 0;
            }
            else
            {
                end = position - l_absolute;
            }

            if (long(position + l_absolute) > TOTAL_NUMBERS)
            {
                start = TOTAL_NUMBERS - 1;
            }
            else
            {
                start = position + l_absolute;
            }

            forward_move_value = -1;
            move_forward = false;
        }

        // now, loop through from start to end
        // we will pick the first valid swap spot
        for (unsigned long r = start; r != end;)
        {
            if (r == position || swaps.find(r) != swaps.end() || myset.find(r) != myset.end())
            {
                // stopping condition
                if (r == end)
                    break;
                if (move_forward)
                    r++;
                else
                    r--;
                continue;
            }
            else
            {
                // if we found an eligible swap
                swaps.insert(r);
                if (r == 0)
                {
                    std::cout << "swap 0 (4)" << std::endl;
                }

                if (abs(int(r - position)) < min_l)
                    min_l = abs(int(r - position));
                else if (abs(int(r - position)) > max_l)
                    max_l = abs(int(r - position));

                if (array[position] == 0 || array[r] == 0)
                {
                    std::cout << "here (3)" << std::endl;
                }

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

            // stopping condition
            if (r == end)
                break;
            if (move_forward)
                r++;
            else
                r--;
        }
        // manually increment iterator only if found is false
        // else, the erase operation would have automatically moved the iterator ahead
        if (!found)
        {
            ++iter;
        }
    }

    std::cout << "Left out sources after brute force = " << left_out_sources.size() << std::endl;

    std::cout << "Swaps made = " << noise_counter << std::endl;
    // std::cout << "Noise limit = " << noise_limit << std::endl;
    std::cout << "Min L = " << min_l << std::endl;
    std::cout << "Max L = " << max_l << std::endl;

    double median_l = findMedian(l_values, l_values.size());
    std::cout << "Median L = " << median_l << std::endl;

    for (unsigned long j = 0; j < TOTAL_NUMBERS; j++)
    {
        myfile1 << array[j] << "," << generateValue(payload_size);
        if (j != TOTAL_NUMBERS - 1)
            myfile1 << "\n";
    }

    // myfile.close();
    myfile1.close();

    return f1name;
}

// arguments to program:
// unsigned long pTOTAL_NUMBERS, unsigned int pdomain, unsigned long windowSize, short k, int pseed

int main(int argc, char **argv)
{
    // if (argc < 5)
    // {
    //     std::cout << "Program requires 6 inputs as parameters. \n Use format: ./execs/workload_generator.exe totalNumbers domain windowSize kNumber lNumber seedValue type" << std::endl;
    //     return 0;
    // }

    // unsigned long totalNumbers = atol(argv[1]);
    // int noisePercentage = atol(argv[2]);
    // int lPercentage = atoi(argv[3]);
    // int seedValue = atoi(argv[4]);
    // std::string type = argv[5];
    // // std::cout<<"wind = "<<windowSize<<std::endl;

    // generate_one_file(totalNumbers, noisePercentage, lPercentage, seedValue, type);

    args::ArgumentParser parser("Sortedness Parser.", "");

    args::Group group1(parser, "These arguments are REQUIRED",
                       args::Group::Validators::DontCare);
    args::Group group4(parser, "Optional switches and parameters:",
                       args::Group::Validators::DontCare);

    args::ValueFlag<unsigned long> total_numbers_cmd(group1, "N", "Total number of entries to generate", {'N', "total_entries"});
    // args::ValueFlag<unsigned long> domain_cmd(group1, "D", "Domain of entries", {'D', "domain"});
    args::ValueFlag<int> k_cmd(group1, "K", "% of out of order entries", {'K', "k_pt"});
    args::ValueFlag<int> l_cmd(group1, "L", "Maximum displacement of entries as %", {'L', "l_pt"});
    args::ValueFlag<int> seed_cmd(group1, "S", "Seed Value", {'S', "seed_val"});
    args::Flag text_file_cmd(group1, "txt", "output as txt file", {"txt", "txt_file"});
    args::ValueFlag<std::string> path_cmd(group1, "dir_path", "Path to output directory", {'o', "path"});
    args::ValueFlag<double> alpha_cmd(group1, "a", "Alpha Value", {'a', "alpha_val"});
    args::ValueFlag<double> beta_cmd(group1, "b", "Beta Value", {'b', "beta_val"});
    args::ValueFlag<int> payload_cmd(group1, "P", "Payload Size in Bytes", {'P', "beta_val"});

    if (argc == 1)
    {
        std::cout << parser;
        exit(0);
    }

    try
    {
        parser.ParseCLI(argc, argv);
        unsigned long totalNumbers = args::get(total_numbers_cmd);
        // unsigned long domain = args::get(domain_cmd);
        int K = args::get(k_cmd);

        // fix K for new definition
        K = K / 2;

        // since we are using rand() function, we only have to take l as an int
        int L = args::get(l_cmd);
        int seedValue = args::get(seed_cmd);
        std::string type = text_file_cmd ? "txt" : "bin";
        std::string pathToDirectory = args::get(path_cmd);
        double alpha = args::get(alpha_cmd);
        double beta = args::get(beta_cmd);
        int payload_size = args::get(payload_cmd);

        // for simplicity lets use window size = 1
        int windowSize = 1;

        // std::cout << "Total = " << totalNumbers << std::endl;
        // std::cout << "domain = " << domain << std::endl;

        generate_one_file(totalNumbers, K, L, seedValue, pathToDirectory, type, alpha, beta, payload_size);
    }
    catch (args::Help &)
    {
        std::cout << parser;
        exit(0);
        // return 0;
    }
    catch (args::ParseError &e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    catch (args::ValidationError &e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
}

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

using namespace boost::math;

inline bool ledger_exists();
void generate_one_file(unsigned long pTOTAL_NUMBERS, int k, int l, int pseed);
unsigned int get_number_domain(unsigned long position, unsigned long total, unsigned long domain_);
std::string generate_partitions_stream(unsigned long TOTAL_NUMBERS, int K, int L, int seed, std::string folder, std::string type);

inline void showProgress(const long &n, const long &count);
inline void showProgress(const long &workload_size, const long &counter)
{
    // std::cout << “flag = ” << flag << std::endl;
    // std::cout<<“2----“;
    if (counter / (workload_size / 100) >= 1)
    {
        for (int i = 0; i < 104; i++)
        {
            std::cout << "\b";
            fflush(stdout);
        }
    }
    for (int i = 0; i < counter / (workload_size / 100); i++)
    {
        std::cout << "=";
        fflush(stdout);
    }
    std::cout << std::setfill(' ') << std::setw(101 - counter / (workload_size / 100));
    std::cout << counter * 100 / workload_size << "%";
    fflush(stdout);
    if (counter == workload_size)
    {
        std::cout << "\n";
        return;
    }
}

inline bool ledger_exists()
{
    std::ifstream f("dataledger.txt");
    return f.good();
}

int sample_beta()
{
    double alpha = 2, beta = 3;
    unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
    std::default_random_engine e(seed);

    std::uniform_real_distribution<double> distr(0, 1);
    // double randFromUnif = distr(e);

    double randFromUnif = ((double)rand() / (RAND_MAX));

    beta_distribution<> dist(alpha, beta);
    double randFromDist = quantile(dist, randFromUnif);

    std::cout << "distr = " << randFromDist << "\t";

    int m = -100;
    int M = 100;

    int y = m + (M - m) * randFromDist;

    std::cout << "conv = " << y << std::endl;

    return randFromDist;
}

void generate_one_file(unsigned long pTOTAL_NUMBERS, int k, int l, int pseed, std::string type)
{
    std::ofstream outfile;

    srand(time(NULL));
    outfile.open("dataledger.txt", std::ios_base::app);

    // std::string folder_name = "mixed_workload/";
    std::string folder_name = "./";
    // std::string folder_name = "vary_buffer_workload/";
    //    std::string folder_name = "sorting_workload/";
    outfile << generate_partitions_stream(pTOTAL_NUMBERS, k, l, pseed, folder_name, type) << std::endl;
    // for (int i = 0; i < 10; i++)
    // {
    //     sample_beta();
    // }
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

    return ret;
}

unsigned long generate_random_in_range(unsigned long position, unsigned long Total_Numbers, int L)
{
    int l = L;

    long ret;

    uint min_jump = 1;
    uint max_jump = l;
    // std::cout << "Max jump possible = " << max_jump << std::endl;

    // a jump can be only of l windowSizeaces
    // int jump = rand() % (max_jump) + min_jump;
    unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
    std::default_random_engine e(seed);
    std::uniform_int_distribution<unsigned long> distr(min_jump, max_jump); // this is inclusive on both ends
    unsigned long jump = distr(e);

    // now lets do a small check. If both jumping forward and backward is invalid,
    // we need to reconfigure the jump to the max limit.
    // note, if position < jump, position-jump < 0
    if ((position + jump) >= Total_Numbers && (position <= jump))
    {
        // we can pick both ends now to determine the max jump
        // lets do a coin toss
        float p = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        if (p < 0.5)
        {
            // max jump configured upto position
            max_jump = position - 1;
        }
        else
        {
            max_jump = Total_Numbers - (position - 1) - 1;
        }

        // redo jump
        jump = rand() % (max_jump) + min_jump;
    }

    if (position <= l && ((position + jump) <= Total_Numbers))
    {
        // ret=rand() % position+l;
        // return ((unsigned long)rand() % position+l);

        // we can only jump forward
        ret = position + jump;
        if (ret < 0)
        {
            std::cout << "oops (negative index)" << std::endl;
            std::cout << "jump = " << jump << "; ret = " << ret << "; position = " << position << std::endl;
        }
        if (ret > Total_Numbers)
        {
            std::cout << "oops overboard" << std::endl;
        }
    }
    else if ((unsigned long)(position + l) >= Total_Numbers)
    {
        // ret = rand() % (Total_Numbers - (position - l)) + (position - l);
        // return ((unsigned long)rand() % (Total_Numbers - (position - l))) + (position - l);

        // we can only jump backward
        ret = position - jump;
        if (ret < 0)
        {
            std::cout << "oops (negative index)" << std::endl;
            std::cout << "jump = " << jump << "; ret = " << ret << "; position = " << position << std::endl;
        }
        if (ret > Total_Numbers)
        {
            std::cout << "oops overboard" << std::endl;
        }
    }
    else
    {
        // ret = rand() % ((position + l) - (position - l)) + (position - l);
        // return ((unsigned long)rand() % ((position + l) - (position - l))) + (position - l);

        // we can jump forward or backward
        // let's toss a coin to find out what to do
        float p = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        // we move backwards with p < 0.5
        if (p < 0.5)
            ret = position - jump;
        else
            ret = position + jump;

        if (ret < 0)
        {
            std::cout << "oops (negative index)" << std::endl;
            std::cout << "jump = " << jump << "; ret = " << ret << "; position = " << position << std::endl;
        }
        if (ret > Total_Numbers)
        {
            std::cout << "oops overboard" << std::endl;
        }
    }
    if (ret < 0)
    {
        std::cout << "oops (negative index)" << std::endl;
        std::cout << "jump = " << jump << "; ret = " << ret << "; position = " << position << std::endl;
    }

    if (ret > Total_Numbers)
    {
        std::cout << "oops overboard" << std::endl;
    }

    return (unsigned long)ret;
}

// code inspired from GeekForGeeks
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
std::string generate_partitions_stream(unsigned long TOTAL_NUMBERS, int K, int L, int seed, std::string folder = "./Data", std::string type = "bin")
{
    // float p_outOfRange = (double)K / TOTAL_NUMBERS;
    float p_outOfRange = K;

    std::srand(seed);

    // unsigned long array[TOTAL_NUMBERS];
    //  std::vector<unsigned long> array(TOTAL_NUMBERS, 0);
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
    // f1name += ".dat";
    if (type.compare("txt") == 0)
    {
        f1name += ".txt";
        myfile1.open(f1name);
    }

    else
    {
        f1name += ".dat";
        myfile1.open(f1name, std::ios::binary);
    }

    // std::ofstream myfile(fname, std::ios::binary);

    // if (type.compare("txt") == 0)
    //     myfile1(f1name);

    unsigned long noise_limit = TOTAL_NUMBERS * p_outOfRange / 100.0;
    unsigned long l_absolute = TOTAL_NUMBERS * L / 100.0;
    // assert(noise_limit == K);
    unsigned long noise_counter = 0;

    std::unordered_set<unsigned long> myset;
    std::unordered_set<unsigned long> swaps;
    std::unordered_set<unsigned long> left_out_sources;
    unsigned long w = 0;
    unsigned long windowSize = 1;
    std::cout << "pOut = " << p_outOfRange << std::endl;
    std::cout << "L% = " << L << std::endl;
    std::cout << "L absolute = " << l_absolute << std::endl;
    std::cout << "limit = " << noise_limit << std::endl;
    unsigned long min_l = l_absolute, max_l = 0;

    std::vector<long> l_values;

    double alpha = 5.0;
    double beta = 10.0;

    for (unsigned long i = 0; i < TOTAL_NUMBERS; i++, w += windowSize)
    {
        array[i] = w;
    }

    // generate noise_limit number of unique random numbers
    int ctr = 0;
    // std::vector<unsigned int> ks;
    while (ctr < noise_limit)
    {
        unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
        std::default_random_engine e(seed);

        std::uniform_int_distribution<unsigned long> distr(1, TOTAL_NUMBERS - 1);
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

    std::cout << "ctr = " << ctr << std::endl;

    // first, we want to ensure that the max displacement is L
    // this means at least one swap has to happen by L positions
    for (auto itr = myset.begin(); itr != myset.end(); itr++)
    {
        unsigned long i = *itr;
        unsigned long r = i + l_absolute;

        std::cout << "r = " << r << std::endl;

        // make sure r not another source for a swap
        if (myset.find(r) != myset.end())
        {
            // if indeed this is a source, move on to the next source and then pick a swap at L positions
            continue;
        }
        swaps.insert(r);
        // max_l = r - i;
        if (abs(int(r - i)) < min_l)
            min_l = abs(int(r - i));
        else if (abs(int(r - i)) > max_l)
            max_l = abs(int(r - i));
            
        unsigned long temp = array[i];
        array[i] = array[r];
        array[r] = temp;

        noise_counter++;

        l_values.push_back(r - i);

        // we should also remove this source
        itr = myset.erase(itr);
        break;
    }

    std::cout << "Have at least one element with L" << std::endl;
    // now, loop through the sources values
    for (auto itr = myset.begin(); itr != myset.end(); itr++)
    {
        unsigned long r;
        unsigned long i = *itr;
        // std::cout << i << " ";
        int num_tries = 4;
        while (num_tries > 0)
        {
            // r = generate_random_in_range(i, TOTAL_NUMBERS, l_absolute);
            r = generate_beta_random_in_range(i, TOTAL_NUMBERS, l_absolute, alpha, beta);
            num_tries--;

            // check for cascading swaps, i.e. we should not pick a spot again to swap
            // also we should not pick one of our already defined source places
            if ((r == i) || swaps.find(r) != swaps.end() && myset.find(r) != myset.end())
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

                if (r == i)
                {
                    std::cout << "same place" << std::endl;
                }

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
            if ((r == i) || swaps.find(r) != swaps.end() && myset.find(r) != myset.end())
            {
                // if we run out of tries, we keep the element in the left_out_sources
                continue;
            }
            else
            {
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
        // else, the erase operation would have automatically moved the iterator ahead
        if (!found)
        {
            ++it;
        }
    }

    std::cout << "Left out sources after increased jumps = " << left_out_sources.size() << std::endl;

    // now let us give one final try with brute force
    for (auto it = left_out_sources.begin(); it != left_out_sources.end();)
    {
        unsigned long position = *it;
        unsigned long start = position - l_absolute;
        unsigned long end = position + l_absolute;

        bool found = false;

        // start position should usually be (position - l) and end should be (position + l)
        //  however, we need to check for edge cases

        if (position - l_absolute < 0)
        {
            start = 0;
        }

        if (position + l_absolute > TOTAL_NUMBERS)
        {
            end = TOTAL_NUMBERS;
        }

        // now, loop through from start to end
        // we will pick the first valid swap spot
        for (unsigned long r = start; r < end; r++)
        {
            if (swaps.find(r) != swaps.end() && myset.find(r) != myset.end())
            {

                continue;
            }
            else
            {
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
                it = left_out_sources.erase(it);
                found = true;

                l_values.push_back(abs(int(r - position)));

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

    std::cout << "Left out sources after brute force = " << left_out_sources.size() << std::endl;

    std::cout << "Noise counter = " << noise_counter << std::endl;
    std::cout << "Noise limit = " << noise_limit << std::endl;
    std::cout << "Min L = " << min_l << std::endl;
    std::cout << "Max L = " << max_l << std::endl;

    double median_l = findMedian(l_values, l_values.size());
    std::cout << "Median L = " << median_l << std::endl;

    if (type.compare("txt") == 0)
    {
        for (unsigned long j = 0; j < TOTAL_NUMBERS; ++j)
        {
            // auto pair = std::make_pair(array[j], array[j]);
            // //myfile.write(reinterpret_cast<char *>(&pair), sizeof(std::pair<unsigned int, unsigned int>));
            // myfile1.write(reinterpret_cast<char *>(&array[j]), sizeof(int));
            myfile1 << array[j] << ",";
        }
    }
    else
    {
        for (unsigned long j = 0; j < TOTAL_NUMBERS; ++j)
        {
            // auto pair = std::make_pair(array[j], array[j]);
            // myfile.write(reinterpret_cast<char *>(&pair), sizeof(std::pair<unsigned int, unsigned int>));
            myfile1.write(reinterpret_cast<char *>(&array[j]), sizeof(unsigned long));
            // std::cout<<array[j]<<std::endl;
            // break;
        }
    }

    // myfile.close();
    myfile1.close();

    return f1name;
}

// arguments to program:
// unsigned long pTOTAL_NUMBERS, unsigned int pdomain, unsigned long windowSize, short k, int pseed

int main(int argc, char **argv)
{
    if (argc < 5)
    {
        std::cout << "Program requires 6 inputs as parameters. \n Use format: ./execs/workload_generator.exe totalNumbers domain windowSize kNumber lNumber seedValue type" << std::endl;
        return 0;
    }

    unsigned long totalNumbers = atol(argv[1]);
    int noisePercentage = atol(argv[2]);
    int lPercentage = atoi(argv[3]);
    int seedValue = atoi(argv[4]);
    std::string type = argv[5];
    // std::cout<<"wind = "<<windowSize<<std::endl;

    generate_one_file(totalNumbers, noisePercentage, lPercentage, seedValue, type);
}

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

    outfile.close();
}

unsigned int get_number_domain(unsigned long position, unsigned long total, unsigned int domain_)
{
    return (position * domain_) / total;
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

// unsigned long find_swap_brute_force(unsigned long position, unsigned long Total_Numbers, int l)
// {
//     unsigned long start = position - l;
//     unsigned long end = position + l;

//     // start position should usually be (position - l) and end should be (position + l)
//     //  however, we need to check for edge cases

//     if (position - l < 0)
//     {
//         start = 0;
//     }

//     if (position + l > Total_Numbers)
//     {
//         end = Total_Numbers;
//     }

//     // now, loop through from start to end
//     // we will pick the first valid swap spot
//     for(unsigned long i = start; i < end; i++)
//     {

//     }
// }

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
    unsigned long min_l = l_absolute, max_l = l_absolute;
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

        std::uniform_int_distribution<unsigned long> distr(1, TOTAL_NUMBERS);
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

    // now, loop through these values
    for (auto itr = myset.begin(); itr != myset.end(); itr++)
    {
        unsigned long r;
        unsigned long i = *itr;
        // std::cout << i << " ";
        int num_tries = 4;
        while (num_tries > 0)
        {
            r = generate_random_in_range(i, TOTAL_NUMBERS, l_absolute);
            num_tries--;

            // check for cascading swaps, i.e. we should not pick a spot again to swap
            // also we should not pick one of our already defined source places
            if (swaps.find(r) != swaps.end() && myset.find(r) != myset.end())
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

                if (abs(r - i) < min_l)
                    min_l = abs(r - i);
                else if (abs(r - i) > max_l)
                    max_l = abs(r - i);

                unsigned long temp = array[i];
                array[i] = array[r];
                array[r] = temp;

                noise_counter++;
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
            r = generate_random_in_range(i, TOTAL_NUMBERS, l_absolute);
            num_tries--;
            // check for cascading swaps, i.e. we should not pick a spot again to swap
            // also we should not pick one of our already defined source places
            if (swaps.find(r) != swaps.end() && myset.find(r) != myset.end())
            {
                // if we run out of tries, we keep the element in the left_out_sources
                continue;
            }
            else
            {
                // if we found an eligible swap
                swaps.insert(r);

                if (abs(r - i) < min_l)
                    min_l = abs(r - i);
                else if (abs(r - i) > max_l)
                    max_l = abs(r - i);

                unsigned long temp = array[i];
                array[i] = array[r];
                array[r] = temp;

                noise_counter++;

                // remove the current source from left out sources
                it = left_out_sources.erase(it);
                found = true;

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

                if (abs(r - position) < min_l)
                    min_l = abs(r - position);
                else if (abs(r - position) > max_l)
                    max_l = abs(r - position);

                unsigned long temp = array[position];
                array[position] = array[r];
                array[r] = temp;

                noise_counter++;

                // remove the current source from left out sources
                it = left_out_sources.erase(it);
                found = true;

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

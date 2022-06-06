#include <iostream>
#include "btree.h"
#include <chrono>
#include <fstream>
#include "args.hxx"
#include <vector>
#include "util.h"

using namespace std;

int main(int argc, char *argv[])
{
    args::ArgumentParser parser("Sample STX Btree Test Parser.", "");

    args::Group group1(parser, "These arguments are REQUIRED",
                       args::Group::Validators::DontCare);

    args::ValueFlag<std::string> input_file_cmd(group1, "input_file_path", "Path to input file", {'f', "input_file_path"});
    args::ValueFlag<unsigned long> num_load_cmd(group1, "num_load", "Number of rows to load", {'l', "num_load"});
    args::ValueFlag<unsigned long> num_queries_cmd(group1, "num_queries", "Number of queries to execute", {'q', "num_queries"});
    args::Flag bulkload_cmd(group1, "bulkload", "bulk load entries to the tree", {"bk", "bulkload"});

    if (argc == 1)
    {
        std::cout << parser;
        exit(0);
    }
    try
    {
        parser.ParseCLI(argc, argv);

        string input_file = args::get(input_file_cmd);
        unsigned long num_load = args::get(num_load_cmd);
        unsigned long num_queries = args::get(num_queries_cmd);
        bool bulkload = bulkload_cmd ? true : false;

        std::vector<pair<unsigned long, string>> data;

        string line, word, temp;
        vector<string> row;
        cout << "now starting to read" << endl;

        string file_contents = readFileIntoString(input_file);
        istringstream sstream(file_contents);
        string record;
        while (std::getline(sstream, record))
        {
            row.clear();
            istringstream line(record);
            while (std::getline(line, record, ','))
            {
                row.push_back(record);
            }

            // first word of every row is an unsigned long
            unsigned long key = stoul(row[0]);

            // add to our data vector
            data.push_back(make_pair(key, row[1]));
        }
        stx::btree<unsigned long, string> tree;

        if (bulkload)
        {
            // bulkload tree here
            auto start = std::chrono::high_resolution_clock::now();
            if (num_load != 0)
            {
                auto end = data.begin() + num_load;

                tree.bulk_load(data.begin(), end);
            }
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration_sort = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
            cout << "Pre-loading time = " << duration_sort << endl;
            cout << "Pre-loaded using bulkload " << num_load << " rows." << endl;
        }
        else
        {
            // loading phase
            auto start = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < num_load; i++)
            {
                tree.insert(data[i].first, data[i].second);
            }

            auto stop = std::chrono::high_resolution_clock::now();
            auto duration_sort = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
            cout << "Pre-loading time = " << duration_sort << endl;
            cout << "Pre-loaded " << num_load << " rows." << endl;
        }
        // now do mixed reads and writes
        unsigned long tot_inserts = num_load, tot_queries = 0, num_oper = 0;
        unsigned long i = num_load;
        unsigned long n = data.size();
        auto start = std::chrono::high_resolution_clock::now();
        int num_empt = 0;
        while (true)
        {
            if (tot_inserts < n && tot_queries < num_queries)
            {
                double p = (double)rand() / RAND_MAX;
                if (p < 0.5)
                {
                    tree.insert(data[i].first, data[i].second);
                    i++;
                    tot_inserts++;
                }
                else
                {
                    // pick a number between 1 and tot_inserts
                    int query_index = (rand() % tot_inserts) + 1;
                    bool res = tree.exists(query_index);
                    if (!res)
                        num_empt++;
                    tot_queries++;
                }
                num_oper++;
            }
            else if (tot_inserts < n && tot_queries >= num_queries)
            {
                tree.insert(data[i].first, data[i].second);
                i++;
                tot_inserts++;
                num_oper++;
            }
            else if (tot_inserts >= n && tot_queries <= num_queries)
            {
                // pick a number between 1 and tot_inserts
                int query_index = (rand() % tot_inserts) + 1;
                bool res = tree.exists(query_index);
                if (!res)
                    num_empt++;
                tot_queries++;
                num_oper++;
            }
            if (tot_inserts >= n && tot_queries >= num_queries)
            {
                break;
            }
        }

        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();

        cout << "Time in nanoseconds for all operations = " << duration << endl;

        cout << "Total Number of Loads = " << num_load << endl;
        cout << "Total Number of inserts = " << tot_inserts << endl;
        cout << "Total Number of queries = " << tot_queries << endl;
        cout << "Total Number of operations = " << num_oper << endl;
        cout << "num empt = " << num_empt << endl;
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

    return 0;
}
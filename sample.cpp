#include <iostream>
#include "btree.h"
#include <chrono>
#include <fstream>
#include "args.hxx"

using namespace std;

int main(int argc, char *argv[])
{
    args::ArgumentParser parser("Sample STX Btree Test Parser.", "");

    args::Group group1(parser, "These arguments are REQUIRED",
                       args::Group::Validators::DontCare);

    args::ValueFlag<std::string> input_file_cmd(group1, "input_file_path", "Path to input file", {'f', "input_file_path"});

    if (argc == 1)
    {
        std::cout << parser;
        exit(0);
    }
    try
    {
        parser.ParseCLI(argc, argv);

        string input_file = args::get(input_file_cmd);

        long int size = 0;
        int *data;

        ifstream infile(input_file, ios::in | ios::binary);
        if (!infile)
        {
            cout << "Cannot open file!" << endl;
            return 0;
        }

        FILE *file = fopen(input_file.c_str(), "rb");
        if (file == NULL)
            return 0;

        fseek(file, 0, SEEK_END);
        size = ftell(file);
        fclose(file);

        cout << "size = " << size << endl;

        data = new int[size / sizeof(int)];
        infile.read((char *)data, size);
        // fclose(input_file);
        infile.close();

        int num = size / sizeof(int);
        cout << "Number of keys in file = " << num << endl;

        stx::btree<int, int> tree;

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < num; i++)
        {
            tree.insert(data[i] + 1, data[i] + 1);
        }

        auto stop = std::chrono::high_resolution_clock::now();
        auto duration_sort = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
        cout << "Time taken for inserts = " << duration_sort << " nanoseconds" << endl;
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

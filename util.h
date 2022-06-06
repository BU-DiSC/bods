#ifndef UTIL_H
#define UTIL_H
#include <iostream>
#include <fstream>

using namespace std;
std::string readFileIntoString(const std::string &path)
{
    auto ss = ostringstream{};
    ifstream input_file(path);
    if (!input_file.is_open())
    {
        cerr << "Could not open the file - '"
             << path << "'" << endl;
        exit(EXIT_FAILURE);
    }
    ss << input_file.rdbuf();
    return ss.str();
}

#endif
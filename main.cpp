//====================================================================================
//                        ------->  Revision History  <------
//====================================================================================
//
//   Date     Who   Ver  Changes
//====================================================================================
// 03-Jul-24  DWW   1.0  Initial creation
//====================================================================================
#define REVISION "1.0"

/*
    This program denormalizes a Xilinx Block Design file into key/value pairs.

    This is not a full-featured XML parser... just enough to perform the task
    at hand.

*/

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <stdexcept>
#include <string>
#include <vector>


#include "ulw_json.h"

ulw_json::Parser JSON;

using std::string;
using std::vector;

string  input_file;
void execute(int argc, char** argcv);


//=============================================================================
// main() - Denormalizes a simple XML file
//=============================================================================
int main(int argc, char** argv)
{
    try
    {
        execute(argc, argv);
    }
    catch(const std::exception& e)
    {
        fprintf(stderr, "parse_xbd: %s\n", e.what());
        exit(1);
    }
}
//=============================================================================



//=============================================================================
// show_help() - Displays some minimal help-text
//=============================================================================
void show_help()
{
    printf("parse_xbd %s\n", REVISION);
    printf("usage: parse_xbd <filename>\n");
    exit(1);
}
//=============================================================================


//=============================================================================
// parse_command_line() - Parses command-line parameters
//=============================================================================
void parse_command_line(char** argv)
{
    if (argv[1] == nullptr)
        show_help();
    input_file = argv[1];
}
//=============================================================================


//=============================================================================
// execute() - Spins through the input file, outputting denormalized key-value
//             pairs
//=============================================================================
void execute(int argc, char** argv)
{
    // Parse the command line
    parse_command_line(argv);

    // Parse the JSON file into denormalized key/value pairs
    auto v = JSON.parse(input_file);

    // Print the key-value pairs
    for (auto& e : v)
    {
      printf("%s = %s\n", e.key.c_str(), e.value.c_str());
    }

}
//=============================================================================

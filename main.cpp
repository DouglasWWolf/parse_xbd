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

using std::string;
using std::vector;

const char OCB = '{';
const char CCB = '}';
const char OSB = '[';
const char CSB = ']';

vector<string>  hier;
volatile size_t bitbucket;
string          input_file;
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
// throwRuntime() - Throws a runtime exception
//=============================================================================
static void throwRuntime(const char* fmt, ...)
{
    char buffer[1024];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    throw std::runtime_error(buffer);
}
//=============================================================================


//=============================================================================
// heirarchy() - Returns a string that represents the XML hierarchy thus far
//=============================================================================
string hierarchy()
{
    string result;

    for (auto& s : hier)
    {
        result = result + s + ".";
    }    

    return result;
}
//=============================================================================


//=============================================================================
// read_file() - Reads the XML file and returns a pointer to the contents
//=============================================================================
const char* read_file(string filename)
{
    // Convert the filename to a const char*
    const char* fn = filename.c_str();

    // Open the file and complain if we can't
    FILE* ifile = fopen(fn, "r");
    if (ifile == nullptr) throwRuntime("Can't open %s", fn);

    // Find out how big the file is
    fseek(ifile, 0, SEEK_END);
    int file_size = ftell(ifile);
    rewind(ifile);

    // Allocate enough RAM to hold the entire file
    char* xmldata = new char[file_size + 1];

    // Read the input file into the buffer and make sure it's nul-terminated
    bitbucket = fread(xmldata, 1, file_size, ifile);
    xmldata[file_size] = 0;

    // Return a pointer to the XML data
    return xmldata;
}
//=============================================================================


//=============================================================================
// skip_first_brace() - Skips over the initial "open curly brace" in the input
//=============================================================================
const char* skip_first_brace(const char* ptr)
{
    while (*ptr)
    {
        int c = *ptr++;
        if (c == OCB) return ptr;        
    }

    // If we get here, there was no opening curly brace
    throwRuntime("No opening brace found");

    // This is just to keep the compiler happy
    return nullptr;
}
//=============================================================================


//=============================================================================
// show() - A routine that shows the xml buffer from a particular location.
//          Useful for debugging
//=============================================================================
void show(const char* ptr)
{
    char buffer[100];

    strncpy(buffer, ptr, sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = 0;
    fprintf(stderr, "\n%s\n", buffer);
}
//=============================================================================



//=============================================================================
// skip_whitespace() - Skips over whitespace <AND COMMAS>.   We treat commas
//                     as whitespace because, in this application, it's 
//                     convenient to ignore them.
//=============================================================================
const char* skip_whitespace(const char* p)
{
    while (*p == ' ' || *p == 9 || *p == '\n' || *p == '\r' || *p == ',') ++p;
    return p;
}
//=============================================================================


//=============================================================================
// fetch_quoted_string() - Retreives the string of characters that is between
//                         double-quotes
//=============================================================================
const char* fetch_quoted_string(const char*p, char* dest)
{
    p = skip_whitespace(p);
    
    if (*p != 34) 
    {
        show(p);
        throwRuntime("Expected double-quote");
    }

    // Skip over the opening quotation mark
    ++p;

    while (true)
    {
        int c = *p++;
        if (c == 0) throwRuntime("Unexpected EOF!");
        if (c == 34) break;
        *dest++ = c;       
    }

    *dest = 0;
    return p;
}
//=============================================================================


//=============================================================================
// get_key() - XML has "key" : "value" pairs.    This fetches the "key" portion
//             of that pair
//=============================================================================
const char* get_key(const char* p, char* dest)
{
    p = skip_whitespace(p);
    if (*p == CCB || *p == CSB)
    {
        dest[0] = CCB;
        dest[1] = 0;
        return p+1;
    }

    if (*p == 34)
    {
        p = fetch_quoted_string(p, dest);
    }

    return p;
}
//=============================================================================


//=============================================================================
// get_value() - XML has "key" : "value" pairs.   This fetches the "value" 
//              portion of that pair
//=============================================================================
const char* get_value(const char* p, char* dest)
{
    p = skip_whitespace(p);
    
    // If the next character isn't a colon, there's no value
    if (*p != ':')
    {
        *dest = 0;
        return p;
    }

    // Skip over the colon and any whitespace
    p = skip_whitespace(p+1);
    
    // If the value was an open curly brace, we're done
    if (*p == OCB || *p == OSB)
    {
        dest[0] = OCB;
        dest[1] = 0;
        return p+1;
    }

    p = fetch_quoted_string(p, dest);
    
    return p;
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
    char* xmlkey = new char[10000000];
    char* xmlval = new char[10000000];

    // Parse the command line
    parse_command_line(argv);

    // Read in the XML file
    const char* xmlptr = read_file(input_file);

    // Skip over the first "{" in the file
    xmlptr = skip_first_brace(xmlptr);

    // We're going to read every key-value pair in the file
    while (true)
    {
        // Fetch the next key
        xmlptr = get_key(xmlptr, xmlkey);
        
        // If it was a "close curly brace", remove a level from the hierarchy
        if (xmlkey[0] == CCB)
        {
            if (hier.size() == 0) exit(0);
            hier.pop_back();
            continue;
        }
        

        // Fetch the value that goes with that key
        xmlptr = get_value(xmlptr, xmlval);
        
        // If it was an "open curly brace", add a level to the hierarchy
        if (xmlval[0] == OCB)
        {
            hier.push_back(xmlkey);
            continue;
        }

        // Print the denormalized key-value pair
        printf("%s%s = \"%s\"\n", hierarchy().c_str(), xmlkey, xmlval);
    }

}
//=============================================================================

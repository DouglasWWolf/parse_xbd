//=============================================================================
// ulw_json.cpp - Implements an ultra-lightweight JSON parser
//=============================================================================
#include <cstdarg>
#include <stdexcept>
#include <cstdint>
#include <cstring>
#include "ulw_json.h"    

// Allow the code that follows to conveniently refer to these STL containers
using std::string;
using std::vector;


static const char OCB = '{';
static const char CCB = '}';
static const char OSB = '[';
static const char CSB = ']';

// This is the maximum allowed size of any one token
static const int MAX_TOKEN = 1000000;


static const char* fetch_token
(
    const char* p, char* dest, size_t dest_size, bool strip_quotes = false
);

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
// show() - A routine that shows the json buffer from a particular location.
//          Useful for debugging
//=============================================================================
static void show(const char* ptr)
{
    char buffer[100];

    memcpy(buffer, ptr, sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = 0;
    fprintf(stderr, "\n%s\n", buffer);
}
//=============================================================================





//=============================================================================
// read_file() - Reads the JSON file and returns a pointer to the contents
//=============================================================================
static const char* read_file(string filename)
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
    char* file_data = new char[file_size + 1];

    // Read the input file into the buffer and make sure it's nul-terminated
    if (fread(file_data, 1, file_size, ifile) != file_size)
    {
        throwRuntime("failed to read %s", fn);        
    }
    file_data[file_size] = 0;

    // Return a pointer to the file_data
    return file_data;
}
//=============================================================================


//=============================================================================
// is_ws() - Returns true if 'c' is a white-space character
//=============================================================================
static bool is_ws(char c)
{
    return (c == 32 || c == 9 || c == 10 || c == 13);
}
//=============================================================================


//=============================================================================
// skip_whitespace() - Skips over whitespace.
//=============================================================================
static const char* skip_whitespace(const char* p)
{
    while (*p == ' ' || *p == 9 || *p == 10 || *p == 13) ++p;

    if (*p == 0) throwRuntime("unexpected end of JSON data");

    return p;
}
//=============================================================================


//=============================================================================
// skip_first_brace() - Skips over the initial "open curly brace" in the input
//=============================================================================
static const char* skip_first_brace(const char* ptr)
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
// fetch_token() - Fetches a single token.  This routine respects "" marks
//
// Returns:  a pointer to the next non-whitespace character after the token
//=============================================================================
static const char* fetch_token
(
    const char* p, char* dest, size_t dest_size, bool strip_quotes
)
{
    // Presume for a moment this isn't a JSON string
    bool is_quoted = false;

    // The first character we output is at the destination
    char* out = dest;

    // Leave room for the nul-terminator at the end
    --dest_size;

    // Find out if we're parsing a quoted string
    if (*p == 34)
    {
        is_quoted = true;
        if (strip_quotes)
            ++p;
        else
            *out++ = *p++;
    }

    // Fetch characters one at a time and stuff them into the output buffer
    while (true)
    {
        // Fetch the next character
        int c = *p++;

        // We should never see a nul-terminator on the input
        if (c == 0) throwRuntime("unexpected end of JSON data");
       

        // If this is a quote mark and we need to strip it...
        if (is_quoted && strip_quotes && c == 34)
        {
            break;
        }

        // Is this the end of the token?
        if (!is_quoted && (is_ws(c) || c == ','))
        {
          --p;
          break;            
        }

        // If this character will fit into the output buffer, 
        // place it there.
        if ((out - dest) < dest_size) *out++ = c;

        // If we just output a closing double-quote, we're done
        if (is_quoted && c == 34) break;
    }

    // Nul-terminate the end of the output token
    *out = 0;

    // Skip over trailing whitespace on the input
    return skip_whitespace(p);
}
//=============================================================================



//=============================================================================
// get_json_key() - JSON has "key" : "value" pairs.   This fetches the "key"
//                  portion of that pair
//=============================================================================
static const char* get_json_key(const char* p, char* dest, int dest_size)
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
        p = fetch_token(p, dest, dest_size, true);
    }
    else
    {
        show(p);
        throwRuntime("malformed JSON");
    }

    return p;
}
//=============================================================================



//=============================================================================
// get_json_value() - XML has "key" : "value" pairs.   This fetches the "value" 
//                    portion of that pair
//=============================================================================
static const char* get_json_value(const char* p, char* dest, int dest_size)
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

    // Fetch the token and hand the caller to the ptr to the next character
    return fetch_token(p, dest, dest_size);
}
//=============================================================================


//=============================================================================
// skip_comma() - This skips over a trailing comma (if it exists)
//=============================================================================
static const char* skip_comma(const char* p)
{
    p = skip_whitespace(p);
    if (*p == ',')
    {
        p = skip_whitespace(p+1);
    }
    return p;
}
//=============================================================================



//=============================================================================
// parse() - Parses a JSON file into a vector of key/value pairs
//=============================================================================
vector<ulw_json::pair_t> ulw_json::Parser::parse(string filename)
{
    ulw_json::pair_t entry;
    vector<ulw_json::pair_t> result;

    // Declare a couple of large buffers to hold the JSON key/value pair
    char* json_key = new char[MAX_TOKEN];
    char* json_val = new char[MAX_TOKEN];

    // Read the entire JSON file into RAM
    const char* json_ptr = read_file(filename);

    // Skip over the opening brace
    json_ptr = skip_first_brace(json_ptr);

    // We're going to read every key-value pair in the file
    while (true)
    {
        // Fetch the next key
        json_ptr = get_json_key(json_ptr, json_key, MAX_TOKEN);
        
        // If it was a "close curly brace", remove a level from the hierarchy
        if (json_key[0] == CCB)
        {
            if (!m_hier.pop()) break;
            json_ptr = skip_comma(json_ptr);
            continue;
        }

        // Fetch the value that goes with that key
        json_ptr = get_json_value(json_ptr, json_val, MAX_TOKEN);
     
        // If it was an "open curly brace", add a level to the hierarchy
        if (json_val[0] == OCB)
        {
            m_hier.push(json_key);
            continue;
        }

        // Stuff this key-value pair into the result vector
        entry.key   = m_hier.str() + json_key;
        entry.value = json_val;
        result.push_back(entry);

        // Print the denormalized key-value pair
        printf("%s%s = %s\n", m_hier.str().c_str(), json_key, json_val);

        // If the next character is a comma, skip it
        json_ptr = skip_comma(json_ptr);
    }
    
    return result;
}
//=============================================================================
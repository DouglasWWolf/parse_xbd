//=============================================================================
// ulw_json.h - Defines an ultra-lightweight JSON parser
//=============================================================================
#pragma once
#include <string>
#include <vector>

 
namespace ulw_json
{
    class Parser;

    class JSON_heir;

    struct pair_t {std::string key, value;};
}

//-----------------------------------------------------------------------------
// This class is used by the JSON parser to keep track of a stack of hierarchy
// levels
//-----------------------------------------------------------------------------
class ulw_json::JSON_heir
{
public:


    void push(const std::string s)
    {
        m_hiervec.push_back(m_hierarchy);
        m_hierarchy = m_hierarchy + s + ".";
    }

    bool pop()
    {
        if (m_hiervec.empty())
        {
            m_hierarchy.clear();
            return false;
        }
        m_hierarchy = m_hiervec[m_hiervec.size() - 1];
        m_hiervec.pop_back();
        return true;
    }

    std::string str()
    {
        return m_hierarchy;
    }


protected:

    // This is the current "top" of the hierarchy
    std::string m_hierarchy;

    // This is a "stack" of hierarchy strings
    std::vector<std::string> m_hiervec;

};
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
// ultra-light-weight JSON parser
//-----------------------------------------------------------------------------
class ulw_json::Parser
{
public:

    // This returns a vector of key/value pairs
    std::vector<pair_t> parse(std::string filename);

protected:

    // This is a stack of strings that describe the JSON hierarchy
    JSON_heir m_hier;
};
//-----------------------------------------------------------------------------
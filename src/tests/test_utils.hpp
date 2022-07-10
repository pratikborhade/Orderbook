#pragma once
#include <stdexcept>
#include <sstream>
void assert(bool condition, const char* message)
{
    if(!condition)
    {
        throw std::runtime_error(message);
    }
}

void assert(bool condition, std::string message)
{
    if(!condition)
    {
        throw std::runtime_error(message);
    }
}

template<typename A, typename B>
void assert_equal(const A& a, const B& b, const char* file, int line)
{
    if( !(a == b) )
    {
        std::stringstream ss;
        ss << "equal assert fail at " << file << "@" << line;
        assert(false, ss.str());
    }
}

#define assert_equal(a,b) assert_equal(a, b, __FILE__, __LINE__)
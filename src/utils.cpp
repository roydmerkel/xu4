/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "utils.h"
#include <cctype>
#include <cstdlib>
#include <ctime>

/**
 * Seed the random number generator.
 */
void xu4_srandom() {
    srand(time(NULL));
}

/**
 * Generate a random number between 0 and (upperRange - 1).  This
 * routine uses the upper bits of random number provided by rand() to
 * compensate for older generators that have low entropy in the lower
 * bits (e.g. MacOS X).
 */
int xu4_random(int upperRange) {
    int r = rand();
    return (int) ((((double)upperRange) * r) / (RAND_MAX+1.0));
}

/**
 * Trims whitespace from a std::string
 */
void trim(std::string *val) {
    using namespace std;
    string::iterator i;
    for (i = val->begin(); (i != val->end()) && isspace(*i); )
        i = val->erase(i);
    for (i = val->end()-1; (i != val->begin()) && isspace(*i); )
        i = val->erase(i)-1;
}

/**
 * Converts the string to lowercase
 */ 
void lowercase(string *val) {
    using namespace std;
    string::iterator i;
    for (i = val->begin(); i != val->end(); i++)
        *i = tolower(*i);
}

/**
 * Converts the string to uppercase
 */ 
void uppercase(string *val) {
    using namespace std;
    string::iterator i;
    for (i = val->begin(); i != val->end(); i++)
        *i = toupper(*i);
}

/**
 * Splits a string into substrings, divided by the charactars in
 * separators.  Multiple adjacent seperators are treated as one.
 */
std::vector<string> split(const string &s, const string &separators) {
    std::vector<string> result;
    string current;

    for (unsigned i = 0; i < s.length(); i++) {
        if (separators.find(s[i]) != string::npos) {
            if (current.length() > 0)
                result.push_back(current);
            current.erase();
        } else
            current += s[i];
    }

    if (current.length() > 0)
        result.push_back(current);

    return result;
}



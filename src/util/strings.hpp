#pragma once

#include <ctime>
#include <string>
#include <vector>

using namespace std;

namespace peer_to_peer
{
    /* Adapted from Jankatay's "util.hpp" */
    
    /**
     * Splits a string into a vector containing substrings separated by quotation marks, then spaces.
     * If double quotes used, string will accept single quotes and vice versa
     * Also allows backslashes to use double quotes in double quotes.
     * After the quotations, will split around whitespace.
     * Allows commands to be used as seperators outside of quotations too.
     * @param str The string to be split
     * @param arguments A reference to the vector containing the resulting argument substrings.
     * Empty if the string could not be split successfully.
     * @return false if errors like unclosed quotations or unknown characters were encountered; otherwise, true
     */
    bool str_to_args(const string& str, vector<string>& arguments);

    /**
     * Splits a single string into a vector containing multiple substrings
     * @param str The string to be split
     * @param delim The delimiter used to split the string; defaults to a space
     * @return A vector containing the substrings derived from the original string as split by the specified delimiter
     */
    vector<string> str_split(const string& str, char delim = ' ');

    /**
     * Converts an epoch from time(NULL) to an ISO-8601 date format string (YYYY/MM/DDTHH:MM:SS)
     * @param epoch The time to be converted into a string
     * @return A string representation of the specified epoch in ISO-8601 format, if conversion was successful; otherwise, an empty string
     */
    string epoch_to_iso8601(time_t epoch);

    /**
     * Converts an epoch from time(NULL) to an uptime string in the form "d..d days, hh hours, mm minutes, and ss seconds."
     * @param epoch The time to be converted into a string
     * @return A string representation of the specified epoch in ISO-8601 format, if conversion was successful.
     * Returns an empty string if an error occurs or NULL info is encountered
     */
    string epoch_to_uptime(time_t epoch);
}
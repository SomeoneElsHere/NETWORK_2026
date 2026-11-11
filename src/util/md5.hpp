#pragma once

#include <filesystem>
#include <optional>
#include <string>

using namespace std;
using namespace std::filesystem;

namespace peer_to_peer
{
    /* Adapted from Jankatay's "util.hpp" */

    /**
     * Wrapper function to calculate the MD5 of a file easily
     * @param file The filename of the file to calculate an MD5 hash for
     * @return A string containing the MD5 hash, if successful; otherwise, nullopt
     */
    optional<string> md5_of_file(const char* file);

    /**
     * Wrapper function to calculate the MD5 of a file easily
     * @param file The filename of the file to calculate an MD5 hash for
     * @return A string containing the MD5 hash, if successful; otherwise, nullopt
     */
    optional<string> md5_of_file(string file);

    /**
     * Wrapper function to calculate the MD5 of a file easily
     * @param file The filesystem path to the file to calculate an MD5 hash for
     * @return A string containing the MD5 hash, if successful; otherwise, nullopt
     */
    optional<string> md5_of_file(path& file);

    /**
     * Wrapper function to calculate the MD5 of a C-string easily
     * @param text The C-string to compute the MD5 hash of
     * @param length The number of characters in the C-string
     * @return A string containing the MD5 hash, if successful; otherwise, nullopt
     */
    optional<string> md5_of_string(const char* text, size_t length);

    /**
     * Wrapper function to calculate the MD5 of a string easily
     * @param text The string to compute the MD5 hash of
     * @return A string containing the MD5 hash, if successful; otherwise, nullopt
     */
    optional<string> md5_of_string(string text);

    /**
     * Checks if a given string is a valid MD5 hash
     * @param alleged_md5 The string to check
     * @return true if the specified string contains a valid MD5 hash; otherwise, false
     */
    bool valid_md5(string& alleged_md5);
}
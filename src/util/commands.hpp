#pragma once

#include <string>

using namespace std;

namespace peer_to_peer
{
    /// The character used to denote the beginning of a command string
    static constexpr char CH_COMMAND_OPEN = '<';

    /// The character used to denote the ending of a command string
    static constexpr char CH_COMMAND_CLOSE = '>';

    /**
     * Attempts to parse a command string from a raw char buffer
     * @param buffer The buffer to attempt to parse a command string from
     * @param length The length of the buffer
     * @param out_cmd_str The memory location to store the parsed command string; pass nullptr to skip creation of the command string
     * @return true if the span of characters defined by the specified buffer and length follows the expected format of a command string (<command_name args...>); otherwise, false
     */
    bool try_get_command_str(char buffer[], size_t length, string* out_cmd_str);

    /**
     * Utility method used to compare a command string with a given command name
     * @param command_str The user-provided command string to check
     * @param command_name The name of the command being checked for
     * @param out_cutoff_index The memory location to store the index in the specified command string at which the first character after all characters in the specified command name occur.
     * Stores 0 if the command name was not found.
     * Does not store anything if nullptr is passed.
     * @return true if the command string starts with the specified command name; otherwise, false
     */
    bool command_compare(const string& command_str, const string& command_name, size_t* out_cutoff_index);
} // namespace peer_to_peer
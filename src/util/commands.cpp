#include "commands.hpp"

using namespace std;

namespace peer_to_peer
{
    /// Validates "<name args...>" framing in a raw buffer and extracts the inner command string
    bool try_get_command_str(char buffer[], size_t length, string* out_cmd_str)
    {
        // Check if buffer starts with command opener char
        if(buffer[0] == CH_COMMAND_OPEN)
        {
            int end = length - 1;
            bool foundCloser = false;
            while(end > 0)
            {
                if(buffer[end] == CH_COMMAND_CLOSE)
                {
                    foundCloser = true;
                    break;
                }
                else if(!isspace(buffer[end]) && buffer[end] != '\0') // Uncomment this to allow space characters in command string outside of opener/closer
                    break;

                end--;
            }

            if(foundCloser)
            {
                if(out_cmd_str != nullptr)
                    *out_cmd_str = string(buffer + sizeof(char), end - 1);

                return true;
            }
        }

        return false;
    }

    /// Prefix-matches a command name against a command string; stores the offset where its arguments begin
    bool command_compare(const string& command_str, const string& command_name, size_t* out_cutoff_index)
    {
        int cutoff_index = 0;
        if(out_cutoff_index != nullptr)
            *out_cutoff_index = cutoff_index;

        for(int i = 0; i < command_name.length(); i++)
        {
            if(i >= command_str.length())
                return false;

            // Space-like characters needn't match 100%
            if(isspace(command_str[i]) && isspace(command_name[i]))
                continue;

            if(command_str[i] != command_name[i])
                return false;
        }

        cutoff_index = command_name.length();

        // Uncomment to disallow a space after command name if nothing comes after that space
        /*if(command_str.length() == command_name.length() + 1)
            return false;
        else*/
        if(command_str.length() > command_name.length() + 1)
        {
            if(!isspace(command_str[command_name.length()]))
                return false;

            cutoff_index++;
        }

        if(out_cutoff_index != nullptr)
        {
            *out_cutoff_index = cutoff_index;

            // Skip next space
            // if(isspace(command_str[command_name.length()]))
            //    *out_cutoff_index += 1;
        }

        return true;
    }
} // namespace peer_to_peer

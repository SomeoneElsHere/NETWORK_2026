#include "commands.hpp"

using namespace std;

namespace peer_to_peer
{
    ///
    bool try_get_command_str(char buffer[], size_t length, string* out_cmd_str)
    {
        // Check if buffer starts with command opener char
        //string raw_cmd_string(buffer, length);
        //if(decapsulate(raw_cmd_string))
        if(buffer[0] == CH_COMMAND_OPEN)
        {
            //if(out_cmd_str != nullptr)
            //    *out_cmd_str = raw_cmd_string;//string(buffer + sizeof(char), end - 1);
            //
            //return true;
            int end = length - 1;//raw_cmd_string.length() - 1;
            bool foundCloser = false;
            while(end > 0)
            {
                //string f = "Checking character '";
                //f += buffer[end];
                //f += "'.";

                //_debug_print(std::move(f));
                if(buffer[end] == CH_COMMAND_CLOSE)
                {
                    foundCloser = true;
                    break;
                }
                else if(!isspace(buffer[end]) && buffer[end] != '\0') // Uncomment this to allow space characters in command string outside of opener/closer
                    break;

                end --;
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

    ///
    bool command_compare(const string& command_str, const string& command_name, size_t* out_cutoff_index)
    {
        int cutoff_index = 0;
        if(out_cutoff_index != nullptr)
            *out_cutoff_index = cutoff_index;

        for(int i = 0; i < command_name.length(); i ++)
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
        else*/ if(command_str.length() > command_name.length() + 1)
        {
            if(!isspace(command_str[command_name.length()]))
                return false;

            cutoff_index ++;
        }

        if(out_cutoff_index != nullptr)
        {
            *out_cutoff_index = cutoff_index;

            // Skip next space
            //if(isspace(command_str[command_name.length()]))
            //    *out_cutoff_index += 1;
        }

        return true;
    }

    /* Adapted from Jankatay's "util.hpp" */

    ///
    string encapsulate(const string& msg) {
        return CH_COMMAND_OPEN + msg + CH_COMMAND_CLOSE + '\n';
    }

    ///
    string encapsulate(string&& msg) {
        string val = string(msg.length() + 3, '\0'); // 3 = <, >, \n
        val[0] = CH_COMMAND_OPEN;
        val.replace(1, msg.length(), msg, 0, msg.length());
        val[val.length() - 2] = CH_COMMAND_CLOSE;
        val[val.length() - 1] = '\n';
        return val;
    }

    /// 
    bool decapsulate(string& msg) {
      // '<'
      if(msg.empty()) return false;
      if(msg.front() != CH_COMMAND_OPEN) return false;
      msg.erase(0, 1);

      // '\n'
      if(msg.empty()) return false;
      if(msg.back() != '\n') return false;
      msg.pop_back();

      // '>'
      if(msg.empty()) return false;
      if(msg.back() != CH_COMMAND_CLOSE) return false;
      msg.pop_back();

      // success
      return true;
    }
}
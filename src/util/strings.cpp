#include "strings.hpp"

using namespace std;

constexpr int iso8601_buffer_length = 20; // remind me to update it in the year 9999

constexpr int seconds_per_minute = 60;
constexpr int seconds_per_hour = seconds_per_minute * 60;
constexpr int seconds_per_day = seconds_per_hour * 24;

namespace peer_to_peer
{
    /* Adapted from Jankatay's "util.hpp" */

    ///
    bool str_to_args(const string& str, vector<string>& arguments)
    {
        arguments = vector<string>();
        
        // sanitize and init
        if(str.empty())
            return true;

        bool dquotes = false, quotes = false, escape = false;
        string current = "";

        // while looping over the string
        for(unsigned char c : str) {
            // special characters
            if((c == '"') && (!quotes) && (!escape)) // handle double quotations
                dquotes = !dquotes;
            else if((c == '\'') && (!dquotes) && (!escape)) // handle single quotations
                quotes = !quotes;
            else if(c == '\\' && !escape) { // the escape/backslash
                escape = true; 
                continue;
            }
            else if(isspace(c) && (!quotes) && (!dquotes) && (!escape)) { // whitespace
                arguments.push_back(current);
                current.clear();
            } else if(isprint(c)) current.push_back(c);                       // normal characters
            else {                                                            // unkown, unprintable characters
                fprintf(stderr, "WARNING - Invalid character \"%du\"-> %s:%d\n", c, __FILE__, __LINE__);
                return false;
            } 
            escape = false;
        }

        // sanitize and push last string if there is any left
        if(quotes || dquotes || escape) {
          fprintf(stderr, "WARNING - Unclosed quotations or stray backslash -> %s:%d\n", __FILE__, __LINE__);
          return false;
        }
        if(!current.empty())
            arguments.push_back(current);

        return true;
    }

    ///
    vector<string> str_split(const string& str, char delim)
    {
        // init
        if(str.empty()) return vector<string>{};
        vector<string> ret = {};
        int prev = 0;

        // loop over finding the substr and adding to ret
        for(int tmp = 0; tmp < std::string::npos; prev = tmp+1) {
          tmp = str.find(delim, prev);
          if(tmp >= std::string::npos) break;
          size_t len = tmp - prev;
          ret.push_back(str.substr(prev, len));
        }
    
        // finish return
        ret.push_back(str.substr(prev));
        return ret;
    }

    ///
    string epoch_to_iso8601(time_t epoch) {
      if(epoch < 0) return ""; // sanitize

      struct tm date_utc = {};
      if (!gmtime_r(&epoch, &date_utc)) return "";

      char bufstr[iso8601_buffer_length] = "";

      strftime(bufstr, iso8601_buffer_length, "%FT%T", &date_utc);
      return bufstr; // bufstr already set empty on strftime error
    }

    ///
    string epoch_to_uptime(time_t epoch) {
      // sanitize
      int current_time = time(NULL);
      if ((epoch < 0) || (current_time < 0) || (epoch > current_time)) return "";

      time_t diff = current_time - epoch;

      // split to days, hours, and minutes
      size_t days = diff / seconds_per_day;
      diff -= days * seconds_per_day;
      size_t hours = diff / seconds_per_hour;
      diff -= hours * seconds_per_hour;
      size_t minutes = diff / seconds_per_minute;
      diff -= minutes * seconds_per_minute;

      // success, leftover diff is seconds
      string ret = to_string(days) + " days, ";
      ret += to_string(hours) + " hours, ";
      ret += to_string(minutes) + " minutes, and ";
      ret += to_string(diff) + " seconds.";
      return ret;
    }
}
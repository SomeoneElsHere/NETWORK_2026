#include "strings.hpp"

using namespace std;

constexpr int iso8601_buffer_length = 20; // remind me to update it in the year 9999

constexpr int seconds_per_minute = 60;
constexpr int seconds_per_hour = seconds_per_minute * 60;
constexpr int seconds_per_day = seconds_per_hour * 24;

namespace peer_to_peer {
    /* Adapted from Jankatay's "util.hpp" */

    /// Splits into arguments honoring quotes (spaces allowed inside quotes; backslash escapes quotes)
    bool str_to_args(const string& str, vector<string>& arguments)
    {
        arguments = vector<string>();

        // sanitize and init
        if(str.empty()) {
            return true;
        }

        bool dquotes = false, quotes = false, escape = false;
        string current = "";

        // while looping over the string
        for(unsigned char c : str)
        {
            // special characters
            if((c == '"') && (!quotes) && (!escape)) { // handle double quotations
                dquotes = !dquotes;
            } else if((c == '\'') && (!dquotes) && (!escape)) { // handle single quotations
                quotes = !quotes;
            else if(c == '\\' && !escape)
            { // the escape/backslash
                escape = true;
                continue;
            }
            else if(isspace(c) && (!quotes) && (!dquotes) && (!escape))
            { // whitespace
                arguments.push_back(current);
                current.clear();
            }
            else if(isprint(c))
                current.push_back(c); // normal characters
            else
            { // unknown, unprintable characters
                fprintf(stderr, "WARNING - Invalid character \"%du\"-> %s:%d\n", c, __FILE__, __LINE__);
                return false;
            }
            escape = false;
        }

        // sanitize and push last string if there is any left
        if(quotes || dquotes || escape)
        {
            fprintf(stderr, "WARNING - Unclosed quotations or stray backslash -> %s:%d\n", __FILE__, __LINE__);
            return false;
        }
        
        if(!current.empty()) {
            arguments.push_back(current);
        }

        return true;
    }

    /// Splits on a single delimiter char (used for peer entry lines ip:port:start:end:timestamp)
    vector<string> str_split(const string& str, char delim)
    {
        // init
        if(str.empty())
            return vector<string>{};
        vector<string> ret = {};
        int prev = 0;

        // loop over finding the substr and adding to ret
        for(int tmp = 0; tmp < std::string::npos; prev = tmp + 1)
        {
            tmp = str.find(delim, prev);
            if(tmp >= std::string::npos)
                break;
            size_t len = tmp - prev;
            ret.push_back(str.substr(prev, len));
        }

        // finish return
        ret.push_back(str.substr(prev));
        return ret;
    }

    /// time_t -> "YYYY/MM/DDTHH:MM:SS" (empty string on failure)
    string epoch_to_iso8601(time_t epoch)
    {
        if(epoch < 0)
            return ""; // sanitize

        struct tm date_utc = {};
        if(!gmtime_r(&epoch, &date_utc))
            return "";

        char bufstr[iso8601_buffer_length] = "";

        strftime(bufstr, iso8601_buffer_length, "%FT%T", &date_utc);
        return bufstr; // bufstr already set empty on strftime error
    }

    /// time_t -> "d days, hh hours, mm minutes, and ss seconds." human-readable string
    string epoch_to_uptime(time_t epoch)
    {
        // sanitize
        int current_time = time(NULL);
        if((epoch < 0) || (current_time < 0) || (epoch > current_time))
            return "";

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

    /// Strict non-negative integer parse: the whole string must be digits (rejects trailing junk)
    bool str_to_u64(const string& str, unsigned long long& out)
    {
        if(str.empty())
            return false;

        char* endptr = nullptr;
        unsigned long long value = strtoull(str.c_str(), &endptr, 10);

        // The entire string must have been consumed (no trailing characters)
        if(endptr == str.c_str() || *endptr != '\0')
            return false;

        out = value;
        return true;
    }
} // namespace peer_to_peer

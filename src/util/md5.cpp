#include "libmd/md5.h"
#include "md5.hpp"

// C++ Standard Library
#include <cstring>
#include <filesystem>

// Unix Specification
#include <sys/types.h>

using namespace std;
using namespace std::filesystem;

namespace peer_to_peer
{
    ///
    optional<string> md5_of_file(const char* file) {
      char buf[MD5_DIGEST_STRING_LENGTH];
      MD5_CTX context = {};
      MD5Init(&context);
      MD5File(file, buf);
      MD5End(&context, NULL);
      return string(buf);
    }
    
    ///
    optional<string> md5_of_file(string file) { return md5_of_file(file.c_str()); }

    ///
    optional<string> md5_of_file(path& file) { return md5_of_file(file.string()); }

    ///
    optional<string> md5_of_string(const char* text, size_t length)
    {
        uint buffer_length = length * sizeof(char);
        uint8_t data[buffer_length];
        memcpy(data, text, buffer_length);

        char buf[MD5_DIGEST_STRING_LENGTH];
        
        MD5_CTX context = {};
        MD5Init(&context);
        MD5Data(data, buffer_length, buf);
        MD5End(&context, NULL);
        return string(buf);
    }

    ///
    optional<string> md5_of_string(string text) { return md5_of_string(text.c_str(), text.length()); }

    ///
    bool valid_md5(string& alleged_md5) {
        // sanitize and split md5 string into pieces
        if(alleged_md5.length() != 32) return false;
        for(char c : alleged_md5) {
            bool valid_letter = (c >= 'a') || (c >= 'A');
            valid_letter &= (c <= 'f') || (c <= 'F');
            valid_letter |= isdigit(c);
            if(!valid_letter) return false;
        }
        // success
        return true;
    }
}
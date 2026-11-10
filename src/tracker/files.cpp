#include "files.hpp"

using namespace std;

namespace peer_to_peer::tracker
{
    /* Adapted from Jankatay's "files.hpp" */

    ///
    TrackerFile::TrackerFile(string filename, string filesize, optional<string> description, string md5) {
        //TODO: sanitize the strtoll
        this -> filename = filename;
        char* endptr = NULL;
        this -> filesize = strtoll(filesize.c_str(), &endptr, 10);
        this -> description = description;
        this -> md5 = md5;
        this -> quickstring = filename + " " + filesize + " " + md5 + "\n";
    };

    ///
    bool TrackerFile::has_filename(string& filename)
        { return this -> filename == filename; }  
        
    ///
    list<TrackerFile> produce_file_list() {
        return list<TrackerFile>();
    }
}
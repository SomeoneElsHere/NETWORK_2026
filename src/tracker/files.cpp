#include "files.hpp"
#include "../util/strings.hpp"

// C++ Standard Library
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace std;

namespace peer_to_peer::tracker {
    /* Adapted from Jankatay's "files.hpp" */

    /// The file extension used for tracker files, per the project specification ("<filename>.track")
    static const string TRACKER_FILE_EXT = ".track";

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
    bool TrackerFile::has_filename(string& filename) { 
        return this -> filename == filename; 
    }  

    string TrackerFile::to_file_contents() const {
        ostringstream out;

        // Header fields, in the order required by the project specification
        out << "Filename: " << this -> filename << "\n";
        out << "Filesize: " << this -> filesize << "\n";
        if(this -> description.has_value() && !(this -> description -> empty())) {
            out << "Description: " << this -> description.value() << "\n";
        }
        out << "MD5: " << this -> md5 << "\n";

        // Peer entries: <ip>:<port>:<start byte>:<end byte>:<time stamp>
        out << "#list of peers follows next\n";
        for(const PeerInfo& peer : this -> peers) {
            out << peer.addr << ":"
                << peer.port << ":"
                << peer.start_byte << ":"
                << peer.end_byte << ":"
                << (unsigned long long)peer.time_stamp << "\n";
        }

        return out.str();
    }

    /// Save the tracker file to the location at `directory`. 
    bool TrackerFile::save_to(const string& directory) const {
        error_code ec;
        filesystem::create_directories(directory, ec); // no-op if the directory already exists
        if (ec) {
            return false;
        }
        
        ofstream file(filesystem::path(directory) / (this -> filename + TRACKER_FILE_EXT), ios::out | ios::trunc);
        if (!file.is_open()) { 
            return false;
        }

        file << this -> to_file_contents();
        bool success = file.good();
        file.close();
        return success;
    }

    optional<TrackerFile> TrackerFile::try_load(const string& path) {
        ifstream file(path);
        if(!file.is_open())
            return nullopt;

        optional<string> filename, filesize, description, md5;
        vector<PeerInfo> peers;

        string line;
        while(getline(file, line)) {
            // Comments must be ignored, per the project specification
            if(line.empty() || line[0] == '#')
                continue;

            // Header fields
            if(line.rfind("Filename: ", 0) == 0)
                { filename = line.substr(10); continue; }
            if(line.rfind("Filesize: ", 0) == 0)
                { filesize = line.substr(10); continue; }
            if(line.rfind("Description: ", 0) == 0)
                { description = line.substr(13); continue; }
            if(line.rfind("MD5: ", 0) == 0)
                { md5 = line.substr(5); continue; }

            // Anything else should be a peer entry: <ip>:<port>:<start>:<end>:<timestamp>
            vector<string> fields = str_split(line, ':');
            if(fields.size() == 5)
            {
                unsigned long long port, start, end, stamp;
                if(str_to_u64(fields[1], port) && str_to_u64(fields[2], start)
                   && str_to_u64(fields[3], end) && str_to_u64(fields[4], stamp))
                {
                    peers.push_back(PeerInfo{
                        fields[0],
                        (int16_t)port,
                        (size_t)start,
                        (size_t)end,
                        (time_t)stamp
                    });
                }
                // Unparseable peer entries are skipped rather than failing the whole file
            }
        }
        file.close();

        // Required header fields per the project specification
        if(!filename.has_value() || !filesize.has_value() || !md5.has_value())
            return nullopt;

        TrackerFile tracker_file(filename.value(), filesize.value(), description, md5.value());
        tracker_file.peers = peers;
        return tracker_file;
    }
        
    ///
    list<TrackerFile> produce_file_list(const string& directory) {
        list<TrackerFile> files;

        error_code ec;
        filesystem::create_directories(directory, ec); // ensure the shared directory exists
        if(ec) {
            return files;
        }

        for(const auto& entry : filesystem::directory_iterator(directory, ec)) {
            if(ec) {
                break;
            }
            if(!entry.is_regular_file()) {
                continue;
            }
            if(entry.path().extension() != TRACKER_FILE_EXT) {
                continue;
            }

            optional<TrackerFile> tracker_file = TrackerFile::try_load(entry.path().string());
            if(tracker_file.has_value()) {
                files.push_back(tracker_file.value());
            }
        }

        return files;
    }
}

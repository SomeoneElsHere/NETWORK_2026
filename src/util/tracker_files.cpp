#include "util/md5.hpp"
#include "util/networking.hpp"
#include "util/strings.hpp"
#include "tracker_files.hpp"
#include "networking.hpp"

#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

using namespace std;
using namespace std::filesystem;

namespace peer_to_peer
{
    /* Adapted from Jankatay's "trackerfile.hpp" */

    ///
    list<string> list_trackers(const string& target_folder)
    {
        // init
        list<string> ret;
        error_code ferr_status = error_code();
        directory_iterator files = directory_iterator(target_folder, ferr_status);
        if(ferr_status)
            return ret;

        // go through and parse
        int idx = 0;
        for(directory_entry file : files) {
            if(!file.is_regular_file())
                continue;
            fstream fs(file.path());

            TrackerFile tf;
            if(!TrackerFile::try_parse_from_stream(fs, &tf))
                continue;

            ret.push_back(tf.get_quick_string());
        }
    
        // finish
        return ret;
    }

    /* Alternate version of TrackerFile class */

    ///
    const string TrackerFile::FILE_EXTENSION = ".track";

    ///
    TrackerFile::TrackerFile() :
        filename("."),
        filesize(0),
        description(nullopt),
        md5("0"), // Replace with "proper" MD5 hash of empty file
        peers()
    {

    }

    ///
    TrackerFile::TrackerFile(string filename, size_t filesize, optional<string> description, string md5, vector<PeerInfo> peers) :
        filename(filename),
        filesize(filesize),
        description(description),
        md5(md5),
        peers(peers) {}

    const string KEY_FILENAME = "Filename";
    const string KEY_FILESIZE = "Filesize";
    const string KEY_DESCRIPTION = "Description";
    const string KEY_MD5 = "MD5";

    const int PINF_IND_PORT = 0;
    const int PINF_IND_BYTE_START = 1;
    const int PINF_IND_BYTE_END = 2;
    const int PINF_IND_TIMESTAMP = 3;

    ///
    bool TrackerFile::try_parse_from_args(vector<string> args, TrackerFile* out_trackerfile)
    {
        // Filesize
        if(!valid_uint(args[1]))
            return false;

        // MD5 Hash
        if(!valid_md5(args[3]))
            return false;

        // Peer IP Address & Port
        if(!valid_address(args[4], args[5]))
            return false;

        if(out_trackerfile != nullptr)
        {
            size_t filesize = stoul(args[1]);

            PeerInfo pinf;
            pinf.addr = args[4];
            pinf.port = stoi(args[5]);
            pinf.start_byte = 0;
            pinf.end_byte = filesize;
            pinf.time_stamp = time(NULL);

            vector<PeerInfo> pinfs;
            pinfs.push_back(pinf);

            *out_trackerfile = TrackerFile(
                args[0],
                filesize,
                args[2],
                args[3],
                pinfs
            );
        }

        return true;
    }

    /**
     * Attempts to parse one line of a tracker file being read from an input stream
     * @param stream The stream to scan characters through
     * @param out_key A reference to the parsed key
     * @param out_values A reference to all parsed values
     * @return true if the line was parsed correctly; otherwise, false
     */
    bool try_parse_line(istream& stream, string& out_key, vector<string>& out_values)
    {
        string next_str;

        // Initialize outputs
        out_key = "";
        out_values = vector<string>();

        bool empty_line = true;
        char next_c, last_c = '\0';
        bool parseable = true;

        while(stream.get(next_c))
        {
            if(next_c == '\n' || next_c == '\r')
            {
                // End of the line, stop reading characters
                break;
            }
            else if(next_c == TrackerFile::CH_COMMENT)
            {
                // Read remaining characters until next newline
                parseable = false;
            }

            if(parseable)
            {
                if(next_c == TrackerFile::CH_KEY_DELIMITER)
                {
                    if(out_key.empty())
                    {
                        out_key = next_str;
                    }
                    else
                    {
                        out_values.push_back(next_str);
                    }

                    next_str = "";
                }
                else if(!isspace(next_c))
                {
                    next_str += next_c;
                }
                empty_line = false;
                //else if(last_c != TrackerFile::CH_KEY_DELIMITER && out_values.empty())
                //{
                //    // Spaces only valid after first delimiter
                //    return false;
                //}
            }
            
            // Save last read character before reading next one
            last_c = next_c;
        }

        // Add last string onto value set
        if(!next_str.empty())
        {
            if(out_key.empty())
                return false;

            out_values.push_back(next_str);
        }

        // A key and at least one value are required for a line to be considered correct
        if(!empty_line && (out_key.empty() || out_values.size() == 0))
            return false;

        return true;
    }

    ///
    bool TrackerFile::try_parse_from_stream(istream& stream, TrackerFile* out_trackerfile)
    {
        if(!stream.good())
        {
            // Stream is not readable
            //if(out_trackerfile != nullptr)
            //    *out_trackerfile = nullopt;
            return false;
        }

        optional<string> filename = nullopt;
        optional<size_t> filesize = nullopt;
        optional<string> description = nullopt;
        optional<string> md5 = nullopt;
        vector<PeerInfo> peers;

        string key;
        vector<string> values;

        PeerInfo next_peer;

        bool allRequiredFields = false;

        while(!stream.eof())
        {
            if(!try_parse_line(stream, key, values))
            {
                // Malformed line
                //if(out_trackerfile != nullptr)
                //    *out_trackerfile = nullopt;
                return false;
            }

            // Skip line
            if(key.empty() && values.empty())
                continue;

            if(key == KEY_FILENAME)
            {
                // Filename
                filename = values[0];

                if(filesize.has_value() && md5.has_value())
                    allRequiredFields = true;
            }
            else if(key == KEY_FILESIZE)
            {
                // Filesize
                filesize = stoul(values[0]);

                if(filename.has_value() && md5.has_value())
                    allRequiredFields = true;
            }
            else if(key == KEY_DESCRIPTION)
            {
                // Description (optional)
                description = values[0];
            }
            else if(key == KEY_MD5)
            {
                // MD5 hash
                md5 = values[0];

                if(filename.has_value() && filesize.has_value())
                    allRequiredFields = true;
            }
            else if(allRequiredFields && values.size() == 4)
            {
                // Verify that the IP address and port strings are valid
                if(!valid_address(key, values[PINF_IND_PORT]))
                    return false;

                // Peer information
                next_peer.addr = key;
                next_peer.port = stoi(values[PINF_IND_PORT]);
                next_peer.start_byte = stoi(values[PINF_IND_BYTE_START]);
                next_peer.end_byte = stoi(values[PINF_IND_BYTE_END]);
                next_peer.time_stamp = stol(values[PINF_IND_TIMESTAMP]);

                peers.push_back(next_peer);
                next_peer = PeerInfo();
            }
        }

        if(!allRequiredFields)
        {
            // At least one missing required field
            //if(out_trackerfile != nullptr)
            //    *out_trackerfile = nullopt;
            return false;
        }

        if(out_trackerfile != nullptr)
        {
            *out_trackerfile = TrackerFile(
                filename.value(),
                filesize.value(),
                description,
                md5.value(),
                peers
            );
        }
        
        return true;
    }

    /**
     * Attempts to write a line of a tracker file containing a key-value pair to an output stream
     * @param stream The stream to write the line to
     * @param key A reference to the key to be written
     * @param value The string representation of the value to be written
     * @return true if the line was able to be written; otherwise, false
     */
    bool try_write_kvp(ostream& stream, const string& key, string&& value)
    {
        stream.write(key.c_str(), key.length());
        stream.put(TrackerFile::CH_KEY_DELIMITER);
        stream.put(' ');
        stream.write(value.c_str(), value.length());
        stream.put('\n');

        return !stream.fail();
    }

    /**
     * Attempts to write a commented line to an output stream
     * @param stream The stream to write the comment to
     * @param comment The characters that follow the comment delimiter ('#')
     * @return true if the commented line was able to be written; otherwise, false
     */
    bool try_write_comment(ostream& stream, string&& comment)
    {
        stream.put(TrackerFile::CH_COMMENT);
        stream.write(comment.c_str(), comment.length());
        stream.put('\n');

        return !stream.fail();
    }

    /**
     * Attempts to write a line of a tracker file containing peer information to an output stream
     * @param stream The stream to write the line to
     * @param peer The peer information to be written
     * @return true if the line containing peer information was able to be written; otherwise, false
     */
    bool try_write_peerinfo(ostream& stream, PeerInfo& peer)
    {
        // IP Address
        stream.write(peer.addr.c_str(), peer.addr.length());
        stream.put(TrackerFile::CH_KEY_DELIMITER);

        // Port
        string num_str = to_string(peer.port);
        stream.write(num_str.c_str(), num_str.length());
        stream.put(TrackerFile::CH_KEY_DELIMITER);
        
        // Start Byte
        num_str = to_string(peer.start_byte);
        stream.write(num_str.c_str(), num_str.length());
        stream.put(TrackerFile::CH_KEY_DELIMITER);
        
        // End Byte
        num_str = to_string(peer.end_byte);
        stream.write(num_str.c_str(), num_str.length());
        stream.put(TrackerFile::CH_KEY_DELIMITER);

        // Timestamp
        num_str = to_string(peer.time_stamp);
        stream.write(num_str.c_str(), num_str.length());

        return !stream.fail();
    }

    ///
    bool TrackerFile::try_write_to_stream(ostream& stream)
    {
        if(!stream.good())
        {
            // Stream is not writable
            return false;
        }

        // Filename
        if(!try_write_kvp(stream, KEY_FILENAME, this -> get_filename()))
            return false;

        // Filesize
        if(!try_write_kvp(stream, KEY_FILESIZE, to_string(this -> get_filesize())))
            return false;

        // Description (optional, defaults to empty)
        if(!try_write_kvp(stream, KEY_DESCRIPTION, this -> get_description().value_or("")))
            return false;

        // MD5 hash
        if(!try_write_kvp(stream, KEY_MD5, this -> get_md5()))
            return false;

        // Peer list comment
        if(!try_write_comment(stream, "list of peers follows next"))
            return false;

        // Peer information
        for(PeerInfo pinf : this -> peers)
        {
            if(!try_write_peerinfo(stream, pinf))
                return false;
            stream.put('\n');
        }

        return true;
    }

    ///
    string TrackerFile::get_filename() { return this -> filename; }

    ///
    size_t TrackerFile::get_filesize() { return this -> filesize; }

    ///
    optional<string> TrackerFile::get_description() { return this -> description; }

    ///
    string TrackerFile::get_md5() { return this -> md5; }

    ///
    vector<PeerInfo> TrackerFile::get_peers() { return this -> peers; }

    ///
    string TrackerFile::get_quick_string()
    {
        if(this -> quickstring.empty())
            this -> quickstring = this -> filename + " " + to_string(this -> filesize) + " " + this -> md5;

        return this -> quickstring;
    }

    ///
    bool TrackerFile::try_add_peer(PeerInfo&& peer)
    {
        // Don't add it if it already exists
        for(PeerInfo& pinf : this -> peers)
        {
            if(pinf.addr == peer.addr && pinf.port == peer.port)
            {
                return false;
            }
        }

        this -> peers.push_back(peer);
        return true;
    }

    ///
    bool TrackerFile::try_remove_peer(string peer_addr, int16_t peer_port)
    {
        // Don't remove it if it doesn't exists
        auto peer_it = this -> peers.begin();
        
        while(peer_it != this -> peers.end())
        {
            PeerInfo* pinf = peer_it.base();
            if(pinf -> addr == peer_addr && pinf -> port == peer_port)
            {
                this -> peers.erase(peer_it);
                return true;
            }
            else
                peer_it ++;
        }

        return false;
    }
}
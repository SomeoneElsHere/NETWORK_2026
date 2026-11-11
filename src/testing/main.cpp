#include "util/commands.hpp"
#include "util/tracker_files.hpp"
#include "util/md5.hpp"

#include <fstream>
#include <iostream>
#include <optional>
#include <string>

using namespace std;
using namespace peer_to_peer;

/// Test method that demonstrates parsing a TrackerFile object from a file
int tracker_file_reading()
{
    string fname = "testfile.txt" + TrackerFile::FILE_EXTENSION;

    optional<string> md5Str = md5_of_file(fname);
    if(md5Str.has_value())
        cout << "MD5 hash is: " << md5Str.value() << endl;

    ifstream fileStream(fname);
    if(!fileStream.is_open())
    {
        // Failed to open the specified file for reading
        return 1;
    }

    TrackerFile t_file;
    if(TrackerFile::try_parse_from_stream(fileStream, &t_file))
    {
        cout << "Filename: " << t_file.get_filename() << endl;
        cout << "Filesize: " << t_file.get_filesize() << " bytes" << endl;
        optional<string> desc = t_file.get_description();
        if(desc.has_value())
            cout << "Description: " << desc.value() << endl;
        cout << "MD5: " << t_file.get_md5() << endl << endl;

        for(PeerInfo pinf : t_file.get_peers())
        {
            cout << "---" << endl;
            cout << "IP Address: " << pinf.addr << endl;
            cout << "Port: " << pinf.port << endl;
            cout << "Start Byte: " << pinf.start_byte << endl;
            cout << "End Byte: " << pinf.end_byte << endl;
            cout << "Timestamp: " << pinf.time_stamp << endl;
        }
    }
    else
    {
        cout << "Tracker file was improperly formatted.";
    }
    fileStream.close();

    string input;
    getline(cin, input);
    return 0;
}

#define PROMPT(msg) input.clear(); cout << msg; getline(cin, input);

/// Test method that demonstrates writing the contents of a TrackerFile object to a file
int tracker_file_writing()
{
    string input;
    
    // Filename
    PROMPT("Filename: ");
    string filename = input;
    
    // Filesize
    PROMPT("Filesize: ");
    size_t filesize = stoul(input);

    // Description
    PROMPT("Description: ");
    optional<string> description = nullopt;
    if(!input.empty())
        description = input;

    // MD5 Hash
    PROMPT("Content (for MD5 hash): ");
    string md5_hash = md5_of_file(input).value_or("");

    vector<PeerInfo> peers;
    while(true)
    {
        input.clear();
        while(input != "Y" && input != "N")
        {
            PROMPT("Add peer? [Y/N] ");
        }

        if(input == "N")
            break;

        // IP Address
        PROMPT("IP Address: ");
        string ip_addr = input;

        // Port
        PROMPT("Port: ");
        int16_t port = stoi(input);

        // Start Byte
        PROMPT("Start Byte: ");
        size_t startbyte = stoi(input);

        // End Byte
        PROMPT("End Byte: ");
        size_t endbyte = stoi(input);

        // Timestamp
        PROMPT("Timestamp");
        time_t timestamp = stol(input);

        peers.push_back(
            PeerInfo
            {
                ip_addr,
                port,
                startbyte,
                endbyte,
                timestamp
            }
        );
    }

    TrackerFile tf(
        filename,
        filesize,
        description,
        md5_hash,
        peers
    );

    string fname = filename + TrackerFile::FILE_EXTENSION;

    ofstream fileStream(fname);
    if(!fileStream.is_open())
    {
        // Failed to open the specified file for reading
        return 1;
    }

    if(!tf.try_write_to_stream(fileStream))
    {
        cout << "Failed to write the tracker file." << endl;
        return 1;
    }

    return 0;
}

/// Test method that demonstrates the ability to compute the MD5 hash of a string and streams
int md5_strings()
{
    string input;

    // MD5 Hash
    PROMPT("Content (for MD5 hash): ");
    string md5_hash = md5_of_file(input).value_or("");

    optional<string> hash = md5_of_string(input);
    if(!hash.has_value())
    {
        cout << "Failed to convert string to MD5 hash." << endl;
        return 1;
    }

    cout << "Hash of string: " << hash.value() << endl;

    const string fname = "temp";
    ofstream fileStream(fname);
    if(!fileStream.is_open())
    {
        // Failed to open the specified file for reading
        return 2;
    }

    fileStream.write(input.c_str(), input.length());
    fileStream.close();

    optional<string> md5Str = md5_of_file(fname);
    if(md5Str.has_value())
        cout << "Hash of file: " << md5Str.value() << endl;
    else
    {
        cout << "File could not be converted to MD5 hash." << endl;
        return 1;
    }

    return 0;
}

/// Test method that demonstrates encapsulation and decapsulation
int encap_decap()
{
    string input;

    // Content
    PROMPT("Content: ");

    string encapsulated = encapsulate(std::move(input));
}

/* Program used for testing utility methods and other code without peer/tracker */
int main()
{
    return md5_strings();
}
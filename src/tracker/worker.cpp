#include "util/commands.hpp"
#include "util/misc.hpp"
#include "util/networking.hpp"
#include "util/strings.hpp"
#include "util/tracker_files.hpp"
#include "worker.hpp"

// C++ Standard Library
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

// Unix Specification
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;
using namespace std::filesystem;

namespace peer_to_peer::tracker
{
    static string ip_addr_str(const sockaddr_in& peer_ipv4)
    {
        char ipaddr[INET_ADDRSTRLEN];
        inet_ntop(
            AF_INET,
            &(peer_ipv4.sin_addr),
            ipaddr,
            INET_ADDRSTRLEN
        ); //get ip as char*

        return string(ipaddr);
    }

    ///
    Worker::Worker(const string shared_directory, int peer_fd, const sockaddr_in& peer_ipv4, bool close_fd_on_completion) :
        shared_directory(shared_directory),
        peer_fd(peer_fd),
        peer_address(ip_addr_str(peer_ipv4)),
        peer_port(peer_ipv4.sin_port),
        close_fd_on_completion(close_fd_on_completion)
    {
        this -> work_thread = new thread(&Worker::work, this);
        this -> work_thread -> detach(); //let it do its thang
        this -> lock = new mutex();
        this -> completed = false;
        this -> killswitch = false;
    }

    ///
    Worker::~Worker()
    {
        if(!this -> completed)
        {
            this -> signal_to_stop();
        }
        this -> wait_for_completion();

        delete this -> lock;
    }

    ///
    string Worker::get_peer_str()
    {
        if(this -> peer_str.empty())
            this -> peer_str += this -> peer_address + ':' + to_string(this -> peer_port);

        return this -> peer_str;
    }

    ///
    bool Worker::is_complete() { return this -> completed; }

    ///
    void Worker::signal_to_stop()
    {
        int peer_fd = this -> peer_fd.load();
        if(peer_fd != -1)
        {
            if(shutdown(peer_fd, SHUT_RD) == 0)
                this -> peer_fd.store(-1);
        }
        
        if(!this -> killswitch.load() && !this -> completed)
            this -> killswitch.store(true);
    }

    ///
    void Worker::wait_for_completion()
    {
        wait_and_dispose_thread(this -> work_thread);
        //if(this -> work_thread != nullptr)
        //{
        //    if(this -> work_thread -> joinable())
        //        this -> work_thread -> join();
        //    delete this -> work_thread;
        //    this -> work_thread = nullptr;
        //}
        this -> killswitch.store(false);
    }

    constexpr int MAX_MSG_SIZE = 255;

    ///**
    // * Attempts to parse a command string from a raw char buffer
    // * @param buffer The buffer to attempt to parse a command string from
    // * @param length The length of the buffer
    // * @param out_cmd_str The memory location to store the parsed command string; pass nullptr to skip creation of the command string
    // * @return true if the span of characters defined by the specified buffer and length follows the expected format of a command string (<command_name args...>); otherwise, false
    // */
    //static bool try_get_command_str(char buffer[], size_t length, string* out_cmd_str)
    //{
    //    // Check if buffer starts with command opener char
    //    if(buffer[0] == CH_COMMAND_OPEN)
    //    {
    //        int end = length - 1;
    //        bool foundCloser = false;
    //        while(end > 0)
    //        {
    //            //string f = "Checking character '";
    //            //f += buffer[end];
    //            //f += "'.";
//
    //            //_debug_print(std::move(f));
    //            if(buffer[end] == CH_COMMAND_CLOSE)
    //            {
    //                foundCloser = true;
    //                break;
    //            }
    //            else if(!isspace(buffer[end]) && buffer[end] != '\0') // Uncomment this to allow space characters in command string outside of opener/closer
    //                break;
//
    //            end --;
    //        }
//
    //        if(foundCloser)
    //        {
    //            if(out_cmd_str != nullptr)
    //                *out_cmd_str = string(buffer + sizeof(char), end - 1);
    //            
    //            return true;
    //        }
    //    }
//
    //    return false;
    //}

    ///
    void Worker::work()
    {
        char buf[MAX_MSG_SIZE];
        while(!(this -> killswitch.load()))
        {
            this_thread::sleep_for(WORK_DELAY);

            fill_n(buf, MAX_MSG_SIZE, '\0');
            int recvBytes = recv(this -> peer_fd, buf, sizeof(buf)-1, 0);
            if (recvBytes <= 0) // if failure to recieve stop. Store data at buf
            {
                break;
            }

            _debug_print("Received message: " + string(buf));
            
            if(buf[0] == CH_COMMAND_OPEN)
            {
                _debug_print("Received message may be in command format.");
                string command_str;
                // Received data is a command
                if(try_get_command_str(buf, recvBytes, &command_str))
                {
                    // Command was formatted properly
                    _debug_print("Command string: '" + command_str + "'.");
                    if(!this -> try_handle_command(command_str))
                    {
                        // Command was not successfully handled
                        send_string(this -> peer_fd, "Command failed.");
                        _debug_print("The command was not successfully handled.");
                    }
                }
                else
                {
                    // Command was not formatted properly
                    send_string(this -> peer_fd, "Improper format");
                    _debug_print("Received message was improperly formatted as command.");
                }
            }
            else 
            {
                // Received data is part of a file
                send_string(this -> peer_fd, "Not a command.");
                _debug_print("Received message was not in command format.");
            }
        }

        if(this -> close_fd_on_completion)
        {
            // Close peer connection before end of thread
            int peer_fd = this -> peer_fd.load();
            if(peer_fd != -1)
            {
                close(peer_fd);
                this -> peer_fd.store(-1);
            }
        }

        this -> completed = true;
    }

    ///**
    // * Utility method used to compare a command string with a given command name
    // * @param command_str The user-provided command string to check
    // * @param command_name The name of the command being checked for
    // * @param out_cutoff_index The memory location to store the index in the specified command string at which the first character after all characters in the specified command name occur.
    // * Stores 0 if the command name was not found.
    // * Does not store anything if nullptr is passed.
    // * @return true if the command string starts with the specified command name; otherwise, false
    // */
    //bool command_compare(const string& command_str, const string& command_name, size_t* out_cutoff_index)
    //{
    //    int cutoff_index = 0;
    //    if(out_cutoff_index != nullptr)
    //        *out_cutoff_index = cutoff_index;
//
    //    for(int i = 0; i < command_name.length(); i ++)
    //    {
    //        if(i >= command_str.length())
    //            return false;
//
    //        // Space-like characters needn't match 100%
    //        if(isspace(command_str[i]) && isspace(command_name[i]))
    //            continue;
//
    //        if(command_str[i] != command_name[i])
    //            return false;
    //    }
//
    //    cutoff_index = command_name.length();
//
    //    // Uncomment to disallow a space after command name if nothing comes after that space
    //    /*if(command_str.length() == command_name.length() + 1)
    //        return false;
    //    else*/ if(command_str.length() > command_name.length() + 1)
    //    {
    //        if(!isspace(command_str[command_name.length()]))
    //            return false;
//
    //        cutoff_index ++;
    //    }
//
    //    if(out_cutoff_index != nullptr)
    //    {
    //        *out_cutoff_index = cutoff_index;
//
    //        // Skip next space
    //        //if(isspace(command_str[command_name.length()]))
    //        //    *out_cutoff_index += 1;
    //    }
//
    //    return true;
    //}

    ///
    bool Worker::try_handle_command(const string& command_str)
    {
        // Determine the name of the command to run
        size_t args_ind = 0;
        switch(command_str[0])
        {
            case CMDCH_REQ_LIST:
                if(command_compare(command_str, COMMAND_REQ_LIST, nullptr))
                {
                    this -> handle_req_list();
                    return true;
                }
                break;
            case CMDCH_GET:
                if(command_compare(command_str, COMMAND_GET, &args_ind))
                {
                    this -> handle_get_tracker(command_str.substr(args_ind));
                    return true;
                }
                break;
            case CMDCH_TRACKER_CREATE:
                if(command_compare(command_str, COMMAND_TRACKER_CREATE, &args_ind))
                {
                    vector<string> arguments;
                    if(str_to_args(command_str.substr(args_ind), arguments))
                    {
                        this -> handle_create_tracker(arguments);
                        return true;
                    }
                }
                break;
            //case CMDCH_TRACKER_UPDATE:
            //    if(command_compare(command_str, COMMAND_TRACKER_UPDATE, &args_ind))
            //    {
            //        vector<string> arguments;
            //        if(str_to_args(command_str.substr(args_ind), arguments))
            //        {
            //            this -> handle_update_tracker(arguments);
            //            return true;
            //        }
            //    }
            //    break;
        }

        // Command string did not start with a valid command
        return false;
    }

    ///
    void Worker::handle_req_list()
    {
        list<string> tracker_files = list_trackers(this -> shared_directory);

        // <REP LIST X>
        string message = encapsulate(COMMAND_RESPONSE_PREFIX + "LIST " + to_string(tracker_files.size()));
        send_string(this -> peer_fd, message);
        message.clear();

        // <# filename filesize fileMD5>
        int num = 1;
        for(string tracker_file : tracker_files)
        {
            message = encapsulate(to_string(num) + " " + tracker_file);
            send_string(this -> peer_fd, message);
            message.clear();
            num ++;
        }

        // <REP END>
        message = encapsulate(COMMAND_RESPONSE_PREFIX + "END");
        send_string(this -> peer_fd, message);

        cout << ("Peer " + this -> get_peer_str() + " requested tracker file list.") << endl;
    }

    ///
    void Worker::handle_get_tracker(const string& filename)
    {
        string message = encapsulate(COMMAND_RESPONSE_PREFIX + " GET BEGIN");

        //send_string(this -> peer_fd, "GET '" + filename + "' REQUESTED");
        //_debug_print("GET: '" + filename + "'");

        cout << ("Peer " + this -> get_peer_str() + " requested tracker file for '" + filename + "'.") << endl;
    }

    ///
    void Worker::send_result_string(const string& command_name, const string& result)
    {
        send_string(this -> peer_fd, encapsulate(command_name + " " + result));
    }

    ///
    void Worker::handle_create_tracker(const vector<string>& ct_args)
    {
        //string cmdstr = "createtracker: ";
        //bool sep = false;
        //for(string ct_arg : ct_args)
        //{
        //    if(sep)
        //        cmdstr += ", ";
//
        //    cmdstr += "'" + ct_arg + "'";
        //    sep = true;
        //}
//
        //_debug_print(cmdstr);

        vector<string> args = ct_args;
        /* Adapted from Jankatay's proc_create function in worker.hpp */

        // get args
        if(args.size() == 5) args.emplace(args.begin() + 2, string(""));
        if(args.size() != 6) { // missing arguments
            _debug_print("Missing arguments");
            this -> send_result_string(COMMAND_TRACKER_CREATE, COMMAND_RESULT_FAIL);
            return; // still the command was right, technically
        }; 

        /* My code again */
        string tf_filename = this -> shared_directory + "/" + args[0] + TrackerFile::FILE_EXTENSION;
        if(exists(tf_filename))
        {
            _debug_print("File already exists");
            this -> send_result_string(COMMAND_TRACKER_CREATE, COMMAND_RESULT_FILE_ERROR);
            return;
        }

        TrackerFile tf_obj;
        if(TrackerFile::try_parse_from_args(args, &tf_obj))
        {
            bool success = false;

            /* Jankatay came up with the idea to use a mutex here */
            this -> lock -> lock();

            ofstream tf_str(tf_filename);
            if(tf_obj.try_write_to_stream(tf_str))
            {
                this -> send_result_string(COMMAND_TRACKER_CREATE, COMMAND_RESULT_SUCCESS);
                success = true;
            }
            tf_str.close();

            this -> lock -> unlock();

            if(success)
                return;
        }
        
        this -> send_result_string(COMMAND_TRACKER_CREATE, COMMAND_RESULT_FAIL);
        return;
    }
}
#include "../util/commands.hpp"
#include "../util/misc.hpp"
#include "../util/networking.hpp"
#include "../util/strings.hpp"
#include "files.hpp"
#include "worker.hpp"

// C++ Standard Library
#include <cctype>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>

// Unix Specification
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

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
    Worker::Worker(int peer_fd, const sockaddr_in& peer_ipv4, list<TrackerFile> tracker_files, bool close_fd_on_completion) :
        peer_fd(peer_fd),
        peer_address(ip_addr_str(peer_ipv4)),
        close_fd_on_completion(close_fd_on_completion)
    {
        this -> work_thread = new thread(&Worker::work, this);
        this -> work_thread -> detach(); //let it do its thang
        this -> lock = new mutex();
        this -> completed = false;
        this -> killswitch = false;

        this -> tracker_files = tracker_files;
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
        send_string(this -> peer_fd, "LIST REQUESTED");
        _debug_print("REQ LIST");
    }

    ///
    void Worker::handle_get_tracker(const string& filename)
    {
        send_string(this -> peer_fd, "GET '" + filename + "' REQUESTED");
        _debug_print("GET: '" + filename + "'");
    }

    ///
    void Worker::handle_create_tracker(const vector<string>& ct_args)
    {
        string cmdstr = "createtracker: ";
        bool sep = false;
        for(string ct_arg : ct_args)
        {
            if(sep)
                cmdstr += ", ";

            cmdstr += "'" + ct_arg + "'";
            sep = true;
        }

        
        send_string(this -> peer_fd, "CREATE TRACKER FOR REQUESTED");
        _debug_print(std::move(cmdstr));
    }
}
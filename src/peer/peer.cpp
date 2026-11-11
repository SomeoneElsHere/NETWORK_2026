#include "util/commands.hpp"
#include "util/misc.hpp"
#include "util/networking.hpp"
#include "util/strings.hpp"
#include "peer.hpp"

// C++ Standard Library
#include <iostream>

// Unix Specification
#include <unistd.h>

using namespace std;
using namespace peer_to_peer::peer;



namespace peer_to_peer
{
    ///
    Peer::Peer()
    {
        sockaddr_in default_addr;
        
        default_addr.sin_family = AF_INET;
        default_addr.sin_port = htons(TRACKER_PORT);
        default_addr.sin_addr.s_addr = INADDR_ANY;
        
        this -> client_config = PeerClientConfig{
            default_addr,
            1
        };
        this -> tracker_socket_fd = -1;
        this -> tracker_listener = nullptr;

        this -> killswitch = false;
        this -> actively_listening = false;
    }
    
    ///
    Peer::Peer(PeerClientConfig clientConfig)
    {
        this -> client_config = clientConfig;
        this -> tracker_socket_fd = -1;
        this -> tracker_listener = nullptr;
        
        this -> killswitch = false;
        this -> actively_listening = false;
    }

    ///
    Peer::~Peer()
    {
        this -> close_peer();
    }

    constexpr int MAX_MSG_SIZE = 255;

    ///
    void Peer::listen_to_tracker()
    {   
        _debug_print("Tracker listening thread started.");
        this -> actively_listening = true;

        char buf[MAX_MSG_SIZE];
        bool terminated = false;
        while(!(this -> killswitch.load()) && !terminated)
        {
            this_thread::sleep_for(TRACKER_LISTEN_DELAY);

            string received_str;
            if(!try_receive_string(this -> tracker_socket_fd, buf, MAX_MSG_SIZE, &received_str))
            {
                _debug_print("Connection suddenly terminated.");
                break;
            }
            //fill_n(buf, MAX_MSG_SIZE, '\0');
            //int recvBytes = recv(this -> tracker_socket_fd, buf, sizeof(buf)-1, 0);
            //if (recvBytes <= 0) // if failure to recieve stop. Store data at buf
            //{
            //    _debug_print("Connection suddenly terminated.");
            //    break;
            //}

            string full_message = string(buf);
            if(decapsulate(full_message))
            {
                cout << (CH_COMMAND_OPEN + full_message + CH_COMMAND_CLOSE + '\n');

                vector<string> resp_args = str_split(full_message);
                if(resp_args[0] == COMMAND_RESPONSE_PREFIX)
                {
                    // LIST Response
                    if(resp_args[1] == "LIST")
                    {
                        int tf_count = stoi(resp_args[2]);
                        _debug_print("Will receive " + resp_args[2] + " tracker files.");

                        // <# filename filesize fileMD5>
                        while(tf_count > 0)
                        {
                            if(!try_receive_string(this -> tracker_socket_fd, buf, MAX_MSG_SIZE, &received_str))
                            {
                                _debug_print("Connection suddenly terminated in the middle of list request.");
                                terminated = true;
                                break;
                            }

                            cout << full_message;
                        }

                        // <REP LIST END>
                        if(!terminated)
                        {
                            if(!try_receive_string(this -> tracker_socket_fd, buf, MAX_MSG_SIZE, &received_str))
                            {
                                _debug_print("Connection suddenly terminated in the middle of list request.");
                                terminated = true;
                            }
                            else if(!decapsulate(full_message))
                            {
                                _debug_print("Not encapsulated.");
                                terminated = true;
                            }
                            else if(full_message != (COMMAND_RESPONSE_PREFIX + "LIST END"))
                            {
                                _debug_print("Final list item incorrect.");
                                terminated = true;
                            }
                        }
                        
                    }
                    // GET Response
                    else if(resp_args[1] == "GET")
                    {

                    }
                }
                else if(resp_args[0] == "createtracker" || resp_args[1] == "updatetracker")
                {
                    // Nothing to do.
                }
            }
            else
            {
                _debug_print("Received text '" + string(buf) + "'.");
            }
        }

        if(this -> killswitch.load())
            _debug_print("Killswitch was activated.");

        this -> actively_listening = false;
        _debug_print("Tracker listening thread stopped.");
    }

    ///
    bool Peer::is_listening() { return this -> actively_listening; }

    ///
    bool Peer::try_init_socket(int* error_code)
    {
        if(this -> tracker_socket_fd >= 0)
            return false;
    
        tracker_socket_fd = socket(
            AF_INET,
            SOCK_STREAM,
            0
        );
    
        return check_for_error(tracker_socket_fd < 0, error_code);
    }

    ///
    bool Peer::try_connect(int* error_code)
    {
        if(this -> tracker_socket_fd < 0)
            return false;
    
        sockaddr* tracker_socket_addr = (sockaddr*)(&this -> client_config.tracker_addr);
    
        int success = connect(
            this -> tracker_socket_fd,
            tracker_socket_addr,
            sizeof(this -> client_config.tracker_addr)
        );
    
        if(success == 0)
        {
            this -> tracker_listener = new thread(&Peer::listen_to_tracker, this);
            this -> tracker_listener -> detach();
            _debug_print("Tracker listener thread started.");
        }
        return check_for_error(success == -1, error_code);
    }

    ///
    bool Peer::try_send_to_tracker(std::string message, int len, int* error_code)
    {
        if(this -> tracker_socket_fd < 0)
            return false;
    
        //std::string content = (std::to_string(len) +":"+ message);
        size_t sentBytes = send_string(
            this -> tracker_socket_fd, 
            message
        );
    
        return check_for_error(sentBytes == -1, error_code);
    }

    ///
    void Peer::close_peer()
    {
        // Wait for peer listening thread to close
        if(this -> tracker_listener != nullptr)
        {  
            this -> killswitch.store(true);
            _debug_print("Closing tracker server connection...");
            wait_and_dispose_thread(this -> tracker_listener);
            this -> killswitch.store(false);

            _debug_print("Connection to tracking server closed.");
        }

        // Close peer socket
        if(this -> tracker_socket_fd < 0)
        {
            _debug_print("Closing socket...");
            close(this -> tracker_socket_fd);
            this -> tracker_socket_fd = -1;
            _debug_print("Socket closed.");
        }
    }
}
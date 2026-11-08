#include "config.hpp"
#include "tracker.hpp"

#include <iostream>

using namespace std;
using namespace peer_to_peer;
using namespace peer_to_peer::tracker;

void print_error_code(int error_code, bool fatal, optional<string> context_str)
{
    if(fatal)
        cout << "A fatal problem occurred while the tracker server was listening. Error code: " << error_code << endl;
    else
        cout << "A problem occurred while the tracker server was listening. Error code: " << error_code << endl;

    if(context_str.has_value())
        cout << "- Context was: \"" << context_str.value() << "\"" << endl;
}

const string commandstr_exit = "exit";

int main()
{
    optional<TrackerServerConfig> trackerConfig = try_parse_sconfig("./sconfig");
    if(!trackerConfig.has_value())
    {
        cout << "The specified sconfig file could not be read. Does it exist, and is it formatted properly?" << endl;
        return 1;
    }

    Tracker tracker(trackerConfig.value());
    if(!tracker.try_init())
    {
        cout << "Could not open a socket for listening. " << endl;
        return 2;
    }
    
    int error_code = 0;
    if(!tracker.try_listen_for_peers(&print_error_code))
    {
        cout << "A problem occurred while starting the tracker server listening thread." << endl;
        return 3;
    }

    do {
        string command;
        getline(cin, command);

        if(command == commandstr_exit)
            break;
    }
    while(tracker.is_listening());

    tracker.close_server();
    return 0;
}

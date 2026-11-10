#include "misc.hpp"

#include <iostream>

using namespace std;

namespace peer_to_peer
{
    ///
    void _debug_print(string &&text)
    {
        #ifdef DEBUG_PRINT
        cout << "[DEBUG] " << text << endl;
        #endif
    }

    ///
    bool check_for_error(bool fail_condition, int* error_code)
    {
        if(fail_condition)
        {
            if(error_code != nullptr)
                *error_code = errno;
        
            return false;
        }
      
        return true;
    }

    ///
    void wait_and_dispose_thread(thread*& t)
    {
        if(t == nullptr)
            return;

        // Wait for thread to complete
        if(t -> joinable())
        {
            t -> join();
        }

        delete t;
        t = nullptr;
    }
}
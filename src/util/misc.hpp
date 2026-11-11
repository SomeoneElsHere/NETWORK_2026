#pragma once

#include <optional>
#include <string>
#include <thread>

using namespace std;

namespace peer_to_peer
{
    /**
     * A function that is called if an error code is returned from a networking function
     * @param error_code The error code that was encountered
     * @param fatal Whether the error resulted in the calling thread/method's termination
     * @param context_str An optional string that describes the context the error occurred in
     */
    typedef void (*NET_ERROR_HANDLER)(int error_code, bool fatal, optional<string> context_str);

    /**
     * If the DEBUG_PRINT macro is set, prints a string to the standard output.
     * Otherwise, does nothing.
     * @param text A reference to the string to be printed
     */
    void _debug_print(const string& text);
    
    /**
     * If the DEBUG_PRINT macro is set, prints a string to the standard output.
     * Otherwise, does nothing.
     * @param text The rvalue string to be printed
     */
    void _debug_print(string&& text);

    /**
     * Checks if a fail condition passed, and if it did, sets an error code pointer if one was specified
     * @param fail_condition The result of the fail condition
     * @param error_code The memory location to store the error code, if the fail condition passes
     * @return false if fail_condition is true; otherwise, true
     */
    bool check_for_error(bool fail_condition, int* error_code);

    /**
     * Joins a heap-allocated thread, if possible.
     * Afterwards, destroy the heap-allocated thread and replace the pointer value with nullptr.
     * @param t The pointer at which the thread to join and then delete is stored.
     */
    void wait_and_dispose_thread(thread*& t);
}
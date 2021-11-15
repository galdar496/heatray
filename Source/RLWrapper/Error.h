//
//  Error.h
//  Heatray
//
//  Check for OpenRL errors.
//
//

#pragma once

#include <Utility/Log.h>

#include <OpenRL/rl.h>

#include <assert.h>

#define checkRLErrors() openrl::checkError(__FILE__, __LINE__)

// Error checking macro that does an error check immediately after the specified RL function.
#define RLFunc(func) func; checkRLErrors()

namespace openrl {

///
/// Check the current OpenRL state for errors.
/// If an error is detected, the system will stop. It is recommended to use the macro
/// CheckRLErrors() instead of this function directly.
///
/// @param filename Filename where the error check is being performed.
/// @param lineNumber Line number where this error check is being performed.
///
///
inline void checkError(const char* filename, int lineNumber)
{
    RLenum errorID = rlGetError();

    if (errorID != RL_NO_ERROR) {
        // An OpenRL error has occured, report it to the user.
        LOG_ERROR("%s (%d) - An OpenRL error occured: 0x%x", filename, lineNumber, errorID);

        // Stop the system.
        assert(0);
    }
}

} // namespace openrl

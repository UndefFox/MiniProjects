#pragma once

#include <stdexcept>



/* General expected application error.
 * Throw this instead of std::runtime_error when the failure
 * is expected and should be reported.
 */
struct GeneralError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};
#pragma once

#include <stdexcept>



struct GeneralError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};
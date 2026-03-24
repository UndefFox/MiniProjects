#pragma once

#include <cstdint>


/** @brief Simple implementation of DiffieHellman key
 *  generation.
 */
namespace DiffieHellman {

/**
 * @brief Applies DiffieHellman formula with given values.
 *
 * @param exp Exponent value.
 * @param base Modulo base value.
 *
 * @return Result of the expression.
 */
uint64_t applyExpression(uint64_t exp, uint64_t base = 56);


}

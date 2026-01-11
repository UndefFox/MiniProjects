#pragma once

#include <immintrin.h>
#include <cstdint>


/** @brief AVX2 optimized PRNG.
 *
 *  The implimentation is based on Xorshift random generator
 *  with rounding done by cool trick that I've found at
 *  \link https://stackoverflow.com/a/70565649 stackoverflow \endlink.
 */
class AVX2_PRNG_Generator {
private:
    __m256i seed;

public:
    AVX2_PRNG_Generator();

    /** @brief Generates 32 bit x 8 random nubers.
     *
     * @param max Maximum value that line can be.
     *
     * @return 32 bit x 8 random nubers stored in __m256i.
     */
    __m256i newRanged32(uint32_t max);

    /** @brief Generates 16 bit x 16 random nubers.
     *
     * @param max Maximum value that line can be.
     *
     * @return 16 bit x 16 random nubers stored in __m256i.
     */
    __m256i newRanged16(uint16_t max);
};

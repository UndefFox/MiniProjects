#include "avx2prng.h"


AVX2_PRNG_Generator::AVX2_PRNG_Generator() :
    seed(_mm256_set_epi32(
        static_cast<uint32_t>(std::rand()),
        static_cast<uint32_t>(std::rand()),
        static_cast<uint32_t>(std::rand()),
        static_cast<uint32_t>(std::rand()),
        static_cast<uint32_t>(std::rand()),
        static_cast<uint32_t>(std::rand()),
        static_cast<uint32_t>(std::rand()),
        static_cast<uint32_t>(std::rand())
    ))
{}

__m256i AVX2_PRNG_Generator::newRanged16(uint16_t max) {
    const __m256i left = newRanged32(max);
    const __m256i right = newRanged32(max);

    return _mm256_packus_epi32(left, right);
}

__m256i AVX2_PRNG_Generator::newRanged32(uint32_t max) {
    __m256i temp = _mm256_setzero_si256();

    temp = _mm256_slli_epi32(seed, 13);
    seed = _mm256_xor_si256(seed, temp);

    temp = _mm256_srai_epi32(seed, 7);
    seed = _mm256_xor_si256(seed, temp);

    temp = _mm256_slli_epi32(seed, 17);
    seed = _mm256_xor_si256(seed, temp);

    const __m256i mantissaMask = _mm256_set1_epi32(0x7FFFFF);
    const __m256i mantissa = _mm256_and_si256(seed, mantissaMask);
    const __m256 one = _mm256_set1_ps(1);
    __m256 val = _mm256_or_ps(_mm256_castsi256_ps(mantissa), one);

    const __m256 rf = _mm256_set1_ps((float)max);
    val = _mm256_sub_ps(_mm256_mul_ps(val, rf), rf);

    return _mm256_cvttps_epi32(val);
}

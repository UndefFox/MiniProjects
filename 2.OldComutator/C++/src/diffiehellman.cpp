#include "diffiehellman.h"


uint64_t DiffieHellman::applyExpression(uint64_t exp, uint64_t base) {
    const uint64_t modulus = 10619863;

    base %= modulus;
    uint64_t result = 1;
    while (exp > 0) {
        if (exp & 1) result = (result * base) % modulus;
        base = (base * base) % modulus;
        exp >>= 1;
    }

    return result;
}

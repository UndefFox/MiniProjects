#include "squarematrix.h"

#include <algorithm>
#include <cassert>
#include <random>
#include <immintrin.h>
#include <cblas.h>



SquareMatrix::SquareMatrix(int size, int seed) :
    m_sideSize(size),
    m_data(size * size)
{
    assert(m_sideSize % TILE_SIDE_SIZE == 0);

    if (seed) {
        std::mt19937 generator(seed);
        std::uniform_real_distribution<storageValue_t> distribution(-1.0f, 1.0f);

        std::generate(m_data.begin(), m_data.end(), [&]() -> float {
            return distribution(generator);
        });
    }
}

void SquareMatrix::set(size_t r, size_t c, storageValue_t value) noexcept {
    *getPointer(r, c) = value;
}

SquareMatrix::storageValue_t SquareMatrix::get(size_t r, size_t c) const noexcept {
    return *getPointerC(r, c);
}

namespace {
inline void multiplyAddPacks(const float* __restrict__ A, const float* __restrict__ B, float* __restrict__ C) noexcept {
    constexpr std::size_t T = SquareMatrix::TILE_SIDE_SIZE;
    constexpr std::size_t W = SquareMatrix::VECTOR_WIDTH;

    for (std::size_t i = 0; i < T; i += 3) {
        for (std::size_t j = 0; j < T; j += 3 * W) {
            __m256 c00 = _mm256_load_ps(C + (i + 0) * T + j + 0 * W);
            __m256 c01 = _mm256_load_ps(C + (i + 0) * T + j + 1 * W);
            __m256 c02 = _mm256_load_ps(C + (i + 0) * T + j + 2 * W);

            __m256 c10 = _mm256_load_ps(C + (i + 1) * T + j + 0 * W);
            __m256 c11 = _mm256_load_ps(C + (i + 1) * T + j + 1 * W);
            __m256 c12 = _mm256_load_ps(C + (i + 1) * T + j + 2 * W);

            __m256 c20 = _mm256_load_ps(C + (i + 2) * T + j + 0 * W);
            __m256 c21 = _mm256_load_ps(C + (i + 2) * T + j + 1 * W);
            __m256 c22 = _mm256_load_ps(C + (i + 2) * T + j + 2 * W);

            for (std::size_t k = 0; k < T; k++) {
                const float* b = B + k * T + j;
                const __m256 b0 = _mm256_load_ps(b + 0 * W);
                const __m256 b1 = _mm256_load_ps(b + 1 * W);
                const __m256 b2 = _mm256_load_ps(b + 2 * W);

                const __m256 a0 = _mm256_broadcast_ss(A + (i + 0) * T + k);
                const __m256 a1 = _mm256_broadcast_ss(A + (i + 1) * T + k);
                const __m256 a2 = _mm256_broadcast_ss(A + (i + 2) * T + k);

                c00 = _mm256_fmadd_ps(a0, b0, c00);
                c01 = _mm256_fmadd_ps(a0, b1, c01);
                c02 = _mm256_fmadd_ps(a0, b2, c02);

                c10 = _mm256_fmadd_ps(a1, b0, c10);
                c11 = _mm256_fmadd_ps(a1, b1, c11);
                c12 = _mm256_fmadd_ps(a1, b2, c12);

                c20 = _mm256_fmadd_ps(a2, b0, c20);
                c21 = _mm256_fmadd_ps(a2, b1, c21);
                c22 = _mm256_fmadd_ps(a2, b2, c22);
            }

            _mm256_store_ps(C + (i + 0) * T + j + 0 * W, c00);
            _mm256_store_ps(C + (i + 0) * T + j + 1 * W, c01);
            _mm256_store_ps(C + (i + 0) * T + j + 2 * W, c02);

            _mm256_store_ps(C + (i + 1) * T + j + 0 * W, c10);
            _mm256_store_ps(C + (i + 1) * T + j + 1 * W, c11);
            _mm256_store_ps(C + (i + 1) * T + j + 2 * W, c12);

            _mm256_store_ps(C + (i + 2) * T + j + 0 * W, c20);
            _mm256_store_ps(C + (i + 2) * T + j + 1 * W, c21);
            _mm256_store_ps(C + (i + 2) * T + j + 2 * W, c22);
        }
    }
}

}

/*
void SquareMatrix::multiplyAdd(const SquareMatrix& lhs, const SquareMatrix& rhs, SquareMatrix& result) noexcept {
    const int n = static_cast<int>(lhs.m_sideSize);

    cblas_sgemm(
        CblasRowMajor,
        CblasNoTrans,
        CblasNoTrans,
        n,
        n,
        n,
        1.0f,
        lhs.m_data.data(),
        n,
        rhs.m_data.data(),
        n,
        1.0f,
        result.m_data.data(),
        n
    );
}
*/

void SquareMatrix::multiplyAdd(const SquareMatrix &lhs, const SquareMatrix &rhs, SquareMatrix &result) noexcept {
    assert(lhs.m_sideSize == rhs.m_sideSize);
    assert(lhs.m_sideSize == result.m_sideSize);
    assert(lhs.m_sideSize % TILE_SIDE_SIZE == 0);
    assert(&lhs != &result);
    assert(&rhs != &result);

    const size_t n = lhs.m_sideSize;

    alignas(64) float leftPack[TILE_SIDE_SIZE * TILE_SIDE_SIZE];
    alignas(64) float rightPack[TILE_SIDE_SIZE * TILE_SIDE_SIZE];
    alignas(64) float resultPack[TILE_SIDE_SIZE * TILE_SIDE_SIZE];

    for (std::size_t rowTileShift = 0; rowTileShift < n; rowTileShift += TILE_SIDE_SIZE) {
        for (std::size_t kTileShift = 0; kTileShift < n; kTileShift += TILE_SIDE_SIZE) {
            for (std::size_t row = 0; row < TILE_SIDE_SIZE; ++row) {
                if (kTileShift + TILE_SIDE_SIZE < n) {
                    _mm_prefetch(lhs.getPointerC(rowTileShift + row, kTileShift + TILE_SIDE_SIZE), _MM_HINT_T2);
                }

                std::copy_n(lhs.getPointerC(rowTileShift + row, kTileShift),
                            TILE_SIDE_SIZE,
                            leftPack + row * TILE_SIDE_SIZE);
            }

            for (std::size_t colTileShift = 0; colTileShift < n; colTileShift += TILE_SIDE_SIZE) {
                for (std::size_t row = 0; row < TILE_SIDE_SIZE; ++row) {
                    if (colTileShift + TILE_SIDE_SIZE < n) {
                        _mm_prefetch(rhs.getPointerC(kTileShift + row, colTileShift + TILE_SIDE_SIZE), _MM_HINT_T1);
                        _mm_prefetch(result.getPointerC(rowTileShift + row, colTileShift + TILE_SIDE_SIZE), _MM_HINT_T1);
                    }

                    std::copy_n(result.getPointerC(rowTileShift + row, colTileShift),
                                TILE_SIDE_SIZE,
                                resultPack + row * TILE_SIDE_SIZE);

                    std::copy_n(rhs.getPointerC(kTileShift + row, colTileShift),
                                TILE_SIDE_SIZE,
                                rightPack + row * TILE_SIDE_SIZE);
                }

                multiplyAddPacks(leftPack, rightPack, resultPack);

                for (std::size_t row = 0; row < TILE_SIDE_SIZE; row++) {
                    std::copy_n(resultPack + row * TILE_SIDE_SIZE,
                                TILE_SIDE_SIZE,
                                result.getPointer(rowTileShift + row, colTileShift));
                }
            }
        }
    }
}

bool SquareMatrix::operator==(const SquareMatrix &rhs) const {
    if (m_sideSize != rhs.m_sideSize) return false;

    for (std::size_t row = 0; row < m_sideSize; row++)
        for (std::size_t column = 0; column < m_sideSize; column++)
            if (this->get(row, column) != rhs.get(row, column)) [[unlikely]] return false;

    return true;
}

SquareMatrix SquareMatrix::operator*(const SquareMatrix& rhs) const noexcept {
    SquareMatrix result(m_sideSize);

    SquareMatrix::multiplyAdd(*this, rhs, result);

    return result;
}

const SquareMatrix::storageValue_t* __restrict__ SquareMatrix::getPointerC(size_t row, size_t col) const noexcept {
    return m_data.data() + row * m_sideSize + col;
}

SquareMatrix::storageValue_t* __restrict__ SquareMatrix::getPointer(size_t row, size_t col) noexcept {
    return m_data.data() + row * m_sideSize + col;
}


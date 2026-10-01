#pragma once

#include <vector>
#include "alignedallocator.hpp"



class SquareMatrix {
public:
    typedef float storageValue_t;

    static constexpr std::size_t TILE_SIDE_SIZE = 120;
    static constexpr std::size_t VECTOR_WIDTH = 8;

    static_assert(TILE_SIDE_SIZE % 8 == 0, "TILE_SIDE_SIZE only supports devisions of 8!");
    static_assert(TILE_SIDE_SIZE % (3 * VECTOR_WIDTH) == 0, "TILE_SIDE_SIZE must be devisable by Kernel size!");


private:
    std::vector<storageValue_t, aligned_allocator<float>> m_data;
    int m_sideSize;


public:
    SquareMatrix(int size, int seed = 0);
    SquareMatrix(const SquareMatrix&) = default;
    SquareMatrix(SquareMatrix&& r) noexcept = default;

    void set(size_t r, size_t c, storageValue_t value) noexcept;
    storageValue_t get(size_t r, size_t c) const noexcept;

    static void multiplyAdd(const SquareMatrix& lhs, const SquareMatrix& rhs, SquareMatrix& store) noexcept;

    SquareMatrix& operator=(const SquareMatrix&) = default;
    SquareMatrix& operator=(SquareMatrix&&) noexcept = default;
    SquareMatrix operator*(const SquareMatrix& rhs) const noexcept;
    bool operator==(const SquareMatrix& rhs) const;

private:
    const storageValue_t* __restrict__ getPointerC(size_t row, size_t col) const noexcept;
    storageValue_t* __restrict__ getPointer(size_t row, size_t col) noexcept;
};

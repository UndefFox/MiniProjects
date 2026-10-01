#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "squarematrix.h"



TEST_CASE("SquareMatrix set and get") {
    SquareMatrix matrix(SquareMatrix::TILE_SIDE_SIZE);

    matrix.set(0, 0, 1.0f);
    matrix.set(0, 1, 2.0f);
    matrix.set(1, 0, 3.0f);
    matrix.set(1, 1, 4.0f);
    matrix.set(SquareMatrix::TILE_SIDE_SIZE - 1, SquareMatrix::TILE_SIDE_SIZE - 1, 5.0f);

    REQUIRE(matrix.get(0, 0) == 1.0f);
    REQUIRE(matrix.get(0, 1) == 2.0f);
    REQUIRE(matrix.get(1, 0) == 3.0f);
    REQUIRE(matrix.get(1, 1) == 4.0f);
    REQUIRE(matrix.get(SquareMatrix::TILE_SIDE_SIZE - 1, SquareMatrix::TILE_SIDE_SIZE - 1) == 5.0f);
}

TEST_CASE("SquareMatrix multiplication correctness") {
    constexpr int size = SquareMatrix::TILE_SIDE_SIZE * 2;

    SquareMatrix a(size, 42);
    SquareMatrix b(size, 99);

    SquareMatrix result = a * b;

    auto naive_multiply = [](const SquareMatrix& lhs, const SquareMatrix& rhs, const int n) {
        SquareMatrix out(n);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                float sum = 0.0f;
                for (int k = 0; k < n; ++k) {
                    sum += lhs.get(i, k) * rhs.get(k, j);
                }
                out.set(i, j, sum);
            }
        }
        return out;
    };

    SquareMatrix expected = naive_multiply(a, b, size);

    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            REQUIRE_THAT(result.get(i, j), Catch::Matchers::WithinAbs(expected.get(i, j), 1e-5f));
        }
    }
}


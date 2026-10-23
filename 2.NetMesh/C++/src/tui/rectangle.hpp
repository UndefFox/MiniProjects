#pragma once

#include <algorithm>
#include <cstdint>



struct Rectangle {
    using coord_t = std::int16_t;
    using dimension_t = std::uint16_t;

    coord_t x;
    coord_t y;
    dimension_t width;
    dimension_t height;

    constexpr Rectangle operator+(const Rectangle rhs) const {
        return {
            .x = static_cast<coord_t>(x + rhs.x),
            .y = static_cast<coord_t>(y + rhs.y),
            .width = static_cast<dimension_t>(width + rhs.width),
            .height = static_cast<dimension_t>(height + rhs.height)
        };
    }

    constexpr Rectangle operator-(const Rectangle rhs) const {
        return {
            .x = static_cast<coord_t>(x - rhs.x),
            .y = static_cast<coord_t>(y - rhs.y),
            .width = static_cast<dimension_t>(width - rhs.width),
            .height = static_cast<dimension_t>(height - rhs.height)
        };
    }

    constexpr Rectangle operator/(const Rectangle rhs) const {
        const int left = std::max(x, rhs.x);
        const int top = std::max(y, rhs.y);
        const int right = std::min(
            static_cast<int>(x) + width,
            static_cast<int>(rhs.x) + rhs.width);
        const int bottom = std::min(
            static_cast<int>(y) + height,
            static_cast<int>(rhs.y) + rhs.height);

        if (left >= right || top >= bottom) {
            return {};
        }

        return {
            .x = static_cast<coord_t>(left),
            .y = static_cast<coord_t>(top),
            .width = static_cast<dimension_t>(right - left),
            .height = static_cast<dimension_t>(bottom - top)
        };
    }

    constexpr Rectangle toPoint() const {
        return {
            .x = x,
            .y = y,
            .width = 0,
            .height = 0
        };
    }
};


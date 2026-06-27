#pragma once

#include <cstdint>
#include <memory>
#include <span>



class Image {
private:
    int width;
    int height;
    std::unique_ptr<uint8_t[]> data;


public:
    Image();
    Image(const std::string& filePath);
    Image(std::span<std::uint8_t> raw, int width, int height);

    Image(const Image& other);
    Image& operator=(const Image& other);
    Image(Image&& other);
    Image& operator=(Image&& other);

    inline std::span<std::uint8_t> getData() { return {data.get(), data.get() + size()}; };
    inline size_t size() const { return (size_t)width * (size_t)height * 4; };
    inline int getWidth() const { return width; };
    inline int getHeight() const { return height; };

    void save(const std::string& filePath);
};

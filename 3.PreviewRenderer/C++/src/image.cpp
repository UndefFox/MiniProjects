#include "image.h"

#include <cstring>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


Image::Image() :
    width(0),
    height(0),
    data(nullptr)
{}

Image::Image(const std::string& filePath) {
    uint8_t* buff = static_cast<uint8_t*>(stbi_load(filePath.data(), &width, &height, nullptr, STBI_rgb_alpha));

    if (buff == nullptr) {
        throw std::runtime_error("Failed to load image");
    }

    data = std::unique_ptr<uint8_t[]>(buff);
}

Image::Image(std::span<std::uint8_t> raw, int width, int height) :
    width(width),
    height(height),
    data(std::make_unique<std::uint8_t[]>(size()))
{
    std::memcpy(data.get(), raw.data(), size());
}

Image::Image(const Image& other) :
    data(std::make_unique<uint8_t[]>(other.size())),
    width(other.width),
    height(other.height)
{
    std::memcpy(data.get(), other.data.get(), other.size());
}

Image& Image::operator=(const Image &other) {
    data = std::make_unique<uint8_t[]>(other.size());
    width = other.width;
    height = other.height;

    std::memcpy(data.get(), other.data.get(), other.size());

    return *this;
}

void Image::save(const std::string& filePath) {
    stbi_write_png(filePath.data(), width, height, STBI_rgb_alpha, data.get(), 0);
}

Image::Image(Image &&other) :
    data(std::move(other.data)),
    width(other.width),
    height(other.height)
{ }

Image& Image::operator=(Image&& other) {
    data = std::move(other.data);
    width = other.width;
    height = other.height;

    return *this;
}



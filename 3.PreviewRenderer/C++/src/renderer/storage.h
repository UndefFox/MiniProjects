#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

#include "device.h"
#include "renderparams.h"



namespace Renderer {

class Storage {
private:
    struct Settings {
        bool drawLines:1 = false;
        bool drawSizes:1 = false;
        bool objectShading:1 = false;
        bool objectTextures:1 = false;
        vk::SampleCountFlagBits sample = vk::SampleCountFlagBits::e1;
        vk::ClearColorValue background;
    };

    struct ArrayBuffer {
        vk::Buffer buffer;
        vk::DeviceMemory memory;
        std::size_t count;
    };

    struct Image {
        vk::Image image;
        vk::DeviceMemory memory;
        vk::ImageView view;

        std::uint32_t width;
        std::uint32_t height;
        vk::Format format;
    };

    struct StagingBuffer{
        Storage& storage;
        vk::Buffer buffer;
        vk::DeviceMemory memory;
        void* map;

        StagingBuffer(Storage& storage, vk::BufferUsageFlags usage, vk::DeviceSize size);
        ~StagingBuffer();
    };


private:
    vk::PhysicalDevice& physicalDevice;
    Device& device;

public:
    Settings settings;

    ArrayBuffer objData;
    ArrayBuffer linesData;
    ArrayBuffer textData;
    ArrayBuffer uboBuffer;

    Image render;
    Image depthBuffer;
    Image objTexture;
    Image textTexture;
    Image outputImage;

    vk::Sampler sampler;


public:
    Storage(const Params& params, vk::PhysicalDevice& physicalDevice, Device& device);
    void destroy();
    std::vector<std::uint8_t> readRenderImage();

private:
    Settings initSettings(const Params &params);

    ArrayBuffer initObjData(const Params &params);
    ArrayBuffer initLinesData(const Params &params);
    ArrayBuffer initTextData(const Params &params);
    ArrayBuffer initUboBuffer(const Params &params);

    Image initRender(const Params &params);
    Image initDepthjBuffer(const Params &params);
    Image initObjTexture(const Params &params);
    Image initTextTexture(const Params &params);
    Image initOutputImage(const Params &params);

    vk::Sampler initSampelr();

    std::uint32_t findMemoryType(std::uint32_t typeFilter,
                                 vk::MemoryPropertyFlags properties) const;
    vk::Format findSupportedFormat(const vk::ArrayProxy<vk::Format> candidates,
                                   vk::ImageTiling tiling,
                                   vk::FormatFeatureFlags features) const;
    bool checkForStencil(const vk::Format format) const;
    ArrayBuffer createArrayBuffer(std::span<const Vertex> data);
    Image creatTextureImage(std::span<const std::uint8_t> data,
                            const std::uint32_t width);
    void transitImageLayout(vk::Image &image, vk::ImageLayout oldLayout,
                            vk::ImageLayout newLayout,
                            vk::AccessFlags srcAccessMask,
                            vk::AccessFlags dstAccessMask,
                            vk::PipelineStageFlags srcStageMask,
                            vk::PipelineStageFlags dstStageMask, vk::ImageAspectFlags aspectFlags);
    void writeImage(vk::Image &image, vk::ImageLayout imageLayout,
                    const std::span<const std::uint8_t> data,
                    const std::uint32_t width);
};

}
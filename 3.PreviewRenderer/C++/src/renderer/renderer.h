#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

#include "device.h"
#include "renderpass.h"
#include "renderparams.h"
#include "storage.h"



namespace Renderer {

class Instance {
public:
    int height;
    int weight;

    float camera_yaw;
    float camera_pitch;

public:
    Params params;
    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    Device device;
    Storage storage;
    RenderPass renderPass;


public:
    Instance(Params params);
    ~Instance();

    void render();
    std::vector<std::uint8_t> renderedImage();

private:
    vk::Instance initInstance();
    vk::PhysicalDevice choosePhysicalDevice();
};

}
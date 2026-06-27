#pragma once

#include <cstdint>
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>


namespace Renderer {

class Device {
private:
    struct QueueDistribution {
        std::array<std::uint32_t, 2> data;

        std::uint32_t& rendering() { return data[0]; };
        std::uint32_t& transfer() { return data[1]; };
    };

    struct Queue {
        vk::Queue queue;
        vk::CommandPool commandPool;
        vk::CommandBuffer buffer;
    };

public:
    vk::PhysicalDeviceFeatures features;
    QueueDistribution queueDistribution;
    vk::Device device;

    Queue render;
    Queue transport;


public:
    Device(const vk::PhysicalDevice& pd);


    void destroy();

    static bool isDeviceSuitable(const vk::PhysicalDevice &pd);

private:
    QueueDistribution initQueuesDistribution(const vk::PhysicalDevice &pd) const;
    vk::Device initLogicalDevice(const vk::PhysicalDevice &pd);
    Queue initRenderQueue();
    Queue initTransportQueue();

    static std::optional<QueueDistribution> getQueueDistribution(const vk::PhysicalDevice &pd);
    Queue getQueue(std::uint32_t familyIndex);
};

}
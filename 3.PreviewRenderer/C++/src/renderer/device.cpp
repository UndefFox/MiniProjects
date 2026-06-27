#include "device.h"

#include <set>



using namespace Renderer;

Device::Device(const vk::PhysicalDevice &pd) :
    features({}),
    queueDistribution(initQueuesDistribution(pd)),
    device(initLogicalDevice(pd)),
    render(initRenderQueue()),
    transport(initTransportQueue())
{ }

void Device::destroy() {
    device.freeCommandBuffers(render.commandPool, render.buffer);
    device.freeCommandBuffers(transport.commandPool, transport.buffer);

    device.destroyCommandPool(render.commandPool);
    device.destroyCommandPool(transport.commandPool);

    device.destroy();
}

bool Device::isDeviceSuitable(const vk::PhysicalDevice &pd) {
    return getQueueDistribution(pd).has_value();
}

Device::QueueDistribution Device::initQueuesDistribution(const vk::PhysicalDevice &pd) const {
    const auto output = getQueueDistribution(pd);

    if (!output.has_value()) {
        throw std::runtime_error("Failed to distribute queues for device!");
    }

    return output.value();
}

vk::Device Device::initLogicalDevice(const vk::PhysicalDevice &pd) {
    constexpr std::array<float, 1> priorities = { 1.0f };

    std::set<std::uint32_t> uniqueQueues;
    uniqueQueues.insert(queueDistribution.data.begin(), queueDistribution.data.end());

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    for (const auto queueIndex : uniqueQueues) {
        queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{
            .queueFamilyIndex = queueIndex,
            .queueCount = priorities.size(),
            .pQueuePriorities = priorities.data()
        });
    }

    vk::PhysicalDeviceFeatures availableFeatures = pd.getFeatures();
    if (availableFeatures.sampleRateShading) features.setSampleRateShading(vk::True);
    if (availableFeatures.wideLines) features.setWideLines(vk::True);

    vk::DeviceCreateInfo deviceCreateInfo{
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .pEnabledFeatures = &features
    };

    return pd.createDevice(deviceCreateInfo);
}

Device::Queue Device::initRenderQueue() {
    return getQueue(queueDistribution.rendering());
}

Device::Queue Device::initTransportQueue() {
    return getQueue(queueDistribution.transfer());
}


std::optional<Device::QueueDistribution> Device::getQueueDistribution(const vk::PhysicalDevice &pd) {
    const auto queueFamilies = pd.getQueueFamilyProperties();

    Device::QueueDistribution output;

    struct Entry{
        bool found = false;
        const vk::QueueFlagBits flags;
        std::uint32_t& store;
    };
    std::array<Entry, 2> queuesToFind{
        Entry{.flags = vk::QueueFlagBits::eGraphics, .store = output.rendering()},
        Entry{.flags = vk::QueueFlagBits::eTransfer, .store = output.transfer()}
    };

    for (std::uint32_t i = 0; i < queueFamilies.size(); i++) {
        for (auto& entry : queuesToFind) {
            if (!entry.found && queueFamilies[i].queueFlags & entry.flags) {
                entry.found = true;
                entry.store = i;
            }
        }
    }

    for (const auto& entry : queuesToFind) {
        if (!entry.found) return {};
    }

    return output;
}

Device::Queue Device::getQueue(uint32_t familyIndex) {
    Queue output;

    output.queue = device.getQueue(familyIndex, 0);

    vk::CommandPoolCreateInfo cmdPoolCreateInfo{
        .flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = familyIndex
    };

    output.commandPool = device.createCommandPool(cmdPoolCreateInfo);

    vk::CommandBufferAllocateInfo allocateBufferInfo{
        .commandPool = output.commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };

    output.buffer = device.allocateCommandBuffers(allocateBufferInfo)[0];

    return output;
}
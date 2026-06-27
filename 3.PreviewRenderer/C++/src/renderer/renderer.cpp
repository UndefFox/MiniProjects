#include "renderer.h"



using namespace Renderer;

Instance::Instance(Params params) :
    params(params),
    instance(initInstance()),
    physicalDevice(choosePhysicalDevice()),
    device(physicalDevice),
    storage(params, physicalDevice, device),
    renderPass(params, device, storage)
{ }

Instance::~Instance() {
    renderPass.destroy();
    storage.destroy();
    device.destroy();
    instance.destroy();
}

vk::Instance Instance::initInstance() {
    vk::ApplicationInfo applicationInfo{
        .pApplicationName   = "Preview Renderer",
        .applicationVersion = vk::makeVersion(0, 0, 1),
        .pEngineName        = "Fox engine",
        .engineVersion      = vk::makeVersion(0, 0, 1),
        .apiVersion         = vk::ApiVersion11
    };

    const std::array<const char*, 0> validationLayers = {
    };

    vk::InstanceCreateInfo instanceCreateInfo{
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = validationLayers.size(),
        .ppEnabledLayerNames = validationLayers.data()
    };

    return vk::createInstance(instanceCreateInfo);
}

vk::PhysicalDevice Instance::choosePhysicalDevice() {
    auto allDevices = instance.enumeratePhysicalDevices();

    for (vk::PhysicalDevice& device : allDevices) {
        if (!Device::isDeviceSuitable(device)) continue;

        return device;
    }

    throw std::runtime_error("Couldn't find sutable device to render!");
}

void Instance::render() {
    renderPass.render();
}

std::vector<uint8_t> Instance::renderedImage() {
    return storage.readRenderImage();
}


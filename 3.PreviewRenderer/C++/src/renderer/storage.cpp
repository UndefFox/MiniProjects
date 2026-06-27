#include "storage.h"



namespace {
const int CHANNEL_COUNT = 4;
}

using namespace Renderer;

Storage::StagingBuffer::StagingBuffer(Storage& storage, vk::BufferUsageFlags usage, vk::DeviceSize size) :
    storage(storage)
{
    vk::BufferCreateInfo bufferInfo{
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive
    };

    buffer = storage.device.device.createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirements = storage.device.device.getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = storage.findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
    };

    memory = storage.device.device.allocateMemory(allocInfo);
    storage.device.device.bindBufferMemory(buffer, memory, 0);

    map = storage.device.device.mapMemory(memory, 0, size);
}

Storage::StagingBuffer::~StagingBuffer() {
    storage.device.device.unmapMemory(memory);
    storage.device.device.destroyBuffer(buffer);
    storage.device.device.freeMemory(memory);
}


Storage::Storage(const Params& params, vk::PhysicalDevice& physicalDevice, Device& device) :
    physicalDevice(physicalDevice),
    device(device),
    settings(initSettings(params)),
    objData(initObjData(params)),
    linesData(initLinesData(params)),
    textData(initTextData(params)),
    uboBuffer(initUboBuffer(params)),
    render(initRender(params)),
    depthBuffer(initDepthjBuffer(params)),
    objTexture(initObjTexture(params)),
    textTexture(initTextTexture(params)),
    outputImage(initOutputImage(params)),
    sampler(initSampelr())
{ }

void Storage::destroy() {
    device.device.destroySampler(sampler);

    device.device.destroyImageView(depthBuffer.view);
    device.device.freeMemory(depthBuffer.memory);
    device.device.destroyImage(depthBuffer.image);
    device.device.destroyImageView(outputImage.view);
    device.device.freeMemory(outputImage.memory);
    device.device.destroyImage(outputImage.image);
    device.device.destroyImageView(render.view);
    device.device.freeMemory(render.memory);
    device.device.destroyImage(render.image);
    device.device.destroyImageView(objTexture.view);
    device.device.freeMemory(objTexture.memory);
    device.device.destroyImage(objTexture.image);
    device.device.destroyImageView(textTexture.view);
    device.device.freeMemory(textTexture.memory);
    device.device.destroyImage(textTexture.image);

    device.device.destroyBuffer(objData.buffer);
    device.device.freeMemory(objData.memory);
    device.device.destroyBuffer(linesData.buffer);
    device.device.freeMemory(linesData.memory);
    device.device.destroyBuffer(textData.buffer);
    device.device.freeMemory(textData.memory);
    device.device.destroyBuffer(uboBuffer.buffer);
    device.device.freeMemory(uboBuffer.memory);
}

Storage::Settings Storage::initSettings(const Params &params) {
    Settings settings{};

    settings.drawLines = params.drawLines && !params.linesVertices.empty();
    settings.drawSizes = params.drawSizes && !params.textVertices.empty() && !params.textTexture.empty();
    settings.objectTextures = params.drawTexture && !params.objectTexture.empty();
    settings.objectShading = params.shading;

    auto availableSamples = physicalDevice.getProperties().limits.sampledImageColorSampleCounts;
    if (availableSamples & vk::SampleCountFlagBits::e8) settings.sample = vk::SampleCountFlagBits::e8;
    else if (availableSamples & vk::SampleCountFlagBits::e4) settings.sample = vk::SampleCountFlagBits::e4;
    else if (availableSamples & vk::SampleCountFlagBits::e2) settings.sample = vk::SampleCountFlagBits::e2;
    else settings.sample = vk::SampleCountFlagBits::e1;

    settings.background = vk::ClearColorValue(params.backroundColor[0], params.backroundColor[1], params.backroundColor[2], params.backroundColor[3]);

    return settings;
}

Storage::ArrayBuffer Renderer::Storage::initObjData(const Params& params) {
    return createArrayBuffer(params.objectVertices);
}

Storage::ArrayBuffer Renderer::Storage::initLinesData(const Params& params) {
    if (settings.drawLines) {
        return createArrayBuffer(params.linesVertices);
    }
    else {
        return {};
    }
}

Storage::ArrayBuffer Renderer::Storage::initTextData(const Params& params) {
    if (settings.drawSizes) {
        return createArrayBuffer(params.textVertices);
    }
    else {
        return {};
    }
}

Storage::ArrayBuffer Storage::initUboBuffer(const Params &params) {
    ArrayBuffer output{.count = 1};

    std::size_t memSize = sizeof(UBOCamera);

    vk::BufferCreateInfo bufferInfo{
        .size = memSize,
        .usage = vk::BufferUsageFlagBits::eUniformBuffer,
        .sharingMode = vk::SharingMode::eExclusive
    };

    output.buffer = device.device.createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirements = device.device.getBufferMemoryRequirements(output.buffer);

    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
    };

    output.memory = device.device.allocateMemory(allocInfo);
    device.device.bindBufferMemory(output.buffer, output.memory, 0);

    UBOCamera uboCamera(params.cameraYaw, params.cameraPitch, params.cameraDistance, params.cameraHeight, params.cameraFOV, (float)params.renderWidth / (float)params.renderHeight);

    void* data = device.device.mapMemory(output.memory, 0, memSize);
    std::memcpy(data, &uboCamera, memSize);
    device.device.unmapMemory(output.memory);

    return output;
}

Storage::Image Storage::initRender(const Params& params) {
    Image output {
        .width = params.renderWidth,
        .height = params.renderHeight,
        .format = vk::Format::eR8G8B8A8Srgb
    };

    vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = output.format,
        .extent = {
            .width = output.width,
            .height = output.height,
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = settings.sample,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc |  vk::ImageUsageFlagBits::eTransferDst,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined
    };

    output.image = device.device.createImage(imageInfo);

    vk::MemoryRequirements memRequirements = device.device.getImageMemoryRequirements(output.image);

    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
    };

    output.memory = device.device.allocateMemory(allocInfo);
    device.device.bindImageMemory(output.image, output.memory, 0);

    vk::ImageViewCreateInfo viewInfo{
        .image = output.image,
        .viewType = vk::ImageViewType::e2D,
        .format = output.format,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    output.view = device.device.createImageView(viewInfo);

    return output;
}

Storage::Image Storage::initDepthjBuffer(const Params &params) {
    Image output {
        .width = params.renderWidth,
        .height = params.renderHeight,
        .format = findSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                                      vk::ImageTiling::eOptimal,
                                      vk::FormatFeatureFlagBits::eDepthStencilAttachment)
    };

    vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = output.format,
        .extent = {
            .width = output.width,
            .height = output.height,
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = settings.sample,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined
    };

    output.image = device.device.createImage(imageInfo);

    vk::MemoryRequirements memRequirements = device.device.getImageMemoryRequirements(output.image);

    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
    };

    output.memory = device.device.allocateMemory(allocInfo);
    device.device.bindImageMemory(output.image, output.memory, 0);

    vk::ImageViewCreateInfo viewInfo{
        .image = output.image,
        .viewType = vk::ImageViewType::e2D,
        .format = output.format,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eDepth | (checkForStencil(output.format) ? vk::ImageAspectFlagBits::eStencil : vk::ImageAspectFlags{}),
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    output.view = device.device.createImageView(viewInfo);

    return output;
}

Storage::Image Storage::initObjTexture(const Params& params) {
    if (settings.objectTextures) {
        return creatTextureImage(params.objectTexture, params.objectTextureWidth);
    }
    else {
        std::vector<std::uint8_t> missingTexture{
            192, 192, 192, 255,  //  light gray
            255, 128, 0, 255,    //  orange
            255, 128, 0, 255,    //  orange
            192, 192, 192, 255   //  light gray
        };

        return creatTextureImage(missingTexture, 2);
    }
}

Storage::Image Storage::initTextTexture(const Params& params) {
    if (settings.drawSizes) {
        return creatTextureImage(params.textTexture, params.textTextureWidth);
    }
    else {
        return {};
    }
}


Storage::Image Storage::initOutputImage(const Params &params) {
    Image output {
        .width = params.renderWidth,
        .height = params.renderHeight
    };

    vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = vk::Format::eR8G8B8A8Srgb,
        .extent = {
            .width = output.width,
            .height = output.height,
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eTransferSrc |  vk::ImageUsageFlagBits::eTransferDst,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined
    };

    output.image = device.device.createImage(imageInfo);

    vk::MemoryRequirements memRequirements = device.device.getImageMemoryRequirements(output.image);

    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
    };

    output.memory = device.device.allocateMemory(allocInfo);
    device.device.bindImageMemory(output.image, output.memory, 0);

    transitImageLayout(output.image,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
        vk::AccessFlags{}, vk::AccessFlags{},
        vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe,
        vk::ImageAspectFlagBits::eColor
    );

    return output;
}


vk::Sampler Storage::initSampelr() {
    vk::SamplerCreateInfo samplerInfo{
        .magFilter = vk::Filter::eNearest,
        .minFilter = vk::Filter::eNearest,
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        .anisotropyEnable = vk::False,
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = vk::BorderColor::eFloatTransparentBlack,
        .unnormalizedCoordinates = vk::False
    };

    return device.device.createSampler(samplerInfo);
}

uint32_t Storage::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const {
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (std::uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    return 0;
}

vk::Format Storage::findSupportedFormat(const vk::ArrayProxy<vk::Format> candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) const {
    for (const auto format : candidates) {
        const vk::FormatProperties props = physicalDevice.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

bool Storage::checkForStencil(const vk::Format format) const {
    switch (format) {
        case vk::Format::eD32SfloatS8Uint:
        case vk::Format::eD24UnormS8Uint:
            return true;
        default:
            return false;
    }
}

Storage::ArrayBuffer Storage::createArrayBuffer(std::span<const Vertex> data) {
    ArrayBuffer output;
    output.count = data.size();

    const size_t sizeInBytes = output.count * sizeof(Vertex);

    vk::BufferCreateInfo bufferInfo{
        .size = sizeInBytes,
        .usage = vk::BufferUsageFlagBits::eVertexBuffer,
        .sharingMode = vk::SharingMode::eExclusive
    };

    output.buffer = device.device.createBuffer(bufferInfo);

    vk::MemoryRequirements memRequirements = device.device.getBufferMemoryRequirements(output.buffer);

    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
    };

    output.memory = device.device.allocateMemory(allocInfo);
    device.device.bindBufferMemory(output.buffer, output.memory, 0);

    void* mappedBuffer = device.device.mapMemory(output.memory, 0, sizeInBytes);
    std::memcpy(mappedBuffer, data.data(), sizeInBytes);
    device.device.unmapMemory(output.memory);

    return output;
}

Storage::Image Storage::creatTextureImage(std::span<const std::uint8_t> data, const std::uint32_t width) {
    Image output{
        .width = width,
        .height = static_cast<uint32_t>(data.size() / CHANNEL_COUNT / width),
        .format = vk::Format::eR8G8B8A8Srgb
    };

    vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = output.format,
        .extent = {
            .width = output.width,
            .height = output.height,
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eLinear,
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined
    };

    output.image = device.device.createImage(imageInfo);

    vk::MemoryRequirements memRequirements = device.device.getImageMemoryRequirements(output.image);

    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
    };

    output.memory = device.device.allocateMemory(allocInfo);
    device.device.bindImageMemory(output.image, output.memory, 0);

    transitImageLayout(output.image,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
        vk::AccessFlags{}, vk::AccessFlagBits::eTransferWrite,
        vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
        vk::ImageAspectFlagBits::eColor
    );

    writeImage(output.image,
        vk::ImageLayout::eTransferDstOptimal,
        data,
        width
    );

    transitImageLayout(output.image,
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
        vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
        vk::ImageAspectFlagBits::eColor
    );

    vk::ImageViewCreateInfo viewInfo{
        .image = output.image,
        .viewType = vk::ImageViewType::e2D,
        .format = output.format,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .levelCount = 1,
            .layerCount = 1
        }
    };

    output.view = device.device.createImageView(viewInfo);

    return output;
}

void Storage::transitImageLayout(
    vk::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
    vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask,
    vk::PipelineStageFlags srcStageMask, vk::PipelineStageFlags dstStageMask,
    vk::ImageAspectFlags aspectFlags)
{
    vk::ImageMemoryBarrier barrier {
        .srcAccessMask = srcAccessMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = image,
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .levelCount = 1,
            .layerCount = 1
        }
    };

    vk::CommandBufferBeginInfo beginInfo {
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };

    device.transport.buffer.begin(beginInfo);

    device.transport.buffer.pipelineBarrier(
        srcStageMask, dstStageMask,
        vk::DependencyFlags{},
        nullptr,
        nullptr,
        barrier
    );

    device.transport.buffer.end();

    vk::SubmitInfo submitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &device.transport.buffer,
    };

    device.transport.queue.submit(submitInfo);
    device.transport.queue.waitIdle();
}

void Storage::writeImage(vk::Image& image, vk::ImageLayout imageLayout, std::span<const std::uint8_t> data, const std::uint32_t width) {
    const std::uint32_t height = data.size() / CHANNEL_COUNT / width;

    const size_t sizeInBytes = sizeof(std::uint8_t) * width * height * CHANNEL_COUNT;

    StagingBuffer stagingBuffer(*this, vk::BufferUsageFlagBits::eTransferSrc, sizeInBytes);

    std::memcpy(stagingBuffer.map, data.data(), sizeInBytes);

    vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };

    device.transport.buffer.begin(beginInfo);

    vk::BufferImageCopy region{
        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .layerCount = 1
        },
        .imageExtent = {width, height, 1}
    };

    device.transport.buffer.copyBufferToImage(
        stagingBuffer.buffer,
        image,
        imageLayout,
        region);

    device.transport.buffer.end();

    vk::SubmitInfo submitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &device.transport.buffer,
    };

    device.transport.queue.submit(submitInfo);
    device.transport.queue.waitIdle();
}

std::vector<uint8_t> Storage::readRenderImage() {
    transitImageLayout(render.image,
        vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eGeneral,
        vk::AccessFlags{}, vk::AccessFlags{},
        vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
        vk::ImageAspectFlagBits::eColor
    );

    transitImageLayout(outputImage.image,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
        vk::AccessFlags{}, vk::AccessFlags{},
        vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe,
        vk::ImageAspectFlagBits::eColor
    );

    std::size_t memSize = sizeof(std::uint8_t) * render.width * render.height * CHANNEL_COUNT;

    StagingBuffer stagingBuffer(*this, vk::BufferUsageFlagBits::eTransferDst, memSize);

    vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };

    device.transport.buffer.begin(beginInfo);

    vk::ImageResolve resolve{
        .srcSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .layerCount = 1,
        },
        .dstSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .layerCount = 1,
        },
        .extent = {
            .width = render.width,
            .height = render.height,
            .depth = 1
        }
    };

    vk::BufferImageCopy region{
        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .layerCount = 1,
        },
        .imageExtent = {
            render.width,
            render.height,
            1
        }
    };

    if (settings.sample == vk::SampleCountFlagBits::e1) {
        device.transport.buffer.copyImageToBuffer(
            render.image,
            vk::ImageLayout::eGeneral,
            stagingBuffer.buffer,
            region
        );
    }
    else {
        device.transport.buffer.resolveImage(
            render.image, vk::ImageLayout::eGeneral,
            outputImage.image, vk::ImageLayout::eGeneral,
            resolve
        );

        device.transport.buffer.copyImageToBuffer(
            outputImage.image,
            vk::ImageLayout::eGeneral,
            stagingBuffer.buffer,
            region
        );
    }

    device.transport.buffer.end();

    vk::SubmitInfo submitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &device.transport.buffer,
    };

    device.transport.queue.submit(submitInfo);
    device.transport.queue.waitIdle();

    std::vector<std::uint8_t> output(memSize);

    std::memcpy(output.data(), stagingBuffer.map, memSize);

    return output;
}

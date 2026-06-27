#include "renderpass.h"

#include "types.h"
#include "parser/binary.hpp"
#include "globals/globals.h"

using namespace Renderer;



void RenderPass::ShaderLoader::process(std::span<std::byte> data) {
    result.resize(data.size() / sizeof(std::uint32_t));
    std::memcpy(result.data(), data.data(), data.size());
}

RenderPass::RenderPass(Params& params, Device& device, Storage& storage) :
    device(device),
    params(params),
    storage(storage),
    renderPass(initRenderPass()),
    uboLayout(initUboLayout()),
    samplerLayout(initSamplerLayout()),
    descriptorPool(initDescriptorPool()),
    uboSet(initUboSet()),
    samplerSets(initSamplerSets()),
    pipelineLayout(initPipelineLayout()),
    objectPipeline(initObjectPipeline()),
    linesPipeline(initLinesPipeline()),
    framebuffer(initFramebuffer())
{
    updateDescriptorSets();
}

void RenderPass::render() {
    vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };

    device.render.buffer.begin(beginInfo);

    const std::array<vk::ClearValue, 2> clearValues{
        vk::ClearValue(storage.settings.background),
        vk::ClearValue(vk::ClearDepthStencilValue{1.0f, 0})
    };

    vk::RenderPassBeginInfo renderPassInfo{
        .renderPass = renderPass,
        .framebuffer = framebuffer,
        .renderArea {
            .offset = {0, 0},
            .extent = {
                .width = storage.render.width,
                .height = storage.render.height
            }
        },
        .clearValueCount = clearValues.size(),
        .pClearValues = clearValues.data()
    };

    device.render.buffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    device.render.buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, {uboSet, samplerSets[1]}, {});

    // Rendering object
    device.render.buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, objectPipeline);

    device.render.buffer.pushConstants<VertexPushConstant>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, VertexPushConstant{.enableShading = storage.settings.objectShading});
    device.render.buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, {samplerSets[0]}, {});

    device.render.buffer.bindVertexBuffers(0, storage.objData.buffer, {0});
    device.render.buffer.draw(storage.objData.count, 1, 0, 0);

    // Rendering lines
    if (storage.settings.drawLines) {
        device.render.buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, linesPipeline);

        device.render.buffer.bindVertexBuffers(0, storage.linesData.buffer, {0});

        device.render.buffer.draw(storage.linesData.count, 1, 0, 0);
    }

    // Rendering text
    if (storage.settings.drawSizes) {
        device.render.buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, objectPipeline);

        device.render.buffer.pushConstants<VertexPushConstant>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, VertexPushConstant{.enableShading = false});

        device.render.buffer.bindVertexBuffers(0, storage.textData.buffer, {0});
        device.render.buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, {samplerSets[1]}, {});

        device.render.buffer.draw(storage.textData.count, 1, 0, 0);
    }

    // Finsih rednder
    device.render.buffer.endRenderPass();
    device.render.buffer.end();

    vk::SubmitInfo submitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &device.render.buffer
    };

    device.render.queue.submit(submitInfo);
    device.render.queue.waitIdle();
}

void RenderPass::destroy() {
    device.device.destroyFramebuffer(framebuffer);
    device.device.destroyPipeline(objectPipeline);
    device.device.destroyPipeline(linesPipeline);
    device.device.destroyPipelineLayout(pipelineLayout);
    device.device.destroyDescriptorPool(descriptorPool);
    device.device.destroyDescriptorSetLayout(uboLayout);
    device.device.destroyDescriptorSetLayout(samplerLayout);
    device.device.destroyRenderPass(renderPass);
}

vk::RenderPass RenderPass::initRenderPass() {
    const std::array<vk::AttachmentDescription, 2> attachments{
        vk::AttachmentDescription{
            .format = storage.render.format,
            .samples = storage.settings.sample,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            .initialLayout = vk::ImageLayout::eUndefined,
            .finalLayout = vk::ImageLayout::eColorAttachmentOptimal
        },
        vk::AttachmentDescription{
            .format = storage.depthBuffer.format,
            .samples = storage.settings.sample,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eDontCare,
            .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            .initialLayout = vk::ImageLayout::eUndefined,
            .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal
        }
    };

    const std::array<vk::AttachmentReference, 2> attachmentsRefs{
        vk::AttachmentReference{
            .attachment = 0,
            .layout = vk::ImageLayout::eColorAttachmentOptimal
        },
        vk::AttachmentReference{
            .attachment = 1,
            .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
        }
    };

    vk::SubpassDescription subpass{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        .pColorAttachments = &(attachmentsRefs[0]),
        .pDepthStencilAttachment = &(attachmentsRefs[1])
    };

    const std::array<vk::SubpassDependency, 1> dependencies{
        vk::SubpassDependency{
            .srcSubpass = vk::SubpassExternal,
            .dstSubpass = 0,
            .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests,
            .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
            .srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite
        }
    };

    vk::RenderPassCreateInfo renderPassCreateInfo{
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = dependencies.size(),
        .pDependencies = dependencies.data()
    };

    return device.device.createRenderPass(renderPassCreateInfo);
}

vk::DescriptorSetLayout RenderPass::initUboLayout() {
    const std::array<vk::DescriptorSetLayoutBinding, 1> bindings{
        vk::DescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
            .pImmutableSamplers = nullptr
        }
    };

    vk::DescriptorSetLayoutCreateInfo layoutCreateInfo{
        .bindingCount = bindings.size(),
        .pBindings = bindings.data()
    };

    return device.device.createDescriptorSetLayout(layoutCreateInfo);
}

vk::DescriptorSetLayout RenderPass::initSamplerLayout() {
    const std::array<vk::DescriptorSetLayoutBinding, 1> bindings{
        vk::DescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
            .pImmutableSamplers = nullptr
        }
    };

    vk::DescriptorSetLayoutCreateInfo layoutCreateInfo{
        .bindingCount = bindings.size(),
        .pBindings = bindings.data()
    };

    return device.device.createDescriptorSetLayout(layoutCreateInfo);
}

vk::DescriptorPool RenderPass::initDescriptorPool() {
    const std::array<vk::DescriptorPoolSize, 2> poolSizes{
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1
        }, {
            .type = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 2
        }
    };

    vk::DescriptorPoolCreateInfo descriptorPoolInfo{
        .maxSets = 3,
        .poolSizeCount = poolSizes.size(),
        .pPoolSizes = poolSizes.data()
    };

    return device.device.createDescriptorPool(descriptorPoolInfo);
}

vk::DescriptorSet RenderPass::initUboSet() {
    vk::DescriptorSetAllocateInfo descriptorAllocInfo{
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &uboLayout
    };

    return device.device.allocateDescriptorSets(descriptorAllocInfo)[0];
}

std::array<vk::DescriptorSet, 2> RenderPass::initSamplerSets() {
    const std::array<vk::DescriptorSetLayout, 2> layouts{samplerLayout, samplerLayout};

    vk::DescriptorSetAllocateInfo descriptorAllocInfo{
        .descriptorPool = descriptorPool,
        .descriptorSetCount = layouts.size(),
        .pSetLayouts = layouts.data(),
    };

    const auto buff = device.device.allocateDescriptorSets(descriptorAllocInfo);

    return {buff[0], buff[1]};
}

vk::PipelineLayout RenderPass::initPipelineLayout() {
    const std::array<vk::PushConstantRange, 1> ranges{
        vk::PushConstantRange{
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
            .offset = 0,
            .size = sizeof(VertexPushConstant)
        }
    };

    const std::array<vk::DescriptorSetLayout, 2> setsLayouts{
        uboLayout,
        samplerLayout
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = setsLayouts.size(),
        .pSetLayouts = setsLayouts.data(),
        .pushConstantRangeCount = ranges.size(),
        .pPushConstantRanges = ranges.data()
    };

    return device.device.createPipelineLayout(pipelineLayoutInfo);
}

vk::Pipeline RenderPass::initObjectPipeline() {
    return createPipeline(rootDirectory.path + "/data/shaders/triangle", RenderType::Object);
}

vk::Pipeline RenderPass::initLinesPipeline() {
    return createPipeline(rootDirectory.path + "/data/shaders/line", RenderType::Line);
}

vk::Framebuffer RenderPass::initFramebuffer() {
    std::array<vk::ImageView, 2> freameBufferAttachments = {
        storage.render.view,
        storage.depthBuffer.view
    };

    vk::FramebufferCreateInfo framebufferInfo{
        .renderPass = renderPass,
        .attachmentCount = freameBufferAttachments.size(),
        .pAttachments = freameBufferAttachments.data(),
        .width = storage.render.width,
        .height = storage.render.height,
        .layers = 1
    };

    return device.device.createFramebuffer(framebufferInfo);
}

vk::ShaderModule RenderPass::loadModule(const std::string &filePath) {
    std::vector<std::uint32_t> code = Parser::Binary::read<ShaderLoader>(filePath);

    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = code.size() * sizeof(std::uint32_t),
        .pCode = code.data()
    };

    return device.device.createShaderModule(createInfo);
}


vk::Pipeline RenderPass::createPipeline(const std::string &shaderName, RenderType renderType) {
    vk::ShaderModule vertModule = loadModule(shaderName + ".vert.spv");
    vk::ShaderModule fragModule = loadModule(shaderName + ".frag.spv");

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = vertModule,
        .pName = "main"
    };

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = fragModule,
        .pName = "main"
    };

    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
        vertShaderStageInfo,
        fragShaderStageInfo
    };

    vk::VertexInputBindingDescription bindingDescription{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = vk::VertexInputRate::eVertex
    };

    std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions{
        vk::VertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(Vertex, pos)
        }, {
            .location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(Vertex, normal)
        }, {
            .location = 2,
            .binding = 0,
            .format = vk::Format::eR32G32Sfloat,
            .offset = offsetof(Vertex, texCoord)
        }
    };

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = renderType == RenderType::Object ? vk::PrimitiveTopology::eTriangleList : vk::PrimitiveTopology::eLineList,
        .primitiveRestartEnable = vk::False
    };

    vk::Viewport viewport{
        .x = 0,
        .y = 0,
        .width = static_cast<float>(storage.render.width),
        .height = static_cast<float>(storage.render.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    vk::Rect2D scissors{
        .offset = {.x = 0, .y = 0},
        .extent = {.width = storage.render.width, .height = storage.render.height}
    };

    vk::PipelineViewportStateCreateInfo viewportState{
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissors
    };

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .lineWidth = (device.features.wideLines ? 3.0f : 1.0f),
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = storage.settings.sample,
        .sampleShadingEnable = device.features.sampleRateShading,
        .minSampleShading = 1.0f,
        .alphaToCoverageEnable = vk::True
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eOne,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp = vk::BlendOp::eAdd,

        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,

        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}
    };

    vk::PipelineDepthStencilStateCreateInfo depthStencil{
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::True,
        .depthCompareOp = vk::CompareOp::eLess
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .stageCount = shaderStages.size(),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .layout = pipelineLayout,
        .renderPass = renderPass
    };

    const auto buff = device.device.createGraphicsPipelines(vk::PipelineCache(), pipelineInfo).value[0];

    device.device.destroyShaderModule(vertModule);
    device.device.destroyShaderModule(fragModule);

    return buff;
}

void RenderPass::updateDescriptorSets() {
    vk::DescriptorBufferInfo uboInfo{
        .buffer = storage.uboBuffer.buffer,
        .offset = 0,
        .range = sizeof(UBOCamera)
    };

    vk::DescriptorImageInfo objTextureInfo{
        .sampler = storage.sampler,
        .imageView = storage.objTexture.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };

    vk::WriteDescriptorSet uboWriteSet{
        .dstSet = uboSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &uboInfo
    };

    vk::WriteDescriptorSet objectWriteSet{
        .dstSet = samplerSets[0],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &objTextureInfo
    };

    device.device.updateDescriptorSets({uboWriteSet, objectWriteSet}, 0);



    if (storage.settings.drawSizes) {
        vk::DescriptorImageInfo textTextureInfo{
            .sampler = storage.sampler,
            .imageView = storage.textTexture.view,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        vk::WriteDescriptorSet writeSet {
            .dstSet = samplerSets[1],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &textTextureInfo
        };

        device.device.updateDescriptorSets(writeSet, 0);
    }
}

#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>

#include "device.h"
#include "renderparams.h"
#include "storage.h"



namespace Renderer {

enum RenderPassType : char {
    Object,
    Line
};

class RenderPass {
private:
    struct ShaderLoader {
        void process(std::span<std::byte> data);

        std::vector<std::uint32_t> result;
    };

private:
    Params& params;
    Device& device;
    Storage& storage;

    vk::RenderPass renderPass;
    vk::DescriptorSetLayout uboLayout;
    vk::DescriptorSetLayout samplerLayout;
    vk::DescriptorPool descriptorPool;
    vk::DescriptorSet uboSet;
    std::array<vk::DescriptorSet, 2> samplerSets;

    vk::PipelineLayout pipelineLayout;
    vk::Pipeline objectPipeline;
    vk::Pipeline linesPipeline;

    vk::Framebuffer framebuffer;


public:
    RenderPass(Params& params, Device& device, Storage& storage);

    void render();
    void destroy();

private:
    vk::RenderPass initRenderPass();
    vk::DescriptorSetLayout initUboLayout();
    vk::DescriptorSetLayout initSamplerLayout();
    vk::DescriptorPool initDescriptorPool();
    vk::DescriptorSet initUboSet();
    std::array<vk::DescriptorSet, 2> initSamplerSets();
    vk::PipelineLayout initPipelineLayout();
    vk::Pipeline initObjectPipeline();
    vk::Pipeline initLinesPipeline();
    vk::Framebuffer initFramebuffer();

    vk::ShaderModule loadModule(const std::string& filePath);
    enum class RenderType {
        Object,
        Line
    };
    vk::Pipeline createPipeline(const std::string &shaderName, RenderType renderType);

    void updateDescriptorSets();
};

}
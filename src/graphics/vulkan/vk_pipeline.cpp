#include "graphics/vulkan/vk_pipeline.hpp"
#include <array>
#include <utility>
#include <vector>

namespace northern::graphics {

namespace {

VkShaderModule createShaderModule(VkDevice device, std::span<const uint32_t> spirv) {
    if (spirv.empty()) {
        return VK_NULL_HANDLE;
    }
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode = spirv.data();
    VkShaderModule module{VK_NULL_HANDLE};
    if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return module;
}

} // namespace

VulkanPipeline::VulkanPipeline(VkDevice device, const PipelineDesc& desc) noexcept
    : device_(device), desc_(desc) {}

VulkanPipeline::~VulkanPipeline() {
    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device_, pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }
    if (layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device_, layout_, nullptr);
        layout_ = VK_NULL_HANDLE;
    }
    if (descriptorSetLayout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
        descriptorSetLayout_ = VK_NULL_HANDLE;
    }
}

VulkanPipeline::VulkanPipeline(VulkanPipeline&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)), desc_(other.desc_),
      pipeline_(std::exchange(other.pipeline_, VK_NULL_HANDLE)),
      layout_(std::exchange(other.layout_, VK_NULL_HANDLE)),
      descriptorSetLayout_(std::exchange(other.descriptorSetLayout_, VK_NULL_HANDLE)) {}

VulkanPipeline& VulkanPipeline::operator=(VulkanPipeline&& other) noexcept {
    if (this != &other) {
        if (pipeline_ != VK_NULL_HANDLE) {
            vkDestroyPipeline(device_, pipeline_, nullptr);
        }
        if (layout_ != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device_, layout_, nullptr);
        }
        if (descriptorSetLayout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
        }
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        desc_ = other.desc_;
        pipeline_ = std::exchange(other.pipeline_, VK_NULL_HANDLE);
        layout_ = std::exchange(other.layout_, VK_NULL_HANDLE);
        descriptorSetLayout_ = std::exchange(other.descriptorSetLayout_, VK_NULL_HANDLE);
    }
    return *this;
}

core::Result<std::unique_ptr<VulkanPipeline>, core::Error>
VulkanPipeline::create(VkDevice device, const PipelineDesc& desc, VkPipelineCache pipelineCache) {
    auto pipeline = std::unique_ptr<VulkanPipeline>(new VulkanPipeline(device, desc));

    VkShaderModule vertShaderModule = createShaderModule(device, desc.vertexShader.spirv);
    if (vertShaderModule == VK_NULL_HANDLE) {
        return core::Error(core::ErrorCode::GraphicsError, "Failed to create vertex shader module");
    }

    VkShaderModule fragShaderModule = createShaderModule(device, desc.fragmentShader.spirv);
    if (fragShaderModule == VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        return core::Error(core::ErrorCode::GraphicsError,
                           "Failed to create fragment shader module");
    }

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    shaderStages[0].pName =
        desc.vertexShader.entryPoint.empty() ? "main" : desc.vertexShader.entryPoint.data();

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShaderModule;
    shaderStages[1].pName =
        desc.fragmentShader.entryPoint.empty() ? "main" : desc.fragmentShader.entryPoint.data();

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = desc.pushConstantSize;

    if (desc.hasTexture) {
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 0;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo descLayoutInfo{};
        descLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descLayoutInfo.bindingCount = 1;
        descLayoutInfo.pBindings = &samplerLayoutBinding;

        VkResult dRes = vkCreateDescriptorSetLayout(device, &descLayoutInfo, nullptr,
                                                    &pipeline->descriptorSetLayout_);
        if (dRes != VK_SUCCESS) {
            vkDestroyShaderModule(device, vertShaderModule, nullptr);
            vkDestroyShaderModule(device, fragShaderModule, nullptr);
            return makeVkError(dRes, "Failed to create VkDescriptorSetLayout");
        }
    }

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (desc.hasTexture) {
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &pipeline->descriptorSetLayout_;
    }
    if (desc.pushConstantSize > 0) {
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushConstantRange;
    }

    VkResult res = vkCreatePipelineLayout(device, &layoutInfo, nullptr, &pipeline->layout_);
    if (res != VK_SUCCESS) {
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        return makeVkError(res, "Failed to create VkPipelineLayout");
    }

    std::vector<VkVertexInputBindingDescription> bindings;
    bindings.reserve(desc.vertexLayout.bindings.size());
    for (const auto& b : desc.vertexLayout.bindings) {
        VkVertexInputBindingDescription vkBinding{};
        vkBinding.binding = b.binding;
        vkBinding.stride = b.stride;
        vkBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindings.push_back(vkBinding);
    }

    std::vector<VkVertexInputAttributeDescription> attributes;
    attributes.reserve(desc.vertexLayout.attributes.size());
    for (const auto& a : desc.vertexLayout.attributes) {
        VkVertexInputAttributeDescription vkAttr{};
        vkAttr.location = a.location;
        vkAttr.binding = a.binding;
        vkAttr.format = toVkFormat(a.format);
        vkAttr.offset = a.offset;
        attributes.push_back(vkAttr);
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vertexInputInfo.pVertexBindingDescriptions = bindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = toVkTopology(desc.topology);
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                   VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        static_cast<VkColorComponentFlags>(desc.blendState.writeMask);
    colorBlendAttachment.blendEnable = desc.blendState.blendEnable ? VK_TRUE : VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = toVkBlendFactor(desc.blendState.srcColorFactor);
    colorBlendAttachment.dstColorBlendFactor = toVkBlendFactor(desc.blendState.dstColorFactor);
    colorBlendAttachment.colorBlendOp = toVkBlendOp(desc.blendState.colorBlendOp);
    colorBlendAttachment.srcAlphaBlendFactor = toVkBlendFactor(desc.blendState.srcAlphaFactor);
    colorBlendAttachment.dstAlphaBlendFactor = toVkBlendFactor(desc.blendState.dstAlphaFactor);
    colorBlendAttachment.alphaBlendOp = toVkBlendOp(desc.blendState.alphaBlendOp);

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkFormat colorFormat = toVkFormat(desc.colorAttachmentFormat);
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &colorFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipeline->layout_;
    pipelineInfo.renderPass = VK_NULL_HANDLE;

    res = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr,
                                    &pipeline->pipeline_);

    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);

    if (res != VK_SUCCESS) {
        return makeVkError(res, "Failed to create graphics pipeline");
    }

    return pipeline;
}

} // namespace northern::graphics

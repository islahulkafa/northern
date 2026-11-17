#pragma once

#include "core/error/error.hpp"
#include "core/error/result.hpp"
#include "graphics/pipeline.hpp"
#include "graphics/vulkan/vk_common.hpp"
#include <memory>

namespace northern::graphics {

class VulkanPipeline {
public:
    static core::Result<std::unique_ptr<VulkanPipeline>, core::Error>
    create(VkDevice device, const PipelineDesc& desc);

    ~VulkanPipeline();

    VulkanPipeline(const VulkanPipeline&) = delete;
    VulkanPipeline& operator=(const VulkanPipeline&) = delete;

    VulkanPipeline(VulkanPipeline&&) noexcept;
    VulkanPipeline& operator=(VulkanPipeline&&) noexcept;

    [[nodiscard]] VkPipeline handle() const noexcept { return pipeline_; }
    [[nodiscard]] VkPipelineLayout layout() const noexcept { return layout_; }
    [[nodiscard]] VkDescriptorSetLayout descriptorSetLayout() const noexcept {
        return descriptorSetLayout_;
    }
    [[nodiscard]] const PipelineDesc& desc() const noexcept { return desc_; }

private:
    explicit VulkanPipeline(VkDevice device, const PipelineDesc& desc) noexcept;

    VkDevice device_{VK_NULL_HANDLE};
    PipelineDesc desc_{};
    VkPipeline pipeline_{VK_NULL_HANDLE};
    VkPipelineLayout layout_{VK_NULL_HANDLE};
    VkDescriptorSetLayout descriptorSetLayout_{VK_NULL_HANDLE};
};

} // namespace northern::graphics

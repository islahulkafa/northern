#include "graphics/vulkan/vk_command_buffer.hpp"
#include "graphics/vulkan/vk_device.hpp"
#include <algorithm>
#include <utility>

namespace northern::graphics {

VulkanCommandBuffer::VulkanCommandBuffer(VkDevice device, VkCommandPool pool,
                                         VulkanDevice* deviceImpl) noexcept
    : device_(device), pool_(pool), deviceImpl_(deviceImpl) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = pool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    vkAllocateCommandBuffers(device_, &allocInfo, &cmd_);
}

VulkanCommandBuffer::~VulkanCommandBuffer() {
    if (cmd_ != VK_NULL_HANDLE && pool_ != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(device_, pool_, 1, &cmd_);
        cmd_ = VK_NULL_HANDLE;
    }
}

VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      pool_(std::exchange(other.pool_, VK_NULL_HANDLE)),
      deviceImpl_(std::exchange(other.deviceImpl_, nullptr)),
      cmd_(std::exchange(other.cmd_, VK_NULL_HANDLE)),
      currentRenderTarget_(other.currentRenderTarget_) {}

VulkanCommandBuffer& VulkanCommandBuffer::operator=(VulkanCommandBuffer&& other) noexcept {
    if (this != &other) {
        if (cmd_ != VK_NULL_HANDLE && pool_ != VK_NULL_HANDLE) {
            vkFreeCommandBuffers(device_, pool_, 1, &cmd_);
        }
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        pool_ = std::exchange(other.pool_, VK_NULL_HANDLE);
        deviceImpl_ = std::exchange(other.deviceImpl_, nullptr);
        cmd_ = std::exchange(other.cmd_, VK_NULL_HANDLE);
        currentRenderTarget_ = other.currentRenderTarget_;
    }
    return *this;
}

void VulkanCommandBuffer::begin() {
    vkResetCommandBuffer(cmd_, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd_, &beginInfo);
}

void VulkanCommandBuffer::end() {
    vkEndCommandBuffer(cmd_);
}

void VulkanCommandBuffer::beginRendering(const RenderingDesc& desc) {
    currentRenderTarget_ = desc.colorAttachment.target;
    VulkanTexture* targetTex = deviceImpl_->getTexture(desc.colorAttachment.target);
    if (!targetTex) {
        return;
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = targetTex->image();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(cmd_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr,
                         1, &barrier);

    VkRenderingAttachmentInfo attachInfo{};
    attachInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachInfo.imageView = targetTex->imageView();
    attachInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachInfo.loadOp =
        desc.colorAttachment.clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachInfo.clearValue.color.float32[0] = desc.colorAttachment.clearColor.r;
    attachInfo.clearValue.color.float32[1] = desc.colorAttachment.clearColor.g;
    attachInfo.clearValue.color.float32[2] = desc.colorAttachment.clearColor.b;
    attachInfo.clearValue.color.float32[3] = desc.colorAttachment.clearColor.a;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {desc.renderArea.origin.x, desc.renderArea.origin.y};
    renderingInfo.renderArea.extent = {static_cast<uint32_t>(desc.renderArea.size.width),
                                       static_cast<uint32_t>(desc.renderArea.size.height)};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &attachInfo;

    vkCmdBeginRendering(cmd_, &renderingInfo);
}

void VulkanCommandBuffer::endRendering() {
    vkCmdEndRendering(cmd_);

    VulkanTexture* targetTex = deviceImpl_->getTexture(currentRenderTarget_);
    if (targetTex) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = targetTex->image();
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(cmd_, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &barrier);
    }
    currentRenderTarget_ = TextureHandle::invalid();
}

void VulkanCommandBuffer::setViewport(const core::Rect& rect) {
    VkViewport vp{};
    vp.x = rect.origin.x;
    vp.y = rect.origin.y;
    vp.width = rect.size.width;
    vp.height = rect.size.height;
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    vkCmdSetViewport(cmd_, 0, 1, &vp);
}

void VulkanCommandBuffer::setScissor(const core::RectI& rect) {
    VkRect2D sc{};
    sc.offset = {rect.origin.x, rect.origin.y};
    sc.extent = {static_cast<uint32_t>(std::max(0, rect.size.width)),
                 static_cast<uint32_t>(std::max(0, rect.size.height))};
    vkCmdSetScissor(cmd_, 0, 1, &sc);
}

void VulkanCommandBuffer::bindPipeline(PipelineHandle pipeline) {
    auto* p = deviceImpl_->getPipeline(pipeline);
    if (p) {
        vkCmdBindPipeline(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, p->handle());
        currentPipeline_ = pipeline;
        currentPipelineLayout_ = p->layout();
    }
}

void VulkanCommandBuffer::bindTexture(uint32_t set, uint32_t binding, TextureHandle texture) {
    (void)binding;
    if (!currentPipeline_.isValid()) {
        return;
    }
    VkDescriptorSet descSet =
        deviceImpl_->getOrCreateTextureDescriptorSet(currentPipeline_, texture);
    if (descSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(cmd_, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipelineLayout_, set,
                                1, &descSet, 0, nullptr);
    }
}

void VulkanCommandBuffer::bindVertexBuffer(uint32_t binding, BufferHandle buffer, uint64_t offset) {
    auto* b = deviceImpl_->getBuffer(buffer);
    if (b) {
        VkBuffer raw = b->handle();
        VkDeviceSize off = offset;
        vkCmdBindVertexBuffers(cmd_, binding, 1, &raw, &off);
    }
}

void VulkanCommandBuffer::bindIndexBuffer(BufferHandle buffer, uint64_t offset,
                                          IndexType indexType) {
    auto* b = deviceImpl_->getBuffer(buffer);
    if (b) {
        vkCmdBindIndexBuffer(cmd_, b->handle(), offset,
                             indexType == IndexType::Uint16 ? VK_INDEX_TYPE_UINT16
                                                            : VK_INDEX_TYPE_UINT32);
    }
}

void VulkanCommandBuffer::pushConstants(PipelineHandle pipeline, ShaderStage stage, uint32_t offset,
                                        uint32_t size, const void* data) {
    auto* p = deviceImpl_->getPipeline(pipeline);
    if (p) {
        VkShaderStageFlags flags = 0;
        if (hasFlag(stage, ShaderStage::Vertex)) {
            flags |= VK_SHADER_STAGE_VERTEX_BIT;
        }
        if (hasFlag(stage, ShaderStage::Fragment)) {
            flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        if (hasFlag(stage, ShaderStage::Compute)) {
            flags |= VK_SHADER_STAGE_COMPUTE_BIT;
        }
        vkCmdPushConstants(cmd_, p->layout(), flags, offset, size, data);
    }
}

void VulkanCommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                               uint32_t firstInstance) {
    vkCmdDraw(cmd_, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                      uint32_t firstIndex, int32_t vertexOffset,
                                      uint32_t firstInstance) {
    vkCmdDrawIndexed(cmd_, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

} // namespace northern::graphics

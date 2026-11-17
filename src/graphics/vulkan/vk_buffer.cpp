#include "graphics/vulkan/vk_buffer.hpp"
#include <cstring>
#include <utility>

namespace northern::graphics {

VulkanBuffer::VulkanBuffer(VkDevice device, VulkanMemoryAllocator* allocator,
                           const BufferDesc& desc) noexcept
    : device_(device), allocator_(allocator), desc_(desc) {}

VulkanBuffer::~VulkanBuffer() {
    if (buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device_, buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
    }
    if (allocator_) {
        allocator_->free(allocation_);
        allocation_ = {};
    }
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
    : device_(std::exchange(other.device_, VK_NULL_HANDLE)),
      allocator_(std::exchange(other.allocator_, nullptr)), desc_(other.desc_),
      buffer_(std::exchange(other.buffer_, VK_NULL_HANDLE)),
      allocation_(std::exchange(other.allocation_, {})) {}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept {
    if (this != &other) {
        if (buffer_ != VK_NULL_HANDLE) {
            vkDestroyBuffer(device_, buffer_, nullptr);
        }
        if (allocator_) {
            allocator_->free(allocation_);
        }
        device_ = std::exchange(other.device_, VK_NULL_HANDLE);
        allocator_ = std::exchange(other.allocator_, nullptr);
        desc_ = other.desc_;
        buffer_ = std::exchange(other.buffer_, VK_NULL_HANDLE);
        allocation_ = std::exchange(other.allocation_, {});
    }
    return *this;
}

core::Result<std::unique_ptr<VulkanBuffer>, core::Error>
VulkanBuffer::create(VkDevice device, VulkanMemoryAllocator& allocator, const BufferDesc& desc,
                     const void* initialData) {
    if (desc.size == 0) {
        return core::Error(core::ErrorCode::InvalidArgument, "Buffer size must be greater than 0");
    }

    auto buffer = std::unique_ptr<VulkanBuffer>(new VulkanBuffer(device, &allocator, desc));

    VkBufferUsageFlags vkUsage = 0;
    if (hasFlag(desc.usage, BufferUsage::Vertex)) {
        vkUsage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (hasFlag(desc.usage, BufferUsage::Index)) {
        vkUsage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (hasFlag(desc.usage, BufferUsage::Uniform)) {
        vkUsage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if (hasFlag(desc.usage, BufferUsage::Storage)) {
        vkUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (hasFlag(desc.usage, BufferUsage::TransferSrc)) {
        vkUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (hasFlag(desc.usage, BufferUsage::TransferDst) || initialData != nullptr) {
        vkUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = desc.size;
    createInfo.usage = vkUsage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult res = vkCreateBuffer(device, &createInfo, nullptr, &buffer->buffer_);
    if (res != VK_SUCCESS) {
        return makeVkError(res, "Failed to create VkBuffer");
    }

    auto allocRes = allocator.allocateBufferMemory(buffer->buffer_, desc.memoryUsage);
    if (allocRes.isErr()) {
        return allocRes.error();
    }
    buffer->allocation_ = allocRes.value();

    if (initialData != nullptr && buffer->allocation_.mappedData != nullptr) {
        std::memcpy(buffer->allocation_.mappedData, initialData, static_cast<size_t>(desc.size));
    }

    return buffer;
}

core::Result<void, core::Error> VulkanBuffer::write(uint64_t offset, uint64_t size,
                                                    const void* data) {
    if (!data) {
        return core::Error(core::ErrorCode::InvalidArgument, "Write data cannot be null");
    }
    if (offset + size > desc_.size) {
        return core::Error(core::ErrorCode::InvalidArgument, "Buffer write out of bounds");
    }
    if (allocation_.mappedData == nullptr) {
        return core::Error(core::ErrorCode::InvalidState,
                           "Cannot directly write to unmapped GPU buffer");
    }

    std::memcpy(static_cast<char*>(allocation_.mappedData) + offset, data,
                static_cast<size_t>(size));
    return {};
}

} // namespace northern::graphics

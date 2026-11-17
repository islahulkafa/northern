#include "graphics/vulkan/vk_device.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <set>
#include <utility>

namespace northern::graphics {

namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
              const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
    (void)type;
    (void)userData;
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "[Vulkan Validation] " << callbackData->pMessage << std::endl;
    }
    return VK_FALSE;
}

} // namespace

VulkanDevice::VulkanDevice(const DeviceDesc& desc) noexcept : desc_(desc) {}

VulkanDevice::~VulkanDevice() {
    waitIdle();

    retiredResources_.clear();
    bufferPool_ = {};
    texturePool_ = {};
    pipelinePool_ = {};

    if (timelineSemaphore_ != VK_NULL_HANDLE) {
        vkDestroySemaphore(device_, timelineSemaphore_, nullptr);
        timelineSemaphore_ = VK_NULL_HANDLE;
    }

    if (commandPool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, commandPool_, nullptr);
        commandPool_ = VK_NULL_HANDLE;
    }

    if (descriptorPool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
        descriptorPool_ = VK_NULL_HANDLE;
    }

    allocator_.reset();

    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }

    if (debugMessenger_ != VK_NULL_HANDLE) {
        auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT"));
        if (func) {
            func(instance_, debugMessenger_, nullptr);
        }
        debugMessenger_ = VK_NULL_HANDLE;
    }

    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

core::Result<std::unique_ptr<VulkanDevice>, core::Error>
VulkanDevice::create(const DeviceDesc& desc) {
    auto device = std::unique_ptr<VulkanDevice>(new VulkanDevice(desc));

    auto res = device->initInstance();
    if (res.isErr()) {
        return res.error();
    }

    res = device->selectPhysicalDevice();
    if (res.isErr()) {
        return res.error();
    }

    res = device->createLogicalDevice();
    if (res.isErr()) {
        return res.error();
    }

    res = device->createCommandPool();
    if (res.isErr()) {
        return res.error();
    }

    res = device->createDescriptorPool();
    if (res.isErr()) {
        return res.error();
    }

    res = device->createTimelineSemaphore();
    if (res.isErr()) {
        return res.error();
    }

    return device;
}

core::Result<void, core::Error> VulkanDevice::initInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = desc_.appName.data();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Northern";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    std::vector<const char*> extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
    };

    std::vector<const char*> layers;
    if (desc_.enableValidation) {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        const char* validationLayer = "VK_LAYER_KHRONOS_validation";
        bool layerFound = false;
        for (const auto& layer : availableLayers) {
            if (std::strcmp(layer.layerName, validationLayer) == 0) {
                layerFound = true;
                break;
            }
        }
        if (layerFound) {
            layers.push_back(validationLayer);
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
    }

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    createInfo.ppEnabledLayerNames = layers.data();

    VkResult vkRes = vkCreateInstance(&createInfo, nullptr, &instance_);
    if (vkRes != VK_SUCCESS) {
        return makeVkError(vkRes, "Failed to create Vulkan instance");
    }

    if (desc_.enableValidation) {
        auto createFunc = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT"));
        if (createFunc) {
            VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
            debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugInfo.pfnUserCallback = debugCallback;
            createFunc(instance_, &debugInfo, nullptr, &debugMessenger_);
        }
    }

    return {};
}

core::Result<void, core::Error> VulkanDevice::selectPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (deviceCount == 0) {
        return core::Error(core::ErrorCode::GraphicsError,
                           "Failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    // Score and pick best device
    VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
    uint32_t bestScore = 0;
    uint32_t bestGraphicsFamily = UINT32_MAX;

    for (const auto& dev : devices) {
        VkPhysicalDeviceProperties props;
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceProperties(dev, &props);
        vkGetPhysicalDeviceFeatures(dev, &features);

        // Must support Vulkan 1.3
        if (props.apiVersion < VK_API_VERSION_1_3) {
            continue;
        }

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

        uint32_t graphicsFamily = UINT32_MAX;
        for (uint32_t i = 0; i < queueFamilyCount; ++i) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsFamily = i;
                break;
            }
        }

        if (graphicsFamily == UINT32_MAX) {
            continue;
        }

        uint32_t score = 100;
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        if (score > bestScore) {
            bestScore = score;
            bestDevice = dev;
            bestGraphicsFamily = graphicsFamily;
        }
    }

    if (bestDevice == VK_NULL_HANDLE) {
        return core::Error(core::ErrorCode::GraphicsError,
                           "Failed to find a suitable Vulkan 1.3 physical device");
    }

    physicalDevice_ = bestDevice;
    graphicsQueueFamily_ = bestGraphicsFamily;
    presentQueueFamily_ = bestGraphicsFamily;

    return {};
}

core::Result<void, core::Error> VulkanDevice::createLogicalDevice() {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {graphicsQueueFamily_, presentQueueFamily_};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkPhysicalDeviceVulkan13Features features13{};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    VkPhysicalDeviceVulkan12Features features12{};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.pNext = &features13;
    features12.timelineSemaphore = VK_TRUE;

    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &features12;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &features2;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkResult res = vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_);
    if (res != VK_SUCCESS) {
        return makeVkError(res, "Failed to create Vulkan logical device");
    }

    vkGetDeviceQueue(device_, graphicsQueueFamily_, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, presentQueueFamily_, 0, &presentQueue_);

    allocator_ = std::make_unique<VulkanMemoryAllocator>(device_, physicalDevice_);

    return {};
}

core::Result<void, core::Error> VulkanDevice::createCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsQueueFamily_;

    VkResult res = vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_);
    if (res != VK_SUCCESS) {
        return makeVkError(res, "Failed to create Vulkan command pool");
    }
    return {};
}

core::Result<void, core::Error> VulkanDevice::createDescriptorPool() {
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128},
    };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 128;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = poolSizes;

    VkResult res = vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_);
    if (res != VK_SUCCESS) {
        return makeVkError(res, "Failed to create Vulkan descriptor pool");
    }
    return {};
}

core::Result<void, core::Error> VulkanDevice::createTimelineSemaphore() {
    VkSemaphoreTypeCreateInfo timelineInfo{};
    timelineInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    timelineInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timelineInfo.initialValue = 0;

    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semInfo.pNext = &timelineInfo;

    VkResult res = vkCreateSemaphore(device_, &semInfo, nullptr, &timelineSemaphore_);
    if (res != VK_SUCCESS) {
        return makeVkError(res, "Failed to create Vulkan timeline semaphore");
    }
    return {};
}

core::Result<std::unique_ptr<Swapchain>, core::Error>
VulkanDevice::createSwapchain(const SwapchainDesc& desc) {
    auto swapchainRes = VulkanSwapchain::create(this, desc);
    if (swapchainRes.isErr()) {
        return swapchainRes.error();
    }
    return std::unique_ptr<Swapchain>(swapchainRes.value().release());
}

core::Result<std::unique_ptr<CommandBuffer>, core::Error> VulkanDevice::createCommandBuffer() {
    return std::make_unique<VulkanCommandBuffer>(device_, commandPool_, this);
}

core::Result<BufferHandle, core::Error> VulkanDevice::createBuffer(const BufferDesc& desc,
                                                                   const void* initialData) {
    auto bufRes = VulkanBuffer::create(device_, *allocator_, desc, initialData);
    if (bufRes.isErr()) {
        return bufRes.error();
    }
    BufferHandle handle = bufferPool_.allocate(std::move(bufRes.value()));
    return handle;
}

core::Result<void, core::Error> VulkanDevice::writeBuffer(BufferHandle handle, uint64_t offset,
                                                          uint64_t size, const void* data) {
    auto* bufPtr = bufferPool_.get(handle);
    if (!bufPtr || !(*bufPtr)) {
        return core::Error(core::ErrorCode::InvalidArgument, "Invalid buffer handle");
    }
    return (*bufPtr)->write(offset, size, data);
}

void* VulkanDevice::mapBuffer(BufferHandle handle) {
    auto* bufPtr = bufferPool_.get(handle);
    if (!bufPtr || !(*bufPtr)) {
        return nullptr;
    }
    return (*bufPtr)->mappedData();
}

core::Result<TextureHandle, core::Error> VulkanDevice::createTexture(const TextureDesc& desc,
                                                                     const void* initialData) {
    auto texRes = VulkanTexture::create(device_, *allocator_, desc);
    if (texRes.isErr()) {
        return texRes.error();
    }
    TextureHandle handle = texturePool_.allocate(std::move(texRes.value()));
    if (initialData != nullptr) {
        auto writeRes = writeTexture(handle, 0, 0, desc.width, desc.height, initialData);
        if (writeRes.isErr()) {
            destroyTexture(handle);
            return writeRes.error();
        }
    }
    return handle;
}

core::Result<void, core::Error> VulkanDevice::writeTexture(TextureHandle handle, uint32_t x,
                                                           uint32_t y, uint32_t width,
                                                           uint32_t height, const void* data) {
    auto* texPtr = texturePool_.get(handle);
    if (!texPtr || !(*texPtr)) {
        return core::Error(core::ErrorCode::InvalidArgument, "Invalid texture handle");
    }
    if (!data || width == 0 || height == 0) {
        return core::Error(core::ErrorCode::InvalidArgument, "Invalid writeTexture arguments");
    }

    VulkanTexture* tex = texPtr->get();

    uint32_t bytesPerPixel = 4;
    switch (tex->desc().format) {
    case Format::R8_Unorm:
        bytesPerPixel = 1;
        break;
    case Format::R16G16_Sfloat:
    case Format::R32_Sfloat:
    case Format::R8G8B8A8_Unorm:
    case Format::B8G8R8A8_Unorm:
        bytesPerPixel = 4;
        break;
    case Format::R32G32_Sfloat:
        bytesPerPixel = 8;
        break;
    case Format::R32G32B32A32_Sfloat:
        bytesPerPixel = 16;
        break;
    default:
        bytesPerPixel = 4;
        break;
    }

    uint64_t uploadSize = static_cast<uint64_t>(width) * height * bytesPerPixel;

    BufferDesc stagingDesc{
        .size = uploadSize,
        .usage = BufferUsage::TransferSrc,
        .memoryUsage = MemoryUsage::CpuToGpu,
    };

    auto stagingRes = VulkanBuffer::create(device_, *allocator_, stagingDesc, data);
    if (stagingRes.isErr()) {
        return stagingRes.error();
    }
    auto stagingBuf = std::move(stagingRes.value());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkResult res = vkAllocateCommandBuffers(device_, &allocInfo, &cmd);
    if (res != VK_SUCCESS) {
        return makeVkError(res, "Failed to allocate command buffer for writeTexture");
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = tex->layout();
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = tex->image();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    barrier.srcAccessMask = 0;
    if (barrier.oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd, srcStage, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                         1, &barrier);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {static_cast<int32_t>(x), static_cast<int32_t>(y), 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuf->handle(), tex->image(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    tex->setLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    res = vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
    if (res != VK_SUCCESS) {
        vkFreeCommandBuffers(device_, commandPool_, 1, &cmd);
        return makeVkError(res, "Failed to submit writeTexture command buffer");
    }

    vkQueueWaitIdle(graphicsQueue_);
    vkFreeCommandBuffers(device_, commandPool_, 1, &cmd);

    return {};
}

core::Result<PipelineHandle, core::Error> VulkanDevice::createPipeline(const PipelineDesc& desc) {
    auto pipeRes = VulkanPipeline::create(device_, desc);
    if (pipeRes.isErr()) {
        return pipeRes.error();
    }
    PipelineHandle handle = pipelinePool_.allocate(std::move(pipeRes.value()));
    return handle;
}

void VulkanDevice::destroyBuffer(BufferHandle handle) {
    auto* bufPtr = bufferPool_.get(handle);
    if (bufPtr && *bufPtr) {
        RetiredResource retired{};
        retired.timelineValue = currentTimelineValue_;
        retired.buffer = std::move(*bufPtr);
        retiredResources_.push_back(std::move(retired));
        bufferPool_.release(handle);
    }
}

void VulkanDevice::destroyTexture(TextureHandle handle) {
    for (auto it = descriptorSetCache_.begin(); it != descriptorSetCache_.end();) {
        if ((it->first & 0xFFFFFFFF) == handle.index) {
            it = descriptorSetCache_.erase(it);
        } else {
            ++it;
        }
    }

    auto* texPtr = texturePool_.get(handle);
    if (texPtr && *texPtr) {
        RetiredResource retired{};
        retired.timelineValue = currentTimelineValue_;
        retired.texture = std::move(*texPtr);
        retiredResources_.push_back(std::move(retired));
        texturePool_.release(handle);
    }
}

void VulkanDevice::destroyPipeline(PipelineHandle handle) {
    for (auto it = descriptorSetCache_.begin(); it != descriptorSetCache_.end();) {
        if ((it->first >> 32) == handle.index) {
            it = descriptorSetCache_.erase(it);
        } else {
            ++it;
        }
    }

    auto* pipePtr = pipelinePool_.get(handle);
    if (pipePtr && *pipePtr) {
        RetiredResource retired{};
        retired.timelineValue = currentTimelineValue_;
        retired.pipeline = std::move(*pipePtr);
        retiredResources_.push_back(std::move(retired));
        pipelinePool_.release(handle);
    }
}

VkDescriptorSet VulkanDevice::getOrCreateTextureDescriptorSet(PipelineHandle pipelineHandle,
                                                              TextureHandle textureHandle) {
    uint64_t key = (static_cast<uint64_t>(pipelineHandle.index) << 32) |
                   static_cast<uint64_t>(textureHandle.index);
    auto it = descriptorSetCache_.find(key);
    if (it != descriptorSetCache_.end()) {
        return it->second;
    }

    auto* pipePtr = pipelinePool_.get(pipelineHandle);
    auto* texPtr = texturePool_.get(textureHandle);
    if (!pipePtr || !(*pipePtr) || !texPtr || !(*texPtr)) {
        return VK_NULL_HANDLE;
    }

    VulkanPipeline* pipe = pipePtr->get();
    VulkanTexture* tex = texPtr->get();

    VkDescriptorSetLayout layout = pipe->descriptorSetLayout();
    if (layout == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet set = VK_NULL_HANDLE;
    VkResult res = vkAllocateDescriptorSets(device_, &allocInfo, &set);
    if (res != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = tex->imageView();
    imageInfo.sampler = tex->sampler();

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
    descriptorSetCache_[key] = set;
    return set;
}

core::Result<void, core::Error> VulkanDevice::submit(CommandBuffer* cmdBuffer,
                                                     Swapchain* swapchain) {
    auto* vkCmd = dynamic_cast<VulkanCommandBuffer*>(cmdBuffer);
    if (!vkCmd) {
        return core::Error(core::ErrorCode::InvalidArgument, "Invalid command buffer");
    }

    currentTimelineValue_++;

    std::vector<VkSemaphore> waitSemaphores;
    std::vector<uint64_t> waitSemaphoreValues;
    std::vector<VkPipelineStageFlags> waitStages;

    std::vector<VkSemaphore> signalSemaphores;
    std::vector<uint64_t> signalSemaphoreValues;

    signalSemaphores.push_back(timelineSemaphore_);
    signalSemaphoreValues.push_back(currentTimelineValue_);

    VkFence submitFence = VK_NULL_HANDLE;

    if (swapchain) {
        auto* vkSwap = dynamic_cast<VulkanSwapchain*>(swapchain);
        if (vkSwap) {
            submitFence = vkSwap->currentInFlightFence();
            vkResetFences(device_, 1, &submitFence);

            waitSemaphores.push_back(vkSwap->currentImageAvailableSemaphore());
            waitSemaphoreValues.push_back(0);
            waitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

            signalSemaphores.push_back(vkSwap->currentRenderFinishedSemaphore());
            signalSemaphoreValues.push_back(0);
        }
    }

    VkTimelineSemaphoreSubmitInfo timelineInfo{};
    timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    timelineInfo.waitSemaphoreValueCount = static_cast<uint32_t>(waitSemaphoreValues.size());
    timelineInfo.pWaitSemaphoreValues =
        waitSemaphoreValues.empty() ? nullptr : waitSemaphoreValues.data();
    timelineInfo.signalSemaphoreValueCount = static_cast<uint32_t>(signalSemaphoreValues.size());
    timelineInfo.pSignalSemaphoreValues = signalSemaphoreValues.data();

    VkCommandBuffer rawCmd = vkCmd->handle();
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = &timelineInfo;
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.empty() ? nullptr : waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.empty() ? nullptr : waitStages.data();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &rawCmd;
    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    VkResult res = vkQueueSubmit(graphicsQueue_, 1, &submitInfo, submitFence);
    if (res != VK_SUCCESS) {
        return makeVkError(res, "Failed to submit command buffer to graphics queue");
    }

    return {};
}

void VulkanDevice::advanceFrame() {
    retireResources();
}

void VulkanDevice::retireResources() {
    if (retiredResources_.empty() || timelineSemaphore_ == VK_NULL_HANDLE) {
        return;
    }

    uint64_t completedValue = 0;
    VkResult res = vkGetSemaphoreCounterValue(device_, timelineSemaphore_, &completedValue);
    if (res != VK_SUCCESS) {
        return;
    }

    auto it = std::remove_if(
        retiredResources_.begin(), retiredResources_.end(),
        [completedValue](const RetiredResource& r) { return r.timelineValue <= completedValue; });
    retiredResources_.erase(it, retiredResources_.end());
}

void VulkanDevice::waitIdle() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }
    retireResources();
}

VulkanBuffer* VulkanDevice::getBuffer(BufferHandle handle) {
    auto* ptr = bufferPool_.get(handle);
    return ptr ? ptr->get() : nullptr;
}

VulkanTexture* VulkanDevice::getTexture(TextureHandle handle) {
    auto* ptr = texturePool_.get(handle);
    return ptr ? ptr->get() : nullptr;
}

VulkanPipeline* VulkanDevice::getPipeline(PipelineHandle handle) {
    auto* ptr = pipelinePool_.get(handle);
    return ptr ? ptr->get() : nullptr;
}

TextureHandle VulkanDevice::registerExternalTexture(std::unique_ptr<VulkanTexture> texture) {
    return texturePool_.allocate(std::move(texture));
}

void VulkanDevice::unregisterExternalTexture(TextureHandle handle) {
    texturePool_.release(handle);
}

} // namespace northern::graphics

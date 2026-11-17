#include "graphics/device.hpp"
#include "graphics/vulkan/vk_device.hpp"

namespace northern::graphics {

core::Result<std::unique_ptr<Device>, core::Error> Device::create(const DeviceDesc& desc) {
    auto vkDeviceRes = VulkanDevice::create(desc);
    if (vkDeviceRes.isErr()) {
        return vkDeviceRes.error();
    }
    return std::unique_ptr<Device>(vkDeviceRes.value().release());
}

} // namespace northern::graphics

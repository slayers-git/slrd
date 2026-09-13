/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_PIPELINECACHE_HPP__
#define __SLRD_VULKAN_PIPELINECACHE_HPP__

#include <span>
#include <string_view>
#include <vector>
#include <vulkan/vulkan.h>

namespace slrd {
    class VKDevice;

    struct VKPipelineCacheHeader {
        uint32_t headerSize;

        uint32_t headerVersion;
        uint32_t vendorID;
        uint32_t deviceID;
        uint32_t driverVersion;
        uint8_t  pipelineCacheUUID[VK_UUID_SIZE];
    };

    class VKPipelineCache {
    public:
        VKPipelineCache() noexcept = default;
        ~VKPipelineCache() noexcept;

        int init(VKDevice *device, std::span<const uint8_t> data) noexcept;
        int init(VKDevice *device, std::string_view path) noexcept;

        [[nodiscard]]
        VkPipelineCache handle() noexcept {
            return m_cache;
        }

        /* Get the related data */
        std::vector<uint8_t> getData() const noexcept;

        /* Save the pipeline cache data to disk */
        int saveToDisk(std::string_view path) noexcept;

    private:
        static bool isValid(
                std::span<const uint8_t> data,
                const VkPhysicalDeviceProperties& props) noexcept;

        VKDevice *m_device = nullptr;
        VkPipelineCache m_cache = VK_NULL_HANDLE;
    };
};

#endif /* #define __SLRD_VULKAN_PIPELINECACHE_HPP__ */

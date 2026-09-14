/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "pipelinecache.hpp"

#include "debug.hpp"
#include "device.hpp"
#include "vulkan/error.hpp"
#include <vulkan/vulkan.h>

#include <fstream>
#include <vulkan/vulkan_core.h>

namespace slrd {
    VKPipelineCache::~VKPipelineCache() noexcept {
        if (m_cache) {
            vkDestroyPipelineCache(m_device->getVkDevice(), m_cache, nullptr);
            m_device->vkdeallocate(VK_OBJECT_TYPE_PIPELINE_CACHE, 0);
        }
    }

    int VKPipelineCache::init(
            VKDevice *device,
            std::span<const uint8_t> data) noexcept {
        std::span<const uint8_t> validated;

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device->getPhysicalDevice(), &props);

        if (!data.empty() && isValid(data, props)) {
            validated = data.subspan(sizeof(VKPipelineCacheHeader));
        }

        VkPipelineCache cache;

        VkPipelineCacheCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        info.initialDataSize = validated.size();
        info.pInitialData = !validated.empty() ? validated.data() : nullptr;

        VK_WRAP_RETURN_RESULT_LOGERROR(
            vkCreatePipelineCache(device->getVkDevice(), &info, nullptr, &cache),
            "Failed to create a VkPipelineCache");

        size_t actual_size = 0;
        VkResult result = vkGetPipelineCacheData(
                device->getVkDevice(), cache, &actual_size, nullptr);
        /* Can reloading introduce size variance? */
        SLRD_COMPLAIN_IF(
                result != VK_SUCCESS ||
                (!validated.empty() && actual_size != validated.size()),
                "Initial pipeline cache data may not have been used.");

        device->vkallocate(VK_OBJECT_TYPE_PIPELINE_CACHE, 0);

        m_cache  = cache;
        m_device = device;

        return 0;
    }

    int VKPipelineCache::init(VKDevice *device, const std::string_view path) noexcept {
        SLRD_ASSERT(!path.empty());

        std::ifstream fs(path.data(), std::ios::binary | std::ios::ate);
        if (!fs.is_open())
            return init(device, std::span<uint8_t>());

        size_t size = fs.tellg();
        fs.seekg(0);

        std::vector<uint8_t> data(size);
        fs.read(reinterpret_cast<char*>(data.data()), size);
        if (!fs.good())
            return -1;

        return init(device, data);
    }

    std::vector<uint8_t> VKPipelineCache::getData() const noexcept {
        size_t size = 0;
        VkResult result;

        std::vector<uint8_t> data;

        auto device = m_device->getVkDevice();
        do {
            result = vkGetPipelineCacheData(device, m_cache, &size, nullptr);
            if (result != VK_SUCCESS || !size) {
                return {};
            }

            data.resize(size);
            result = vkGetPipelineCacheData(device, m_cache, &size, data.data());
        } while (result == VK_INCOMPLETE);

        if (result != VK_SUCCESS) {
            return {};
        }

        data.resize(size);
        return data;
    }

    int VKPipelineCache::saveToDisk(std::string_view path) noexcept {
        SLRD_ASSERT(!path.empty());

        auto data = getData();
        SLRD_COMPLAIN_RETURN(data.empty(),
                -1,
                "VKPipelineCache::saveToDisk: Failed to get cache data");

        std::ofstream fs(path.data(), std::ios::binary);
        SLRD_COMPLAIN_RETURN(!fs.is_open(),
                -1,
                "VKPipelineCache::saveToDisk: Failed to open file");
        
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(m_device->getPhysicalDevice(), &props);

        VKPipelineCacheHeader header;
        header.headerSize = sizeof(header);
        header.headerVersion = VK_PIPELINE_CACHE_HEADER_VERSION_ONE;
        header.vendorID = props.vendorID;
        header.deviceID = props.deviceID;
        header.driverVersion = props.driverVersion;
        memcpy(header.pipelineCacheUUID, props.pipelineCacheUUID, 16);

        fs.write(reinterpret_cast<char *>(&header), sizeof(header));
        fs.write(reinterpret_cast<char *>(data.data()), data.size());

        SLRD_COMPLAIN_RETURN(!fs.good(),
                -1,
                "VKPipelineCache::saveToDisk: Failed to write to file");

        return 0;
    }

    bool VKPipelineCache::isValid(
            std::span<const uint8_t> data,
            const VkPhysicalDeviceProperties& props) noexcept {
        if (data.size() < sizeof(VKPipelineCacheHeader)) {
            return false;
        }

        VKPipelineCacheHeader header;
        std::memcpy(&header, data.data(), sizeof(header));

        return header.headerSize == sizeof(VKPipelineCacheHeader) &&
            header.headerVersion == VK_PIPELINE_CACHE_HEADER_VERSION_ONE &&
            header.vendorID == props.vendorID &&
            header.deviceID == props.deviceID &&
            header.driverVersion == props.driverVersion &&
            memcmp(header.pipelineCacheUUID, props.pipelineCacheUUID, 16) == 0;
    }
};

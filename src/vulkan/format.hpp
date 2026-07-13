/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_FORMAT_HPP__
#define __SLRD_VULKAN_FORMAT_HPP__

#include "debug.hpp"
#include <vulkan/vulkan.hpp>

#include <slrd/format.hpp>

namespace slrd {
    struct VKDevice;

    [[nodiscard]]
    VkFormat getVkFormat (slrd::Format format);

    [[nodiscard]]
    slrd::Format getSLRDFormat (VkFormat vkformat);
    
    [[nodiscard]]
    slrd::Format getFittingDepthFormat (VKDevice *device, slrd::Format format);

    /* Should technically allow for a bitset, but ... */
    inline constexpr VkSampleCountFlagBits getVkSampleCount (slrd::MSAACount type) {
#define __MSAA_CASE(__Type) \
        case MSAA_ ## __Type: count = VK_SAMPLE_ ## __Type ## _BIT; break;
        VkSampleCountFlagBits count = VK_SAMPLE_COUNT_1_BIT;

        switch (type) {
            __MSAA_CASE (COUNT_1);
            __MSAA_CASE (COUNT_2);
            __MSAA_CASE (COUNT_4);
            __MSAA_CASE (COUNT_8);
            __MSAA_CASE (COUNT_16);
            default:
                SLRD_DEBUG_CRIT ("getVkSamleCountOp: invalid enum");
                break;
        }

#undef __MSAA_CASE

        return count;
    }

    inline constexpr VkImageLayout getVkTextureLayout (slrd::TextureLayout layout) {
#define __LAYOUT_CASE(__Layout, __VkLayout) \
        case __Layout: vklayout = __VkLayout; break;
        VkImageLayout vklayout = VK_IMAGE_LAYOUT_GENERAL;

        switch (layout) {
            __LAYOUT_CASE (TEXTURE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED);
            __LAYOUT_CASE (TEXTURE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL);
            __LAYOUT_CASE (TEXTURE_LAYOUT_SWAPCHAIN_SRC, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
            __LAYOUT_CASE (TEXTURE_LAYOUT_COLOR_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            __LAYOUT_CASE (TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
            __LAYOUT_CASE (TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
            __LAYOUT_CASE (TEXTURE_LAYOUT_SHADER_READ_ONLY, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            __LAYOUT_CASE (TEXTURE_LAYOUT_TRANSFER_DST, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            __LAYOUT_CASE (TEXTURE_LAYOUT_TRANSFER_SRC, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            default:
                SLRD_DEBUG_CRIT ("getVkTextureLayout: invalid enum");
                break;
        }

#undef __LAYOUT_CASE

        return vklayout;
    }

    inline constexpr VkAttachmentLoadOp getVkLoadOp (slrd::LoadOperation operation) {
        switch (operation) {
            case LOAD_OPERATION_LOAD:
                return VK_ATTACHMENT_LOAD_OP_LOAD;
            case LOAD_OPERATION_CLEAR:
                return VK_ATTACHMENT_LOAD_OP_CLEAR;
            case LOAD_OPERATION_DONT_CARE:
                return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }

        SLRD_DEBUG_CRIT ("getVkLoadOp: invalid enum");
    }
    inline constexpr VkAttachmentStoreOp getVkStoreOp (slrd::StoreOperation operation) {
        switch (operation) {
            case STORE_OPERATION_STORE:
                return VK_ATTACHMENT_STORE_OP_STORE;
            case STORE_OPERATION_DONT_CARE:
                return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        SLRD_DEBUG_CRIT ("getVkStoreOp: invalid enum");
    }
}

#endif /* #define __SLRD_VULKAN_FORMAT_HPP__ */

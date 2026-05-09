/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "texture.hpp"
#include "debug.hpp"
#include "vulkan/error.hpp"
#include "vulkan/format.hpp"
#include "error.hpp"
#include "device.hpp"

namespace slrd {
    int VKTexture::init (VKDevice *device, const TextureInfo& info) {
        VkImage image;

        m_format = ::slrd::getVkFormat (info.format);
        m_width = info.width;
        m_height = info.height;
        m_depth = info.depth;
        m_type = info.type;

        if (info.type == TEXTURE_TYPE_CUBE_MAP) {
            SLRD_COMPLAIN_RETURN (info.arrayLayers % 6 != 0, -1,
                    "The cubemap texture can't be created, because the " \
                    "layer count is not divisible by 6!");
        }
        
        VkImageCreateInfo imInfo {};
        imInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imInfo.imageType = getVkTextureType (m_type);
        imInfo.format = m_format;
        imInfo.extent.width = info.width;
        imInfo.extent.height = info.height;
        imInfo.extent.depth = info.depth;
        imInfo.mipLevels = info.mipmaps;
        imInfo.arrayLayers = info.arrayLayers;
        imInfo.samples = getVkSampleCount (info.samples);
        imInfo.tiling = getVkTextureTiling (info.tiling);
        imInfo.usage = getVkTextureUsage (info.usage);
        imInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        /* this should probably depend on the queues */
        imInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imInfo.flags = (info.type == TEXTURE_TYPE_CUBE_MAP) ?
            VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

        VmaAllocation allocation;
        VmaAllocationCreateInfo allocationCreateInfo {};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocationCreateInfo.pool = nullptr;
        allocationCreateInfo.flags = 0;

        VmaAllocationInfo allocationInfo {};

        RETURN_LOG_ERROR_IF (vmaCreateImage (device->getVkAllocator (), &imInfo, 
                &allocationCreateInfo, &image, &allocation, &allocationInfo),
                -1, "Failed to allocate memory for image");

        m_image = image;
        m_allocation = allocation;
        setParentDevice (device);
        m_valid = true;

        if (!info.name.empty ())
            setResourceName (info.name, VK_OBJECT_TYPE_IMAGE, m_image);

        // FIXME: The actual size is not calculated and set
        device->allocate (OBJECT_TYPE_TEXTURE, 0);

        return 0;
    }

    int VKTexture::createFromExisting (VKDevice *device,
            VkImage image, TextureType type,
            uint32_t width, uint32_t height, uint32_t depth,
            VkFormat format, VKSwapchain *swapchain) {
        setParentDevice (device);
        m_image = image;

        m_type = type;
        m_width = width;
        m_height = height;
        m_depth = depth;
        m_format = format;
        
        m_swapchain = swapchain;
        m_valid = true;
        
        return 0;
    }

    VKTexture::~VKTexture () {
        /* If the image is managed by this class, destroy it */
        if (m_image && m_allocation)
            vmaDestroyImage (m_device->getVkAllocator (), m_image, m_allocation);
    }

    static constexpr VkImageViewType getVkImageViewType (TextureType tt) {
        switch (tt) {
            case TEXTURE_TYPE_1D: return VK_IMAGE_VIEW_TYPE_1D;
            case TEXTURE_TYPE_2D: return VK_IMAGE_VIEW_TYPE_2D;
            case TEXTURE_TYPE_3D: return VK_IMAGE_VIEW_TYPE_3D;
            case TEXTURE_TYPE_CUBE_MAP: return VK_IMAGE_VIEW_TYPE_CUBE;
        }

        SLRD_DEBUG_CRIT ("getVkImageViewType: invalid type");
    }
    int VKTexture::getDimensions (uint32_t& w, uint32_t& h, uint32_t& depth) {
        w = m_width;
        h = m_height;
        depth = m_depth;
        
        return 0;
    }

    /* Create a custom texture view */
    Ref<ITextureView> VKTexture::createTextureView (const TextureViewInfo& viewData) {
        SLRD_ASSERT (m_image);
        return Ref<ITextureView>::adopt (makeResource<VKTextureView> (this, viewData));
    }

    bool VKTexture::isValid () const {
        return m_valid;
    }
    
    void VKTexture::invalidate () {
        m_valid = false;
    }

    std::string_view VKTexture::getName () const noexcept {
        return getResourceName ();
    }



    int VKTextureView::init (VKTexture *texture, const TextureViewInfo& viewData) {
        SLRD_ASSERT (texture != nullptr);

        VkImageView imageView;

        VkImageViewType vkviewType = getVkImageViewType (texture->m_type);

        VkImageViewCreateInfo ivInfo {};
        ivInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ivInfo.image = texture->m_image;
        ivInfo.format = texture->m_format;
        
        ivInfo.viewType = vkviewType;
        ivInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        ivInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        ivInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        ivInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        ivInfo.subresourceRange.aspectMask = getVkTextureAspectFlags (viewData.aspect);
        ivInfo.subresourceRange.layerCount = viewData.arrayLayers;
        ivInfo.subresourceRange.levelCount = viewData.mipLevels;
        ivInfo.subresourceRange.baseArrayLayer = viewData.arrayLayer;
        ivInfo.subresourceRange.baseMipLevel = viewData.mipLevel;

        VK_WRAP_RETURN_RESULT (
                vkCreateImageView (texture->m_device->getVkDevice (), &ivInfo, nullptr, &imageView)
                );

        m_texture = Ref<VKTexture>::share (texture);
        m_device  = texture->m_device;
        m_view    = imageView;

        if (!viewData.name.empty ())
            setResourceName (viewData.name, VK_OBJECT_TYPE_IMAGE_VIEW, m_view);

        m_device->allocate (OBJECT_TYPE_TEXTURE_VIEW, 0);

        return 0;
    }

    std::string_view VKTextureView::getName () const noexcept {
        return getResourceName ();
    }

    VKTextureView::~VKTextureView () {
        if (m_view) {
            vkDestroyImageView (m_device->getVkDevice (), m_view, nullptr);
        }
    }
};

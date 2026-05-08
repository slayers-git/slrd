/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_TEXTURE_HPP__
#define __SLRD_VULKAN_TEXTURE_HPP__

#include "slrd/ref.hpp"
#include "slrd/texture.hpp"
#include "vulkan/deviceobject.hpp"
#include "vulkan/factory.hpp"
#include "vulkan/resource.hpp"
#include <utility>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace slrd {
    class VKDevice;
    class VKSwapchain;
    class VKTexture;
    class VKTextureView;

    SLRD_RESOURCE_DEFINE_TYPE(VKTextureView, VK_OBJECT_TYPE_IMAGE_VIEW);
    class VKTextureView :
        public VKDeviceObject<ITextureView>,
        public VKNamedResource<VKTextureView> {
    private:
        /* Strong reference, because we need the texture for as long as the
         * view lives */
        Ref<VKTexture> m_texture;
        VkImageView m_view = VK_NULL_HANDLE;

    public:
        inline VKTextureView (VKTexture *texture, VkImageView view);

        VKTextureView () = default;
        ~VKTextureView ();

        [[nodiscard]] auto& getDevice () const {
            return m_device;
        }

        [[nodiscard]] auto& getTexture () const {
            return m_texture;
        }

        [[nodiscard]] VkImageView getView () const {
            return m_view;
        }

        VKTextureView (VKTextureView&& other) noexcept :
            m_texture (std::move (other.m_texture)),
            m_view (std::exchange (other.m_view, VK_NULL_HANDLE)) { }

        VKTextureView operator= (const VKTextureView&) = delete;
        VKTextureView& operator= (VKTextureView&& other) noexcept {
            if (&other != this) {
                m_texture = std::exchange (other.m_texture, nullptr);
                m_view = std::exchange (other.m_view, VK_NULL_HANDLE);
            }

            return *this;
        }

        operator bool () const {
            return m_view;
        }

        int init (VKTexture *texture, const TextureViewInfo& viewData);

        VkImageView handle () const {
            return m_view;
        }

        std::string_view getName () const noexcept final;
    };

    SLRD_RESOURCE_DEFINE_TYPE(VKTexture, VK_OBJECT_TYPE_IMAGE);
    class VKTexture :
            public VKDeviceObject<ITexture>,
            public VKNamedResource<VKTexture> {
    private:
        VkImage m_image = VK_NULL_HANDLE;

        /* The swapchain that this texture was created from
         *
         * Needed, since if the texture is an attachment to the renderpass, we
         * want to know what swapchain to signal when the commandbuffer is
         * submitted */
        VKSwapchain *m_swapchain = nullptr;

        /* The memory of the texture 
         *
         * The texture is considered unmanaged, if this is VK_NULL_HANDLE */
        VmaAllocation m_allocation = VK_NULL_HANDLE;

        uint32_t m_width;
        uint32_t m_height;
        uint32_t m_depth;
        VkFormat m_format;

        TextureType m_type;

        bool m_valid;

    public:
        [[nodiscard]] auto& getDevice () const {
            return m_device;
        }

        [[nodiscard]] VkImage getImage () const {
            return m_image;
        }

        [[nodiscard]] VKSwapchain *getSwapchain () const {
            return m_swapchain;
        }

        [[nodiscard]] uint32_t getWidth () const {
            return m_width;
        }

        [[nodiscard]] uint32_t getHeight () const {
            return m_height;
        }

        [[nodiscard]] uint32_t getDepth () const {
            return m_depth;
        }

        [[nodiscard]] VkFormat getVkFormat () const {
            return m_format;
        }

        [[nodiscard]] TextureType getTextureType () const {
            return m_type;
        }

        /* Initialize the texture */
        int init (VKDevice *device, const TextureInfo& info);

        int createFromExisting (VKDevice *device,
                VkImage image, TextureType type,
                uint32_t width, uint32_t height, uint32_t depth,
                VkFormat format, VKSwapchain *swapchain = nullptr);

        bool isValid () const final;
        int getDimensions (uint32_t& w, uint32_t& h, uint32_t& depth) final;

        /* Mark the texture as invalid */
        void invalidate ();

        /* Create a custom texture view */
        Ref<ITextureView> createTextureView (const TextureViewInfo& view) final;

        ~VKTexture ();

        VkImage handle () const {
            return m_image;
        }
        std::string_view getName () const noexcept final;

        friend class VKTextureView;
    };

    VKTextureView::VKTextureView (VKTexture *texture, VkImageView view) {
        m_device = texture->getDevice ();
        m_texture = Ref<VKTexture>::share(texture);
        m_view = view;
    }

    /* Create a texture */
    inline VKTexture *createVKTexture (VKDevice *device, const TextureInfo& info) {
        return makeResource<VKTexture> (device, info);
    }



    /* Helpers */

    inline constexpr VkImageType getVkTextureType (slrd::TextureType type) {
#define __TYPE_CASE(__Type) \
        case TEXTURE_ ## __Type: vktype = VK_IMAGE_ ## __Type; break;

        VkImageType vktype = VK_IMAGE_TYPE_2D;

        switch (type) {
            __TYPE_CASE (TYPE_1D);
            __TYPE_CASE (TYPE_2D);
            __TYPE_CASE (TYPE_3D);

            /* Synthetic type */
            case TEXTURE_TYPE_CUBE_MAP:
                return VK_IMAGE_TYPE_2D;
        }

#undef __TYPE_CASE

        return vktype;
    }

    inline constexpr VkImageTiling getVkTextureTiling (slrd::TextureTilingMode type) {
#define __TILING_CASE(__Type) \
        case TEXTURE_TILING_ ## __Type: vktiling = VK_IMAGE_TILING_ ## __Type; break;

        VkImageTiling vktiling = VK_IMAGE_TILING_LINEAR;

        switch (type) {
            __TILING_CASE (OPTIMAL);
            __TILING_CASE (LINEAR);
        }

#undef __TILING_CASE

        return vktiling;
    }

    inline constexpr VkImageAspectFlags getVkTextureAspectFlags (slrd::TextureAspectFlags flags) {
        VkImageAspectFlags usage = 0;
        if (flags & TEXTURE_ASPECT_COLOR) {
            usage |= VK_IMAGE_ASPECT_COLOR_BIT;
        }
        if (flags & TEXTURE_ASPECT_DEPTH) {
            usage |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        if (flags & TEXTURE_ASPECT_STENCIL) {
            usage |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        return usage;
    }

    inline constexpr VkImageUsageFlags getVkTextureUsage (slrd::TextureUsageFlags flags) {
        VkImageUsageFlags usage = 0;
        if (flags & TEXTURE_USAGE_SAMPLED) {
            usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if (flags & TEXTURE_USAGE_DEPTH) {
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        if (flags & TEXTURE_USAGE_STENCIL) {
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        if (flags & TEXTURE_USAGE_TRANSFER_DST) {
            usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if (flags & TEXTURE_USAGE_TRANSFER_SRC) {
            usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if (flags & TEXTURE_USAGE_COLOR_ATTACHMENT) {
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        if (flags & TEXTURE_USAGE_STORAGE) {
            usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        }

        return usage;
    }
};

#endif /* #define __SLRD_VULKAN_TEXTURE_HPP__ */

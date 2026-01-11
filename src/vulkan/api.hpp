/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VK_API_HPP__
#define __SLRD_VK_API_HPP__

#include <memory>
#include <slrd/api.hpp>
#include <vulkan/vulkan.h>

#include <slrd/pipeline.hpp>

#include <slrd/platform/vulkan.hpp>

namespace slrd {
    class VKDevice;

    class VKAPI : public VulkanData {
    public:
        VKAPI () = default;

        int init (const APIConfig&);
        ~VKAPI ();
    };

    extern std::unique_ptr<VKAPI> vkapi;

    int initVulkanAPI (const APIConfig&);
    void deinitVulkanAPI (void);
};

#endif /* #define __SLRD_VK_API_HPP__ */

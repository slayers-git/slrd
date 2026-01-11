/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_ERROR_HPP__
#define __SLRD_VULKAN_ERROR_HPP__

#include <format>
#include "../error.hpp"

#include <slrd/config.hpp>

#include <vulkan/vk_enum_string_helper.h>

#define VK_WRAP_CRASH(__Cond) do {                                               \
    VkResult __Result = (__Cond);                                                \
    if (__Result != VK_SUCCESS) {                                                \
        slrd::debug::crit ("Internal VK error: {}", string_VkResult (__Result)); \
    }                                                                            \
} while (0)

#define VK_WRAP_RETURN(__Cond, __RetVal) do { \
    VkResult __Result = (__Cond);             \
    if (__Result != VK_SUCCESS)               \
        return __RetVal;                      \
} while (0)

#define VK_WRAP_RETURN_RESULT(__Cond) do { \
    VkResult __Result = (__Cond);          \
    if (__Result != VK_SUCCESS)            \
        return __Result;                   \
} while (0)

#define __WRAP_HELPER_ARGS(...) , ##__VA_ARGS__

#define RETURN_LOG_ERROR_IF(__Cond, __RetVal, ...) do {  \
    if (__Cond) {                                        \
        g_errorString = std::format (__VA_ARGS__);                      \
        return __RetVal;                                 \
    }                                                    \
} while (0)

#define RETURN_NV_LOG_ERROR_IF(__Cond, ...) do {  \
    if (__Cond) {                                 \
        g_errorString = std::format (__VA_ARGS__);               \
        return;                                   \
    }                                             \
} while (0)

#define VK_WRAP_RETURN_LOGERROR(__Cond, __RetVal, ...) do { \
    VkResult __Result = (__Cond);                           \
    if (__Result != VK_SUCCESS) {                           \
        g_errorString = std::format ("VK Error: {}",        \
                string_VkResult(__Result));                 \
        g_errorString += std::format (__VA_ARGS__);                         \
        return __RetVal;                                    \
    }                                                       \
} while (0)

#define VK_WRAP_RETURN_RESULT_LOGERROR(__Cond, ...) do {    \
    VkResult __Result = (__Cond);                           \
    if (__Result != VK_SUCCESS) {                           \
        g_errorString = std::format ("VK Error: {}",        \
                string_VkResult(__Result));                 \
        g_errorString += std::format (__VA_ARGS__);                         \
        return __Result;                                    \
    }                                                       \
} while (0)

#define WRAP_COND_RETURN(__Cond, __RetVal) do { \
    if ((__Cond)) return __RetVal;              \
} while (0)

/*#define SLRD_VK_PRINT_RESULT(__Result, ...) do {*/
/*} while (0)*/

#undef __WRAP_HELPER_ARGS

#endif /* #define __SLRD_VULKAN_ERROR_HPP__ */

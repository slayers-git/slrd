#ifndef __SLRD_FRAMEWORK_UTIL_HPP__
#define __SLRD_FRAMEWORK_UTIL_HPP__

#include "slrd/buffer.hpp"
#include "slrd/types.hpp"
#include <filesystem>
#include <vector>

namespace util {
    slrd::Ref<slrd::IShader> loadShader (slrd::IDevice *device, const std::vector<std::filesystem::path>& paths);

    slrd::Ref<slrd::IBuffer> createBufferWithData (slrd::IDevice *device,
            slrd::ICommandQueue *queue, slrd::BufferUsageFlags usage,
            void *data, size_t size);

    slrd::Ref<slrd::ITexture> loadCubeMap (slrd::IDevice *device, slrd::ICommandQueue *queue,
            const std::filesystem::path& path,
            slrd::ITextureView **tv);
}

#endif /* #define __SLRD_FRAMEWORK_UTIL_HPP__ */

#include <slrdframework/util.hpp>
#include <format>
#include <fstream>

#define STB_IMAGE_IMPLEMENTATION 1
#include <stb/stb_image.h>

#include <slrd/slrd.hpp>

namespace util {
    std::vector<uint32_t> loadShaderContents (const std::filesystem::path& path) {
        std::ifstream ifs (path.string (), std::ios::in | std::ios::binary | std::ios::ate);
        if (!ifs.is_open ())
            return {};

        std::ifstream::pos_type fileSize = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        if (fileSize & 3) {
            throw std::runtime_error ("Failed to load shader, since size is not divisible by 4.");
        }

        std::vector<uint32_t> bytes(fileSize >> 2);
        ifs.read((char *)bytes.data(), fileSize);
        ifs.close ();

        return bytes;
    }

    slrd::Ref<slrd::IBuffer> createBufferWithData (slrd::IDevice *device,
            slrd::ICommandQueue *queue, slrd::BufferUsageFlags usage,
            void *data, size_t size) {
        slrd::Ref<slrd::IBuffer> result, stagingBuffer;
        auto oneTimeBuffer = queue->getCommandBuffer (true);
        if (!oneTimeBuffer) {
            throw std::runtime_error ("Failed to create onetime command buffer");
        }

        slrd::BufferInfo stagingInfo {};
        stagingInfo.usage = 0;
        stagingInfo.coherent = true;
        stagingInfo.gpu = true;
        stagingInfo.size = size;
        stagingInfo.properties = slrd::BUFFER_PROPERTY_TRANSFER_SRC;

        stagingBuffer = device->createBuffer (stagingInfo);
        if (!stagingBuffer) {
            throw std::runtime_error ("Failed to create the staging buffer");
        }

        void *map = stagingBuffer->map ();
        if (!map) {
            throw std::runtime_error ("Failed to map the staging buffer");
        }

        memcpy (map, data, size);

        slrd::BufferInfo resultInfo;
        resultInfo.usage = usage;
        resultInfo.coherent = false;
        resultInfo.gpu = true;
        resultInfo.size = size;
        resultInfo.properties = slrd::BUFFER_PROPERTY_TRANSFER_DST;

        result = device->createBuffer (resultInfo);
        if (!result) {
            throw std::runtime_error ("Failed to create the buffer");
        }

        oneTimeBuffer->begin ();

        slrd::BufferCopyInfo copyInfo;
        copyInfo.srcBuffer = stagingBuffer.get ();
        copyInfo.dstBuffer = result.get ();
        copyInfo.size = size;
        oneTimeBuffer->copyBuffer (copyInfo);

        oneTimeBuffer->end ();

        slrd::SubmitInfo submitInfo {};
        submitInfo.commandBuffers = { &oneTimeBuffer, 1 };
        if (queue->submit (submitInfo)) {
            throw std::runtime_error ("Failed to submit one time command buffer");
        }

        queue->wait ();

        return result;
    }

    slrd::Ref<slrd::ITexture> loadCubeMap (slrd::IDevice *device, slrd::ICommandQueue *queue,
            const std::filesystem::path& path,
            slrd::ITextureView **view) {
        uint8_t *images[6] = {};

        namespace fs = std::filesystem;

        if (!fs::is_directory (path)) {
            throw std::runtime_error (std::format ("{} is not a directory", path.string ()));
        }

        uint32_t width, height;
        for (uint32_t i = 0; i < 6; ++i) {
            int w, h;
            static const char *exts[] = {
                ".jpg", ".png", ".tga", ""
            };

            std::string cpath;
            {
                bool found = false;
                for (uint32_t j = 0; j < sizeof (exts) / sizeof (exts[0]); ++j) {
                    cpath = std::format ("{}/{}{}", path.string (), i + 1,
                            exts[j]);
                    if (fs::exists (cpath)) {
                        found = true;
                        break;
                    }
                }
                if (!found)
                    throw std::runtime_error (std::format ("Cube face #{} doesn't exist " \
                                "(tried all supported extensions!)", i + 1));
            }

            uint8_t *image = stbi_load (cpath.c_str (), &w, &h, nullptr, 4);
            if (!image) {
                throw std::runtime_error (std::format ("Failed to load {}", cpath));
            }
            images[i] = image;

            if (i > 0) {
                if (!(w == width && h == height)) {
                    throw std::runtime_error ("Invalid sizes");
                }
            }

            width = w;
            height = h;
        }

        slrd::TextureInfo texInfo;
        texInfo.type = slrd::TEXTURE_TYPE_CUBE_MAP;
        texInfo.width = width;
        texInfo.height = height;
        texInfo.depth = 1;
        texInfo.usage = slrd::TEXTURE_USAGE_SAMPLED | slrd::TEXTURE_USAGE_TRANSFER_DST;
        texInfo.format = slrd::FORMAT_RGBA8_UNORM;
        texInfo.arrayLayers = 6;
        slrd::Ref<slrd::ITexture> texture = device->createTexture (texInfo);
        if (!texture) {
            throw std::runtime_error ("Failed to create the texture!");
        }

        slrd::BufferInfo bufInfo;
        bufInfo.usage = 0;
        bufInfo.gpu = true;
        bufInfo.coherent = true;
        bufInfo.size = width * height * 4 * 6;
        bufInfo.properties = slrd::BUFFER_PROPERTY_TRANSFER_SRC;
        slrd::Ref<slrd::IBuffer> buffer = device->createBuffer (bufInfo);
        if (!buffer) {
            throw std::runtime_error ("Failed to create buffer for the texture");
        }

        uint8_t *map = (uint8_t *)buffer->map ();
        if (!map) {
            throw std::runtime_error ("Failed to map the buffer for the texture");
        }

        std::vector<slrd::BufferTextureRegion> regions (6);
        for (uint32_t i = 0; i < 6; ++i) {
            uint32_t offset = width * height * 4 * i;

            memcpy (&map[offset], images[i], width * height * 4);

            /*regions[i].rows = width;*/
            /*regions[i].height = height;*/
            regions[i].offset = offset;
            regions[i].textureViewInfo.arrayLayer = i;
            regions[i].rect = { 0, 0, 0, width, height, 1 };
        }

        auto oneTime = queue->getCommandBuffer ();
        if (!oneTime) {
            throw std::runtime_error ("Failed to create one time command buffer");
        }

        oneTime->begin ();

            slrd::BufferTextureCopyInfo btInfo;
            btInfo.buffer = buffer.get ();
            btInfo.texture = texture.get ();
            btInfo.regions = regions;

            oneTime->copyBufferToImage (btInfo);

        oneTime->end ();

        slrd::SubmitInfo sbInfo;
        sbInfo.commandBuffers = { &oneTime, 1 };
        if (queue->submit (sbInfo)) {
            throw std::runtime_error ("Failed to submit to command queue");
        }
        queue->wait ();

        slrd::TextureViewInfo vInfo;
        vInfo.arrayLayers = 6;
        vInfo.arrayLayer = 0;
        vInfo.mipLevels  = 1;
        *view = texture->createTextureView (vInfo).get ();
        if (!view) {
            throw std::runtime_error ("Failed to create the texture view for the cube map!");
        }

        return texture;
    }

    slrd::Ref<slrd::IShader> loadShader (slrd::IDevice *device, const std::vector<std::filesystem::path>& paths) {
        slrd::Ref<slrd::IShader> shader;
        std::vector<slrd::ShaderBytecode> bytecodes (paths.size ());

        for (uint32_t i = 0; i < paths.size (); ++i) {
            const auto fc = loadShaderContents (paths[i]);
            if (fc.empty ()) {
                throw std::runtime_error ("Failed to load shader!");
            }

            bytecodes[i] = { fc.data (), fc.size () };
        }

        slrd::ShaderInfo shInfo;
        shInfo.bytecodes = bytecodes;
        shader = device->createShader (shInfo);
        if (!shader) {
            throw std::runtime_error ("Failed to create the main shader");
        }

        return shader;
    }
}

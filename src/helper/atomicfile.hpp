/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_HELPER_ATOMICFILE_HPP__
#define __SLRD_HELPER_ATOMICFILE_HPP__

#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <random>
#include <string_view>
#include <system_error>

namespace slrd {
    /**
     * @brief Write data to file atomically (temp + rename)
     *
     * @param path   Path to the original file
     * @param writer Callable to execute when the file is open */
    template<typename Writer>
    inline bool atomicWriteToFile(std::string_view path, Writer&& writer) noexcept {
        if (path.empty())
            return false;

        std::filesystem::path target{ path.begin(), path.end() };

        std::random_device rd;
        std::uint32_t token = rd();
        /* To avoid writing overlapping tempfiles  */
        std::filesystem::path tmp = std::format ("{}.tmp.{:08x}", target.string(), token);

        std::error_code ec;
        std::ofstream fs(tmp, std::ios::binary);
        if (!fs.is_open())
            return false;

        writer(fs);
        fs.flush();

        if (!fs.good()) {
            fs.close();

            std::filesystem::remove(tmp, ec);
            return false;
        }
        fs.close();

        std::filesystem::rename(tmp, target, ec);
        if (ec) {
            std::filesystem::remove(tmp, ec);
            return false;
        }

        return true;
    }
};

#endif /* #define __SLRD_HELPER_ATOMICFILE_HPP__ */
